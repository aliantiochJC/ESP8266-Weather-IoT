# 09 — GitHub Media and Screenshots

A polished GitHub repository benefits from a few screenshots.

## Recommended screenshots

### 1. Hardware
Photo of the NodeMCU connected by USB.

Suggested filename:

```text
media/hardware-nodemcu.jpg
```

### 2. Serial Monitor
Show:

```text
ROUTER CONNECTED!
STA IP: ...
HTTP Response Code: 200
WEATHER INFORMATION
```

Suggested filename:

```text
media/serial-weather.png
```

### 3. Web dashboard
Show the phone browser with:

- Access Point information
- Router / STA information
- Weather
- LED controls
- Heartbeat controls
- PWM information

Suggested filename:

```text
media/web-dashboard.png
```

## Adding an image to README

After creating a `media/` directory:

```markdown
![ESP8266 Web Dashboard](media/web-dashboard.png)
```

## Recommended README image order

1. Final web dashboard
2. Hardware
3. Architecture diagram
4. Serial Monitor result

Keep secrets out of screenshots. Blur or crop Wi-Fi passwords and private network information if necessary.
