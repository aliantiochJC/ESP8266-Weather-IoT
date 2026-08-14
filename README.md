# 🌤️ ESP8266 Weather & IoT Control System

A hands-on **NodeMCU ESP8266 IoT project** that combines Wi-Fi networking, a web control panel, PWM LED control, heartbeat animation, and live weather data from the **Open-Meteo API**.

The project was built incrementally so that every subsystem could be tested and understood before being combined.

---

## 📸 Project Overview

The ESP8266 can operate in two Wi-Fi roles at the same time:

```text
                         INTERNET
                            │
                            ▼
                     ┌─────────────┐
                     │    ROUTER   │
                     └──────┬──────┘
                            │
                      Wi-Fi / STA
                            │
                            ▼
                  ┌───────────────────┐
                  │     ESP8266       │
                  │   NodeMCU 1.0     │
                  │                   │
                  │  Access Point     │
                  │  Wi-Fi Client     │
                  │  Web Server       │
                  │  HTTPS Client     │
                  │  JSON Parser      │
                  │  PWM Controller   │
                  │  Heartbeat LED    │
                  └─────────┬─────────┘
                            │
                          ESPap
                            │
                            ▼
                         📱 Phone
```

The phone can open the ESP8266 web interface and control the LED while the ESP8266 independently connects to a router to access Internet services.

---

## ✨ Features

### Networking
- ESP8266 Access Point (`ESPap`)
- Wi-Fi Client / Station (STA) connection to a router
- AP + STA simultaneous operation
- AP IP address and STA IP address reporting
- Connected AP client count
- Router RSSI, gateway and connection state
- Router reconnection attempts

### LED / PWM
- PWM-controlled built-in LED
- 5 kHz PWM frequency
- 10-bit PWM range (`0–1023`)
- Adjustable brightness: **20%, 50%, 90%**
- Heartbeat-style brightness animation
- Active-low LED handling

### Web Dashboard
- Access Point information
- Router / STA information
- LED brightness buttons
- Heartbeat speed buttons
- PWM information
- Weather information
- MANUAL / WEATHER control mode

### Weather API
- HTTPS request to Open-Meteo
- HTTP response validation
- JSON parsing with ArduinoJson
- Temperature
- Relative humidity
- Wind speed
- Weather code
- Human-readable weather condition
- Observation time
- Periodic updates

---

## 🧭 Development Stages

The project was developed in stages:

| Stage | Description | Status |
|---|---|---|
| 1 | PWM + heartbeat LED | ✅ |
| 2 | Access Point + Web Server | ✅ |
| 3 | Web brightness control + network information | ✅ |
| 4 | Web heartbeat speed control | ✅ |
| 5 | AP + STA router connection | ✅ |
| 6 | Open-Meteo API connection | ✅ |
| 7 | HTTPS + JSON parsing | ✅ |
| 8 | Weather data on web dashboard | ✅ |
| 9 | Weather-based brightness mode | ✅ |

---

# 🛠️ Hardware

- **NodeMCU 1.0 (ESP-12E Module)**
- USB cable
- Computer with Arduino IDE
- 2.4 GHz Wi-Fi router
- Optional external LED and resistor for future expansion

> The built-in NodeMCU LED is typically connected to an active-low GPIO, so its PWM value is inverted in software.

---

# 💻 Software

- Arduino IDE
- ESP8266 board support
- ESP8266WiFi
- ESP8266WebServer
- WiFiClientSecure
- ESP8266HTTPClient
- [ArduinoJson](https://arduinojson.org/)

---

# 📚 Core Concepts

## PWM

PWM means **Pulse Width Modulation**.

The project uses:

```text
Frequency = 5000 Hz
Resolution = 10-bit
Range = 0–1023
```

The user-facing brightness percentage is mapped onto the PWM range.

Examples:

```text
0%   → 0
25%  → ~256
50%  → ~512
75%  → ~768
100% → 1023
```

Because the built-in LED is active-low, the actual raw PWM value is inverted before being written to the pin.

---

## Heartbeat

The heartbeat effect is created by gradually increasing and decreasing brightness:

```text
10 → maximum → 10
```

twice, followed by a pause:

```text
💓 💓 ........ 💓 💓
```

The speed is controlled through:

```text
FADE_DELAY
HEART_DELAY
BEAT_PAUSE
```

Available profiles:

```text
SLOW
NORMAL
FAST
```

---

# 📡 Network Architecture

The ESP8266 uses:

```cpp
WiFi.mode(WIFI_AP_STA);
```

This enables both interfaces.

### Access Point

```text
SSID: ESPap
AP IP: 192.168.4.1
```

Phone access:

```text
http://192.168.4.1
```

### Station / Router

The ESP8266 connects to the configured router and receives an IP using DHCP.

Example:

```text
STA IP: 192.168.1.37
```

These are two different network interfaces belonging to the same ESP8266.

---

# 🌐 Web Server

The web server runs on:

```cpp
ESP8266WebServer server(80);
```

The main route is:

```cpp
server.on("/", handleRoot);
```

The brightness control uses:

```text
/set?brightness=20
/set?brightness=50
/set?brightness=90
```

The heartbeat speed uses:

```text
/speed?value=SLOW
/speed?value=NORMAL
/speed?value=FAST
```

The control mode uses:

```text
/mode?value=MANUAL
/mode?value=WEATHER
```

---

# 🌤️ Weather API

The project uses the [Open-Meteo Weather API](https://open-meteo.com/en/docs).

Example request:

```text
https://api.open-meteo.com/v1/forecast
?latitude=36.8120
&longitude=34.6410
&current=temperature_2m,relative_humidity_2m,wind_speed_10m,weather_code
```

The API response is JSON.

Example:

```json
{
  "current": {
    "time": "2026-08-14T08:30",
    "temperature_2m": 30.9,
    "relative_humidity_2m": 72,
    "wind_speed_10m": 10.8,
    "weather_code": 2
  }
}
```

The project extracts:

```text
temperature_2m
relative_humidity_2m
wind_speed_10m
weather_code
time
```

---

# 🔐 HTTPS

The weather API is accessed using:

```cpp
WiFiClientSecure client;
```

and:

```cpp
HTTPClient http;
http.begin(client, WEATHER_URL);
int httpCode = http.GET();
```

A successful request produced:

```text
HTTP Response Code: 200
```

### Development certificate setting

During development we used:

```cpp
client.setInsecure();
```

This disables server certificate verification.

**Do not treat this as a production security configuration.** For a real deployment, certificate validation should be configured appropriately.

---

# 🧩 JSON Parsing

ArduinoJson is used to convert the response into accessible data.

```cpp
DynamicJsonDocument doc(2048);

DeserializationError error =
    deserializeJson(doc, payload);
```

Then:

```cpp
JsonObject current = doc["current"];
```

and values are read:

```cpp
float temperature = current["temperature_2m"];
int humidity = current["relative_humidity_2m"];
float windSpeed = current["wind_speed_10m"];
int weatherCode = current["weather_code"];
```

This transforms:

```text
Raw JSON
   ↓
Parsed JSON
   ↓
Typed variables
```

---

# 🌦️ Weather Codes

The project converts weather codes into readable text.

Examples:

```text
0       Clear Sky
1       Mainly Clear
2       Partly Cloudy
3       Overcast
45/48   Fog
51-55   Drizzle
61-65   Rain
71-75   Snow
80-82   Rain Showers
95      Thunderstorm
96/99   Thunderstorm with Hail
```

Always check the official Open-Meteo documentation for the authoritative weather-code list:

https://open-meteo.com/en/docs

---

# 💡 Weather-Based LED Mode

The web dashboard provides two modes:

```text
MANUAL
WEATHER
```

### MANUAL

The user chooses:

```text
20%
50%
90%
```

### WEATHER

Brightness is selected from the weather code.

Prototype mapping:

| Condition | Maximum Brightness |
|---|---:|
| Clear Sky | 90% |
| Mainly Clear | 80% |
| Partly Cloudy | 70% |
| Overcast | 50% |
| Fog | 40% |
| Drizzle | 40% |
| Rain | 30% |
| Snow | 40% |
| Rain Showers | 30% |
| Thunderstorm | 20% |

This demonstrates a basic embedded decision pipeline:

```text
Weather API
    ↓
JSON
    ↓
weather_code
    ↓
decision logic
    ↓
brightness
    ↓
PWM
    ↓
LED
```

---

# 🔄 Update Intervals

The prototype separates web refreshes from API requests.

### Web page

```text
2 seconds
```

Used only to refresh the local dashboard.

### Weather API

```text
60 seconds
```

Used to request new weather data.

This prevents the web page refresh interval from causing unnecessary external API requests.

---

# 🖥️ Example Serial Output

```text
ROUTER CONNECTED!
STA IP: 192.168.1.xxx

REQUESTING WEATHER DATA
HTTP Response Code: 200

WEATHER INFORMATION
------------------------------
Temperature : 30.9 °C
Humidity    : 72 %
Wind Speed  : 10.8 km/h
Weather Code: 2
Condition   : Partly Cloudy
Observation : 2026-08-14T08:30
------------------------------
```

---

# 🧪 Testing

Recommended test order:

1. Confirm the correct board:
   `NodeMCU 1.0 (ESP-12E Module)`

2. Confirm Serial Monitor:
   `115200 baud`

3. Test AP:
   - phone sees `ESPap`
   - phone connects successfully

4. Test web server:
   - open `192.168.4.1`

5. Test STA:
   - ESP8266 connects to the router
   - Serial Monitor shows `STA IP`

6. Test Internet:
   - weather API returns HTTP `200`

7. Test JSON:
   - temperature, humidity, wind and weather code appear

8. Test web dashboard:
   - weather section displays the parsed data

9. Test WEATHER mode:
   - weather code changes the LED maximum brightness

---

# 🐛 Troubleshooting

## Only dots appear during Wi-Fi connection

The ESP8266 is waiting for the router connection.

Check:

- SSID
- password
- router availability
- 2.4 GHz compatibility

## HTTP response is not 200

Check:

- router connection
- Internet access
- API URL
- HTTPS/TLS behavior

## JSON parsing fails

Check:

- whether the API returned JSON
- whether the requested fields changed
- ArduinoJson installation
- document capacity

## Web page disappears when the phone disconnects

This is expected when using the ESP8266 Access Point.

The phone must remain connected to `ESPap` to reach:

```text
192.168.4.1
```

The ESP8266 itself can continue running normally.

---

# 🔒 GitHub Security

Never commit real credentials.

Before pushing:

```text
SEARCH THE REPOSITORY FOR:
- real SSID
- real Wi-Fi password
- API keys
- tokens
- certificates/private keys
```

Use placeholders:

```cpp
const char* ROUTER_SSID = "YOUR_WIFI_NAME";
const char* ROUTER_PASSWORD = "YOUR_WIFI_PASSWORD";
```

For repositories containing secrets accidentally, remove the secret from Git history as well as the current file and rotate the credential.

---

# 📁 Repository Structure

```text
.
├── README.md
├── GITHUB_PUBLISHING_NOTES.md
├── docs/
│   ├── 01-project-overview.md
│   ├── 02-wifi-and-networking.md
│   ├── 03-open-meteo-api.md
│   ├── 04-https-request.md
│   ├── 05-json-parsing.md
│   ├── 06-weather-codes.md
│   ├── 07-weather-web-dashboard.md
│   └── 08-weather-led-control.md
└── src/
    └── weather_api_test.ino
```

---

# 🚀 Future Improvements

Possible next steps:

- Replace blocking `delay()` calls with a fully `millis()`-based non-blocking scheduler.
- Add a proper automatic router reconnect state machine.
- Add a weather refresh timestamp.
- Add more weather variables.
- Add forecast data.
- Add charts to the web dashboard.
- Add a configurable location through the web interface.
- Add external LED RGB control.
- Add authenticated web controls.
- Replace `setInsecure()` with proper TLS certificate validation.
