#include <WiFi.h>
#include <Wire.h>
#include <RTClib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// WIFI
const char* ssid = "presanna.s";
const char* password = "password";

WiFiServer server(80);

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// RTC
RTC_DS3231 rtc;

// Pins
#define BUZZER 25
#define HALL_PIN 26
#define IR_PIN 27

// MED TIMES
int medHours[] = {8, 14, 21};
int medMinutes[] = {0, 0, 0};

bool doseTaken[3] = {false, false, false};
bool doseMissed[3] = {false, false, false};

unsigned long lastBeep = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  pinMode(BUZZER, OUTPUT);
  pinMode(HALL_PIN, INPUT);
  pinMode(IR_PIN, INPUT);

  rtc.begin();

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  WiFi.begin(ssid, password);
  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop() {

  WiFiClient client = server.available();

  DateTime now = rtc.now();

  int hour = now.hour();
  int minute = now.minute();
  int second = now.second();

  int hall = digitalRead(HALL_PIN);
  int ir = digitalRead(IR_PIN);

  bool boxOpen = (hall == LOW);

  int currentDose = -1;
  bool alertActive = false;

  // CURRENT DOSE
  for (int i = 0; i < 3; i++) {
    if (hour == medHours[i] && minute == medMinutes[i] && !doseTaken[i]) {
      currentDose = i;
      alertActive = true;
    }
  }

  // MEDICINE TAKEN
  if (boxOpen && ir == LOW && currentDose != -1) {
    doseTaken[currentDose] = true;
    alertActive = false;
  }

  // MISSED DOSE
  for (int i = 0; i < 3; i++) {
    if (hour == medHours[i] && minute > medMinutes[i] + 5 && !doseTaken[i]) {
      doseMissed[i] = true;
    }
  }

  // BUZZER
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

  // NEXT DOSE
  String nextDose = "--";
  for (int i = 0; i < 3; i++) {
    if (hour < medHours[i] || (hour == medHours[i] && minute < medMinutes[i])) {
      nextDose = String(medHours[i]) + ":" + String(medMinutes[i]);
      break;
    }
  }

  // OLED DISPLAY
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0, 0);
  display.print("Time:");
  display.print(hour); display.print(":");
  display.print(minute); display.print(":");
  display.println(second);

  display.setCursor(0, 8);
  display.print("Box:");
  display.println(boxOpen ? "OPEN" : "CLOSE");

  display.setCursor(0, 16);
  display.print("Med:");
  if (currentDose != -1 && doseTaken[currentDose]) display.println("Taken");
  else if (currentDose != -1 && doseMissed[currentDose]) display.println("Missed");
  else display.println("Waiting");

  display.setCursor(0, 24);
  display.print("Next:");
  display.println(nextDose);

  display.display();

  // 🌐 API RESPONSE FOR DASHBOARD
  if (client) {

    String request = client.readStringUntil('\r');
    client.flush();

    // JSON RESPONSE
    String json = "{";
    json += "\"time\":\"" + String(hour) + ":" + String(minute) + "\",";
    json += "\"box\":\"" + String(boxOpen ? "OPEN" : "CLOSE") + "\",";
    
    String medStatus = "Pending";
    if (currentDose != -1 && doseTaken[currentDose]) medStatus = "Taken";
    else if (currentDose != -1 && doseMissed[currentDose]) medStatus = "Missed";

    json += "\"medicine\":\"" + medStatus + "\",";
    json += "\"alert\":\"" + String(alertActive ? "ON" : "OFF") + "\",";
    json += "\"next\":\"" + nextDose + "\",";
    json += "\"hr\":85";  // future MAX30102
    json += "}";

    // SEND RESPONSE
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println();
    client.println(json);

    client.stop();
  }

  delay(200);
}