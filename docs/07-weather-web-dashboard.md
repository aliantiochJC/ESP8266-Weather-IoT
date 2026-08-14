# 07 — Weather Web Dashboard

After Serial Monitor parsing worked, weather data was stored in global variables so the ESP8266 web server could display it.

Example variables:

```cpp
float weatherTemperature;
int weatherHumidity;
float weatherWindSpeed;
int weatherCode;

String weatherCondition;
String weatherObservationTime;
```

## Why store the values?

The API is not queried every time someone opens the web page.

Instead:

```text
API request
   |
   v
weather variables in ESP8266 RAM
   |
   +--> Serial Monitor
   |
   +--> Web page
```

The prototype updates weather data every 60 seconds while the web page refreshes every 2 seconds.

This avoids requesting the external API every time the browser refreshes.

## Dashboard section

The web panel displays:

```text
WEATHER

Temperature
Humidity
Wind Speed
Condition
Weather Code
Observation Time
```

The panel is therefore both a control interface and a monitoring interface.
