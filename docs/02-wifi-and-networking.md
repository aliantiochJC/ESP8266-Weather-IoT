# 02 — Wi-Fi and Networking

## ESP8266 as Access Point

The Access Point is created with:

```cpp
WiFi.mode(WIFI_AP);
WiFi.softAP(AP_SSID, AP_PASSWORD);
```

The ESP8266 becomes the Wi-Fi network itself.

Example:

```text
SSID: ESPap
IP:   192.168.4.1
```

A phone connected to this network can open:

```text
http://192.168.4.1
```

## ESP8266 as Wi-Fi Client

The ESP8266 can also connect to an existing router:

```cpp
WiFi.begin(ROUTER_SSID, ROUTER_PASSWORD);
```

The router normally assigns the ESP8266 an address using DHCP.

Example:

```text
STA IP: 192.168.1.37
```

## AP + STA

The final network mode uses:

```cpp
WiFi.mode(WIFI_AP_STA);
```

This allows both roles at the same time:

```text
AP side:
ESP8266 -> ESPap -> phone

STA side:
ESP8266 -> home router -> internet
```

## Important distinction

`WiFi.softAPIP()` returns the AP-side address.

`WiFi.localIP()` returns the STA-side address.

`WiFi.softAPgetStationNum()` reports the number of clients connected to the ESP8266 Access Point.

`WiFi.status()` is used for the ESP8266's STA connection state.

## Why the web page disappears when the phone disconnects

The phone must remain connected to `ESPap` to reach `192.168.4.1`.

Disconnecting the phone does not stop the ESP8266. It only removes the phone's network path to the ESP8266 AP.
