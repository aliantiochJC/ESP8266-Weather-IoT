# 05 — JSON Parsing with ArduinoJson

## Raw JSON

The first API test printed the entire response.

That proved:

```text
ESP8266 -> Internet -> API -> HTTP 200 -> JSON
```

but the raw response was not convenient to use.

## ArduinoJson

The next step was parsing:

```cpp
#include <ArduinoJson.h>
```

The response is deserialized with:

```cpp
DynamicJsonDocument doc(2048);

DeserializationError error =
    deserializeJson(doc, payload);
```

The library converts the JSON text into an in-memory representation that can be accessed by key.

ArduinoJson documents `deserializeJson()` as the function that parses JSON input into a JSON document.

Official documentation:

https://arduinojson.org/v7/api/json/deserializejson/

## Reading the current object

```cpp
JsonObject current = doc["current"];
```

Then:

```cpp
float temperature = current["temperature_2m"];
int humidity = current["relative_humidity_2m"];
float windSpeed = current["wind_speed_10m"];
int weatherCode = current["weather_code"];
```

The important idea is:

```text
JSON key
   |
   v
program variable
```

## Error handling

Always check the returned `DeserializationError`:

```cpp
if (error) {
    Serial.println(error.c_str());
}
```

This protects the program from assuming that every API response is valid JSON.

## Memory consideration

ESP8266 has limited RAM. For larger API responses, filtering only the fields required by the application is a useful optimization.
