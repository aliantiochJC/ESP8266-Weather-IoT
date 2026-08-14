# 04 — HTTPS Request

## Why HTTPS?

The weather endpoint uses HTTPS, so the ESP8266 must use a secure network client.

The project uses:

```cpp
#include <WiFiClientSecure.h>
```

and:

```cpp
WiFiClientSecure client;
```

## HTTP client

The request is made using:

```cpp
HTTPClient http;

http.begin(client, WEATHER_URL);

int httpCode = http.GET();
```

A successful HTTP request returns:

```text
HTTP Response Code: 200
```

HTTP 200 indicates that the request itself succeeded.

## Development shortcut

During the learning/testing phase we used:

```cpp
client.setInsecure();
```

This disables certificate verification.

That is useful for getting the prototype working, but it should not be treated as a production security configuration. A production device should validate the server certificate.

## Error handling

The code checks:

- router connection,
- HTTP connection creation,
- HTTP response code,
- JSON parsing result.

This makes failures visible in Serial Monitor instead of silently failing.
