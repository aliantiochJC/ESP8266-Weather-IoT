# 03 — Open-Meteo API

## Why Open-Meteo?

For the learning stage, Open-Meteo was convenient because the project could request weather data directly without first building an API-key management workflow.

Open-Meteo's documentation provides current weather variables including:

- temperature at 2 m
- relative humidity at 2 m
- wind speed at 10 m
- weather code

Official documentation:

https://open-meteo.com/en/docs

## Location

The request uses latitude and longitude.

Example values used during testing:

```text
Latitude:  36.8120
Longitude: 34.6410
```

## Request

The endpoint was built as:

```text
https://api.open-meteo.com/v1/forecast
?latitude=36.8120
&longitude=34.6410
&current=temperature_2m,relative_humidity_2m,wind_speed_10m,weather_code
```

Conceptually:

```text
ESP8266
  |
  | HTTPS GET
  v
Open-Meteo
  |
  | JSON response
  v
ESP8266
```

## Example response

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
