# 01 — Project Overview

## Goal

The goal was to turn a NodeMCU ESP8266 into a small IoT controller that can:

1. create its own Wi-Fi network,
2. connect to a router,
3. host a web control panel,
4. control a PWM heartbeat LED,
5. fetch weather data from an online API,
6. parse the returned JSON,
7. display the weather through Serial Monitor and the web page.

## Development philosophy

The project was built incrementally:

```text
LED
 -> PWM
 -> Heartbeat
 -> Access Point
 -> Web Server
 -> Web Controls
 -> AP + STA
 -> Internet
 -> Weather API
 -> JSON
 -> Web Dashboard
 -> Automatic Weather Mode
```

Each stage was tested before the next stage was added.

## Final data flow

```text
Phone
  |
  | HTTP
  v
ESP8266 Web Server
  |
  +--> LED brightness / heartbeat
  |
  +--> network status
  |
  +--> weather display

ESP8266 STA
  |
  | HTTPS GET
  v
Open-Meteo
  |
  | JSON
  v
ArduinoJson
  |
  v
Weather variables
```
