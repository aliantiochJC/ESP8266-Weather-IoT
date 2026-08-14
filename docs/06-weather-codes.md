# 06 — Weather Codes

The Open-Meteo response includes a `weather_code`.

The project maps selected WMO-style codes to readable text:

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

For the tested response:

```text
weather_code = 2
```

the project displays:

```text
Condition: Partly Cloudy
```

The complete and current mapping should always be checked against the API documentation:

https://open-meteo.com/en/docs
