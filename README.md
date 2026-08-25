# 🌦️ ESP8266 IoT Weather & PWM Control System

<p align="center">
  <img src="https://img.shields.io/badge/ESP8266-NodeMCU%20ESP--12E-blue?style=for-the-badge&logo=espressif" alt="ESP8266">
  <img src="https://img.shields.io/badge/Arduino-IDE-00979D?style=for-the-badge&logo=arduino" alt="Arduino IDE">
  <img src="https://img.shields.io/badge/WiFi-AP%20%2B%20STA-2196F3?style=for-the-badge&logo=wifi" alt="WiFi">
  <img src="https://img.shields.io/badge/IoT-Project-orange?style=for-the-badge" alt="IoT">
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Open--Meteo-Weather%20API-43A047?style=for-the-badge" alt="Open-Meteo">
  <img src="https://img.shields.io/badge/HTTPS-Secure%20Communication-7E57C2?style=for-the-badge" alt="HTTPS">
  <img src="https://img.shields.io/badge/ArduinoJson-JSON-00ACC1?style=for-the-badge" alt="ArduinoJson">
  <img src="https://img.shields.io/badge/Google%20Sheets-PWM%20Control-34A853?style=for-the-badge&logo=google-sheets" alt="Google Sheets">
</p>

<p align="center">
  <strong>📡 Wi-Fi • 💡 PWM • ❤️ Heartbeat • 🌦️ Weather • 🌐 Web Dashboard • 📊 Google Sheets</strong>
</p>

---

## 📌 Project Overview

This project is an **ESP8266-based IoT control and monitoring system** developed using Arduino IDE.

The project started with a simple PWM LED control experiment and was progressively expanded with Wi-Fi communication, a local web server, heartbeat animation, network monitoring, weather data and remote control functionality.

The main purpose is to combine the ESP8266's embedded capabilities with real-time web control and external internet services.

The current system brings together:

* 📡 Wi-Fi Access Point
* 🌐 Wi-Fi Station / Router connection
* 💡 PWM LED control
* ❤️ Heartbeat animation
* 🎛️ Web-based control
* 🌦️ Open-Meteo weather API
* 🔐 HTTPS communication
* 🧩 JSON data processing
* 📶 Network monitoring
* 🌤️ Weather-based control
* 📊 Google Sheets → PWM development

---

# 🛠️ Hardware

The project is based on:

* **NodeMCU 1.0 (ESP-12E Module)**
* ESP8266 microcontroller
* USB connection
* Wi-Fi router
* Built-in LED
* Computer for development

<p align="center">
  <img src="media/hardware-setup.jpeg" width="250" alt="Hardware Setup">
</p>
---

# 💻 Software & Libraries

The project is developed with:

* Arduino IDE
* ESP8266 Arduino Core
* ArduinoJson

Main libraries:

```cpp
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
```

### Arduino IDE Board

```text
NodeMCU 1.0 (ESP-12E Module)
```

### Serial Monitor

```text
115200 baud
```

---

# 📡 Wi-Fi AP + STA

One of the important parts of the project is the simultaneous use of **Access Point** and **Station** modes.

```cpp
WiFi.mode(WIFI_AP_STA);
```

This allows the ESP8266 to create its own local network while also connecting to an existing router.

### 🔵 ESP8266 Access Point

The ESP8266 creates:

```text
SSID : ESPap
IP   : 192.168.4.1
```

A phone or computer can connect directly to the ESP8266 and open the web interface.

```text
http://192.168.4.1
```

### 🟢 Router / Station Mode

The ESP8266 also connects to the configured router:

```cpp
WiFi.begin(ROUTER_SSID, ROUTER_PASSWORD);
```

After successful connection, the ESP8266 receives an STA IP address.

Example:

```text
ROUTER CONNECTED ✅

STA IP: 192.168.1.200

RSSI: -59 dBm
```

This router connection is required for external services such as Open-Meteo and Google Apps Script.

---

# 📶 Network Details

The dashboard provides live network information including:

```text
📡 Router Status
🌐 STA IP
🚪 Gateway
📶 RSSI
👥 AP Clients
```

The ESP8266 obtains these values directly from the Wi-Fi subsystem.

For example:

```cpp
WiFi.localIP();
WiFi.gatewayIP();
WiFi.RSSI();
WiFi.softAPgetStationNum();
```

This section is especially useful when troubleshooting router and internet connectivity.

---

# 🌐 Local Web Dashboard

The ESP8266 runs its own local HTTP server.

After connecting to `ESPap`, the dashboard can be opened at:

```text
http://192.168.4.1
```

The interface provides a central control and monitoring area for:

* 📡 Local Wi-Fi
* 🌐 Router connection
* 🌦️ Weather API
* 🎛️ Control Mode
* 💡 LED Control
* ❤️ Heartbeat Speed
* 📶 Network Details
* 🌤️ Weather Details
* ⚡ PWM information

<p align="center">
  <img src="media/web-dashboard.jpeg" width="250" alt="Web Dashboard">
</p>

---

# 💡 PWM LED CONTROL

The LED is controlled using PWM.

The project uses a 10-bit PWM range:

```text
0 – 1023
```

The user interface uses a simpler percentage system:

```text
0% – 100%
```

The selected percentage is converted into the corresponding PWM value.

### 🎚️ Brightness Slider

The dashboard includes a live brightness slider.

The intended behaviour is:

```text
Move Slider
     ↓
Brightness Value
     ↓
ESP8266
     ↓
PWM
     ↓
LED
```

The brightness is updated while the slider is being moved rather than requiring the user to release it first.

Quick brightness controls are also available for testing.

---

# ❤️ HEARTBEAT CONTROL

The LED can operate using a heartbeat-style animation.

Instead of simply switching between ON and OFF, the LED gradually increases and decreases its brightness.

Conceptually:

```text
             █████
           ██     ██
         ██         ██
        █             █
───────                 ───────
```

The heartbeat is implemented using a **non-blocking state machine** with `millis()` timing.

This is important because long `delay()` operations can prevent the ESP8266 from responding quickly to:

* Wi-Fi events
* Web requests
* Status updates
* Weather requests

### Heartbeat Speeds

The interface provides:

```text
🐢 SLOW
⚙️ NORMAL
⚡ FAST
```

Each speed uses different timing parameters for fading and pauses.

---

# 🎛️ CONTROL MODES

The current interface is designed around three control modes.

## 🔵 MANUAL

Manual mode allows the user to directly control the LED brightness from the dashboard.

```text
Browser
   ↓
Brightness
   ↓
ESP8266
   ↓
PWM
   ↓
LED
```

This mode is mainly used for direct testing and manual operation.

---

## 🌦️ WEATHER

Weather mode connects the external weather information to the LED control.

The process is:

```text
Open-Meteo
     ↓
JSON Response
     ↓
Weather Code
     ↓
Weather Condition
     ↓
Brightness Decision
     ↓
PWM
     ↓
LED
```

Different weather conditions can therefore produce different LED behaviour.

---

## 📊 GOOGLE SHEET

Google Sheets control is the latest addition to the project.

The goal is to allow a spreadsheet cell to control the ESP8266 LED brightness.

The planned communication path is:

```text
Google Sheet
     ↓
Google Apps Script
     ↓
HTTPS
     ↓
ESP8266
     ↓
PWM
     ↓
LED
```

The selected spreadsheet cell contains a value between:

```text
0 – 100
```

For example:

```text
A1 = 75
```

should result in approximately:

```text
LED Brightness = 75%
```

The Google Sheets system is currently under development and testing.

---

# 🌦️ OPEN-METEO WEATHER API

The ESP8266 communicates with the Open-Meteo API to obtain current weather information.

The current location used by the project is:

```text
Latitude  : 36.8120
Longitude : 34.6410
```

The current request includes:

```text
temperature_2m
relative_humidity_2m
wind_speed_10m
weather_code
```

The API request follows this structure:

```text
https://api.open-meteo.com/v1/forecast
?latitude=36.8120
&longitude=34.6410
&current=temperature_2m,relative_humidity_2m,wind_speed_10m,weather_code
```

---

# 🌤️ WEATHER DETAILS

The dashboard displays the weather information received from the API.

Current information includes:

```text
🌡️ Temperature
💧 Humidity
💨 Wind Speed
🔢 Weather Code
🌤️ Condition
🕒 Observation Time
```

<p align="center">
  <img src="media/weather-api.jpeg" width="250" alt="Weather API">
</p>

---

# 🌈 WEATHER VISUALIZATION

The interface was redesigned so that weather information is not presented only as plain text.

The dashboard can visually represent conditions such as:

```text
☀️ Clear Sky
🌤️ Mainly Clear
⛅ Partly Cloudy
☁️ Overcast
🌫️ Fog
🌦️ Drizzle
🌧️ Rain
❄️ Snow
🌧️ Rain Showers
⛈️ Thunderstorm
```

The purpose is to connect the actual API data with the visual appearance of the dashboard.

This creates a more direct relationship between:

```text
Weather Data
     ↓
Dashboard
     ↓
Control Logic
     ↓
LED
```

---

# 🔐 HTTPS COMMUNICATION

External weather communication uses HTTPS.

The main components are:

```cpp
WiFiClientSecure
HTTPClient
```

The connection process is:

```text
ESP8266
   │
   ├── DNS Lookup
   │
   ├── TCP 443
   │
   ├── HTTPS GET
   │
   ▼
Open-Meteo
   │
   ▼
JSON Response
```

During development, the Serial Monitor is used to diagnose the connection.

A successful connection may produce:

```text
DNS lookup: api.open-meteo.com

DNS OK: 94.130.142.35

Testing TCP port 443...

TCP 443 OK.

Sending HTTPS GET request...

HTTP Response Code: 200
```

If the request fails, the serial output helps determine whether the problem is related to:

* DNS
* TCP
* HTTPS
* HTTP
* JSON parsing

### Development TLS Note

The current prototype uses:

```cpp
client.setInsecure();
```

This is being used for development and testing. Certificate validation should be implemented for a production system.

---

# 🧩 JSON PARSING

The weather API returns a JSON response.

ArduinoJson is used to process the response.

Example:

```cpp
DynamicJsonDocument doc(2048);

DeserializationError error =
    deserializeJson(doc, payload);
```

The current weather object is accessed through:

```cpp
JsonObject current = doc["current"];
```

The ESP8266 then extracts:

```text
Temperature
Humidity
Wind Speed
Weather Code
Observation Time
```

These values are used by the dashboard and weather control logic.

---

# 🔄 LIVE SYSTEM STATUS

The browser communicates with the ESP8266 through a status endpoint.

The system can provide information such as:

```text
Router connection
Brightness
Control mode
Heartbeat speed
STA IP
Gateway
RSSI
AP clients
Weather availability
Weather data
Google Sheet state
```

The dashboard periodically requests the current status so that the displayed information reflects the actual ESP8266 state.

---

# 🌐 WEB SERVER ENDPOINTS

The ESP8266 web server provides endpoints for the dashboard.

### Main page

```text
/
```

### System status

```text
/status
```

### Brightness

```text
/set
```

### Control mode

```text
/mode
```

### Heartbeat speed

```text
/speed
```

These endpoints allow the browser interface to communicate with the embedded system without requiring an external server.

---

# 🛠️ NETWORK & WEATHER DEBUGGING

During development, one of the main challenges was maintaining reliable communication between the ESP8266, router and external weather service.

The system therefore includes several diagnostic steps.

A typical successful network sequence is:

```text
ROUTER CONNECTED ✅

STA IP: 192.168.1.200

RSSI: -59 dBm

HTTP SERVER STARTED
```

The weather connection can then be checked separately:

```text
REQUESTING WEATHER DATA

DNS lookup: api.open-meteo.com

DNS OK

Testing TCP port 443...

TCP 443 OK.

Sending HTTPS GET request...

HTTP Response Code: 200
```

This makes it possible to identify whether a failure occurs before or after internet connectivity has been established.

---

# 🧪 WEATHER API TEST

A separate minimal Weather API sketch is included in the source directory.

The purpose of this sketch is to test the weather service independently from the complete web dashboard.

The test performs:

```text
Wi-Fi Connection
       ↓
Open-Meteo Request
       ↓
HTTPS
       ↓
JSON Parsing
       ↓
Serial Monitor
```

The output includes:

```text
Temperature
Humidity
Wind Speed
Weather Code
Condition
Observation Time
```

Keeping this test separate makes it easier to determine whether a weather problem comes from the API connection itself or from the integrated dashboard code.

---

# 📁 PROJECT STRUCTURE

```text
ESP8266-IoT-Project/
│
├── README.md
│
├── src/
│   ├── weather_api_test.ino
│   └── ESP8266_IoT_Dashboard.ino
│
├── media/
│   ├── hardware-setup.jpeg
│   ├── web-dashboard.jpeg
│   └── weather-api.jpeg
│
└── docs/
```

### `src/`

Contains the Arduino source code.

### `media/`

Contains screenshots and photographs used to document the project.

### `docs/`

Contains additional project documentation and development notes.

---

# 🔐 SECURITY

Sensitive credentials must not be committed to a public GitHub repository.

Use placeholders:

```cpp
const char* ROUTER_SSID =
    "YOUR_WIFI_NAME";

const char* ROUTER_PASSWORD =
    "YOUR_WIFI_PASSWORD";
```

For Google Sheets:

```cpp
const char* GOOGLE_SHEET_URL =
    "YOUR_APPS_SCRIPT_WEB_APP_URL";

const char* GOOGLE_SHEET_TOKEN =
    "YOUR_SECRET_TOKEN";
```

Never publish:

```text
❌ Wi-Fi password
❌ Google Apps Script token
❌ Private credentials
❌ Secret API keys
```

---

# 📈 PROJECT DEVELOPMENT

The project has been developed incrementally.

### 1A — PWM & Live LED Control

Initial PWM control was implemented and the web slider was improved so that brightness could respond during movement.

### 1B-1 — Wi-Fi & Router

Access Point and Station functionality were integrated.

Router status, STA IP and RSSI were added to the system.

### 1B-2 — System Status

The dashboard was connected to the ESP8266 status endpoint so that network and system information could be updated dynamically.

### 1C — Web Interface

The dashboard was redesigned with dedicated sections for:

```text
📡 Network
🎛️ Control
💡 LED
❤️ Heartbeat
🌦️ Weather
⚡ PWM
```

### 1D-3 — Weather Integration

Open-Meteo communication, HTTPS requests and JSON parsing were integrated into the project.

### 1D-4 — Weather Visualization

Weather conditions were connected to the visual dashboard and LED control logic.

### Google Sheets — Current Development

The latest development stage is the connection of a Google Sheets cell to the ESP8266 PWM system.

This part remains under testing until the complete remote-control process works reliably.

---

# 🚦 CURRENT PROJECT STATUS

### 🟢 Implemented

```text
✓ ESP8266 Access Point
✓ Wi-Fi AP + STA
✓ Router connection
✓ Local Web Server
✓ PWM LED control
✓ Live brightness control
✓ Heartbeat animation
✓ Heartbeat speed control
✓ Network Details
✓ Open-Meteo API
✓ HTTPS communication
✓ JSON parsing
✓ Weather Details
✓ Weather-based control
✓ Visual weather interface
✓ Non-blocking heartbeat
```

### 🟠 Under Development

```text
→ Google Sheets communication
→ Google Apps Script integration
→ Google Sheet control mode
→ Remote PWM control
→ Final dashboard synchronization
```

---

# 🔮 NEXT DEVELOPMENT STEPS

The next stage of the project will focus on stability and integration.

Planned improvements include:

```text
→ Complete Google Sheets PWM control
→ Improve dashboard synchronization
→ Improve network reconnection
→ Improve weather API reliability
→ Improve visual weather presentation
→ Add additional sensors
→ Add data logging
→ Explore MQTT
→ Explore OTA firmware updates
```

---

# 📷 PROJECT MEDIA

The repository contains real project images documenting the hardware and interface.

### Hardware

![Hardware](media/hardware-setup.jpeg)

### Web Dashboard

![Dashboard](media/web-dashboard.jpeg)

### Weather API

![Weather](media/weather-api.jpeg)

---

# ⚡ PROJECT CONCEPT

The project can be summarized by the following development path:

```text
        ESP8266
           │
           ▼
        📡 Wi-Fi
           │
           ▼
      🌐 Web Server
           │
           ▼
       🎛️ Control
           │
           ▼
        💡 PWM
           │
           ▼
       ❤️ Heartbeat
           │
           ▼
      🌦️ Weather API
           │
           ▼
      📊 Google Sheets
```

The objective is to continue transforming the ESP8266 from a simple microcontroller experiment into a complete IoT platform capable of **communication, monitoring, data processing and remote physical control**.

---

<p align="center">

### 🌐 ESP8266 • 📡 Wi-Fi • 🌦️ Weather • 💡 PWM • ❤️ Heartbeat • 📊 IoT

**Developed with Arduino IDE**

</p>
