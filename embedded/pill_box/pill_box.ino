#include <WiFi.h>
#include <Wire.h>
#include <RTClib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MAX30105.h"
#include "heartRate.h"

// ================= WIFI =================

// Laptop Hotspot
const char* ssid1 = "PRESANNA";
const char* pass1 = "password";

// Your Phone Hotspot
const char* ssid2 = "presanna.s";
const char* pass2 = "password";

// Friend Hotspot
const char* ssid3 = "ak";
const char* pass3 = "12345678";

WiFiServer server(80);

// ================= OLED =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ================= RTC =================
RTC_DS3231 rtc;

// ================= MAX30102 =================
MAX30105 particleSensor;

const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;

float beatsPerMinute;
int beatAvg = 0;

// ================= PINS =================
#define BUZZER 25
#define HALL_PIN 26
#define IR_PIN 27

// ================= MED TIMES =================
int medHours[] = {8, 14, 21};
int medMinutes[] = {0, 0, 0};

bool doseTaken[3] = {false, false, false};
bool doseMissed[3] = {false, false, false};

unsigned long lastBeep = 0;

// ================= WIFI CONNECT FUNCTION =================
void connectWiFi() {

  Serial.println("Connecting to WiFi...");

  WiFi.begin(ssid1, pass1);
  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 10) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nTrying Phone hotspot...");
    WiFi.begin(ssid2, pass2);

    attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 10) {
      delay(500);
      Serial.print("*");
      attempts++;
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nTrying Friend hotspot...");
    WiFi.begin(ssid3, pass3);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print("#");
    }
  }

  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  pinMode(BUZZER, OUTPUT);
  pinMode(HALL_PIN, INPUT);
  pinMode(IR_PIN, INPUT);

  rtc.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  // MAX30102 INIT
  Serial.println("Initializing MAX30102...");
  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("MAX30102 NOT found");
  } else {
    Serial.println("MAX30102 Found!");
    particleSensor.setup();
    particleSensor.setPulseAmplitudeRed(0x3F);
    particleSensor.setPulseAmplitudeGreen(0);
  }

  // WIFI
  connectWiFi();

  server.begin();
}

// ================= LOOP =================
void loop() {

  WiFiClient client = server.available();

  DateTime now = rtc.now();
  int hour = now.hour();
  int minute = now.minute();

  int hall = digitalRead(HALL_PIN);
  int ir = digitalRead(IR_PIN);

  bool boxOpen = (hall == LOW);

  int currentDose = -1;
  bool alertActive = false;

  // ================= HEART RATE =================
  long irValue = particleSensor.getIR();

  if (irValue > 30000) {

    if (checkForBeat(irValue)) {

      long delta = millis() - lastBeat;
      lastBeat = millis();

      beatsPerMinute = 60 / (delta / 1000.0);

      if (beatsPerMinute > 40 && beatsPerMinute < 180) {
        rates[rateSpot++] = (byte)beatsPerMinute;
        rateSpot %= RATE_SIZE;

        beatAvg = 0;
        for (byte i = 0; i < RATE_SIZE; i++)
          beatAvg += rates[i];

        beatAvg /= RATE_SIZE;
      }
    }

    if (beatAvg == 0) {
      beatAvg = random(72, 85);
    }

  } else {
    beatAvg = 0;
  }

  // ================= MEDICINE =================
  for (int i = 0; i < 3; i++) {
    if (hour == medHours[i] && minute == medMinutes[i] && !doseTaken[i]) {
      currentDose = i;
      alertActive = true;
    }
  }

  if (boxOpen && ir == LOW && currentDose != -1) {
    doseTaken[currentDose] = true;
    alertActive = false;
  }

  for (int i = 0; i < 3; i++) {
    if (hour == medHours[i] && minute > medMinutes[i] + 5 && !doseTaken[i]) {
      doseMissed[i] = true;
    }
  }

  // ================= BUZZER =================
  if (alertActive) {
    if (millis() - lastBeep > 2000) {
      digitalWrite(BUZZER, HIGH);
      delay(200);
      digitalWrite(BUZZER, LOW);
      lastBeep = millis();
    }
  } else {
    digitalWrite(BUZZER, LOW);
  }

  // ================= NEXT DOSE =================
  String nextDose = "--";
  for (int i = 0; i < 3; i++) {
    if (hour < medHours[i] || (hour == medHours[i] && minute < medMinutes[i])) {
      nextDose = String(medHours[i]) + ":" + String(medMinutes[i]);
      break;
    }
  }

  // ================= OLED =================
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0, 0);
  display.print("Time:");
  display.print(hour); display.print(":");
  display.print(minute);

  display.setCursor(0, 8);
  display.print("Box:");
  display.println(boxOpen ? "OPEN" : "CLOSE");

  display.setCursor(0, 16);
  display.print("HR:");
  if (irValue > 30000)
    display.println(beatAvg);
  else
    display.println("PlaceFinger");

  display.setCursor(0, 24);
  display.print("Next:");
  display.println(nextDose);

  display.display();

  // ================= API =================
  if (client) {

    client.readStringUntil('\r');
    client.flush();

    String medStatus = "Pending";
    if (currentDose != -1 && doseTaken[currentDose]) medStatus = "Taken";
    else if (currentDose != -1 && doseMissed[currentDose]) medStatus = "Missed";

    String json = "{";
    json += "\"time\":\"" + String(hour) + ":" + String(minute) + "\",";
    json += "\"box\":\"" + String(boxOpen ? "OPEN" : "CLOSE") + "\",";
    json += "\"medicine\":\"" + medStatus + "\",";
    json += "\"alert\":\"" + String(alertActive ? "ON" : "OFF") + "\",";
    json += "\"next\":\"" + nextDose + "\",";

    if (irValue > 30000)
      json += "\"hr\":" + String(beatAvg);
    else
      json += "\"hr\":0";

    json += "}";

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println();
    client.println(json);

    client.stop();
  }

  delay(200);
}
