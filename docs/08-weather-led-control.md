# 08 — Weather-Based LED Control

After the API was working, weather data was connected to the LED logic.

## Manual mode

In MANUAL mode, the user selects:

```text
20%
50%
90%
```

The heartbeat uses the selected value as its maximum brightness.

## Weather mode

In WEATHER mode, the weather code determines the maximum brightness.

Example prototype mapping:

```text
Clear Sky        -> 90%
Mainly Clear     -> 80%
Partly Cloudy    -> 70%
Overcast         -> 50%
Fog              -> 40%
Drizzle          -> 40%
Rain             -> 30%
Snow             -> 40%
Rain Showers     -> 30%
Thunderstorm     -> 20%
```

## Decision chain

```text
Open-Meteo
   |
   v
weather_code
   |
   v
getWeatherBrightness()
   |
   v
maximum brightness
   |
   v
heartbeat
   |
   v
PWM
   |
   v
LED
```

## Important design choice

The prototype keeps MANUAL and WEATHER modes separate.

Pressing a manual brightness button switches back to MANUAL mode. Selecting WEATHER mode returns control to the weather-based rule.

This makes it possible to test automatic behavior without removing manual control.
