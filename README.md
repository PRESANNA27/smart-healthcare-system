# Smart Healthcare Monitoring System

A real-time embedded + IoT system for multi-patient medicine tracking and health monitoring using ESP32 and a QR-based web dashboard.

---

## Overview

This project integrates an ESP32-based pill monitoring system with a browser-based dashboard.
Patients are identified using QR codes, and their data is displayed along with real-time sensor updates.

---

## Features

* QR-based patient selection (URL encoded)
* Real-time ESP32 data integration (HTTP polling)
* Medicine schedule tracking (Taken / Missed / Pending)
* IV dosage and patient medical details
* Local data storage (localStorage)
* Responsive dashboard UI
* Multi-patient support (2 patients)

---

## System Architecture

```
QR Code → Browser Dashboard → ESP32 → Sensors
                ↑
         Local Storage (Patient Data)
```

---

## Tech Stack

* Frontend: HTML, CSS, JavaScript
* Embedded: ESP32 (Arduino framework)
* Communication: HTTP (WiFi)
* Server: Python HTTP server
* Storage: Browser localStorage

---

## Hardware

* ESP32
* IR Sensor
* Hall Effect Sensor
* Buzzer + LED
* OLED Display (SSD1306)
* RTC Module (DS3231)
* MAX30102 (Heart Rate Sensor)

---

## ESP32 API

Endpoint:

```
http://<esp32-ip>
```

Example response:

```json
{
  "time": "12:30",
  "box": "OPEN",
  "medicine": "Taken",
  "alert": "ON",
  "next": "14:00",
  "hr": 78
}
```

---

## QR Format

Each QR encodes a patient-specific URL:

```
http://<host-ip>:8000/dashboard.html?patient=ID001
http://<host-ip>:8000/dashboard.html?patient=ID002
```

---

## Getting Started

### 1. Run local server

```
python -m http.server 8000
```

### 2. Open dashboard

```
http://<your-ip>:8000/dashboard.html
```

### 3. Connect devices

* Laptop, ESP32, and phone must be on same network

### 4. Scan QR

* Opens dashboard with auto-selected patient

---

## Data Flow

```
ESP32 → JSON → fetch() → Dashboard → UI Update
```

---

## Key Logic

* URL-based routing using `URLSearchParams`
* Dynamic QR generation using `window.location`
* Polling ESP32 every 2 seconds
* State handling for medicine tracking

---

## Limitations

* Local network only (no cloud)
* Data stored in browser (not persistent across devices)

---

## Future Improvements

* Cloud database integration
* Mobile app
* Notification system (SMS / Email)
* Multi-device sync

---

## Author

Presanna S

---

## License

MIT License
