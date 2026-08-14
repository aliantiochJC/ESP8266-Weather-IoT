/*
 * ESP8266 Weather API - Minimal Test
 *
 * Purpose:
 *   Connect to a router, request current weather from Open-Meteo,
 *   parse the JSON response with ArduinoJson, and print the
 *   selected values to Serial Monitor.
 *
 * IMPORTANT:
 *   Replace the Wi-Fi placeholders before uploading.
 *   Do not commit real credentials to GitHub.
 */

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

const char* WEATHER_URL =
  "https://api.open-meteo.com/v1/forecast"
  "?latitude=36.8120"
  "&longitude=34.6410"
  "&current=temperature_2m,"
  "relative_humidity_2m,"
  "wind_speed_10m,"
  "weather_code";

String weatherDescription(int code) {
  switch (code) {
    case 0: return "Clear Sky";
    case 1: return "Mainly Clear";
    case 2: return "Partly Cloudy";
    case 3: return "Overcast";
    case 45:
    case 48: return "Fog";
    case 51:
    case 53:
    case 55: return "Drizzle";
    case 61:
    case 63:
    case 65: return "Rain";
    case 71:
    case 73:
    case 75: return "Snow";
    case 80:
    case 81:
    case 82: return "Rain Showers";
    case 95: return "Thunderstorm";
    case 96:
    case 99: return "Thunderstorm with Hail";
    default: return "Unknown";
  }
}

void getWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi is not connected.");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure(); // Learning/test only; validate certs in production.

  HTTPClient http;

  if (!http.begin(client, WEATHER_URL)) {
    Serial.println("HTTP begin failed.");
    return;
  }

  int httpCode = http.GET();

  Serial.print("HTTP Response Code: ");
  Serial.println(httpCode);

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();

    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print("JSON parsing failed: ");
      Serial.println(error.c_str());
      http.end();
      return;
    }

    JsonObject current = doc["current"];

    float temperature = current["temperature_2m"];
    int humidity = current["relative_humidity_2m"];
    float windSpeed = current["wind_speed_10m"];
    int weatherCode = current["weather_code"];
    const char* observationTime = current["time"];

    Serial.println();
    Serial.println("================================");
    Serial.println("WEATHER INFORMATION");
    Serial.println("================================");

    Serial.print("Temperature : ");
    Serial.print(temperature);
    Serial.println(" °C");

    Serial.print("Humidity    : ");
    Serial.print(humidity);
    Serial.println(" %");

    Serial.print("Wind Speed  : ");
    Serial.print(windSpeed);
    Serial.println(" km/h");

    Serial.print("Weather Code: ");
    Serial.println(weatherCode);

    Serial.print("Condition   : ");
    Serial.println(weatherDescription(weatherCode));

    Serial.print("Observation : ");
    Serial.println(observationTime);

    Serial.println("================================");
  } else {
    Serial.println("Weather request failed.");
  }

  http.end();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("ESP8266 WEATHER API TEST");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");

  unsigned long startTime = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - startTime < 20000) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected.");
    Serial.print("ESP8266 IP: ");
    Serial.println(WiFi.localIP());
    getWeather();
  } else {
    Serial.println("WiFi connection failed.");
  }
}

void loop() {
  static unsigned long lastRequest = 0;
  unsigned long now = millis();

  if (now - lastRequest >= 60000) {
    lastRequest = now;
    getWeather();
  }

  delay(10);
}
