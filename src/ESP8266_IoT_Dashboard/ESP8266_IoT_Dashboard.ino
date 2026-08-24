#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

// =====================================================
// 1. ACCESS POINT
// =====================================================

const char* AP_SSID = "ESPap";
const char* AP_PASSWORD = "thereisnospoon";


// =====================================================
// 2. ROUTER / WIFI CLIENT
// =====================================================

const char* ROUTER_SSID = "abcd";
const char* ROUTER_PASSWORD = "efgh";


// =====================================================
// 3. WEB SERVER
// =====================================================

ESP8266WebServer server(80);


// =====================================================
// 4. LED
// =====================================================

#define LED_PIN LED_BUILTIN


// =====================================================
// 5. PWM
// =====================================================

const int PWM_FREQUENCY = 5000;
const int PWM_RANGE = 1023;


// =====================================================
// 6. BRIGHTNESS
// =====================================================

const int MIN_BRIGHTNESS = 10;

// MANUAL moddaki seçili değer
int selectedBrightness = 90;


// =====================================================
// GOOGLE SHEETS CONTROL
// =====================================================

// Paste the deployed Google Apps Script Web App URL here.
// Example: https://script.google.com/macros/s/DEPLOYMENT_ID/exec
const char* GOOGLE_SHEET_URL =
  "https://script.google.com/macros/s/AKfycbzV-exdvH220IQch0-sT3w1lNJgxvG94k_d_Y8pYFQJy6rie-U5pvbBIfwWLCpC2padaA/exec";

// Keep this token private. Do NOT publish the real token
// in a public GitHub repository.
const char* GOOGLE_SHEET_TOKEN =
  "73918264";

// Poll the Google Sheet every 10 seconds.
const unsigned long GOOGLE_SHEET_INTERVAL = 10000;

int sheetBrightness = 50;
bool sheetAvailable = false;
unsigned long lastGoogleSheetRequest = 0;
String sheetStatus = "Not read yet";
int sheetHttpCode = 0;


// =====================================================
// 7. CONTROL MODE
// =====================================================

// MANUAL, WEATHER veya SHEET
String controlMode = "MANUAL";


// =====================================================
// 8. HEARTBEAT
// =====================================================

String heartbeatSpeed = "NORMAL";

int FADE_DELAY = 10;
int HEART_DELAY = 100;
int BEAT_PAUSE = 600;


// =====================================================
// 9. ROUTER CHECK
// =====================================================

unsigned long lastWiFiCheck = 0;

const unsigned long WIFI_CHECK_INTERVAL = 15000;

unsigned long lastWiFiReconnectAttempt = 0;


// =====================================================
// 10. WEATHER LOCATION
// =====================================================

// Mersin
const float LATITUDE = 36.8120;
const float LONGITUDE = 34.6410;


// =====================================================
// 11. WEATHER API
// =====================================================

const char* WEATHER_URL =
  "https://api.open-meteo.com/v1/forecast"
  "?latitude=36.8120"
  "&longitude=34.6410"
  "&current=temperature_2m,"
  "relative_humidity_2m,"
  "wind_speed_10m,"
  "weather_code";


// =====================================================
// 12. WEATHER VARIABLES
// =====================================================

bool weatherAvailable = false;
bool weatherStale = false;

float weatherTemperature = 0.0;
int weatherHumidity = 0;
float weatherWindSpeed = 0.0;
int weatherCode = -1;

String weatherCondition = "No data";
String weatherObservationTime = "No data";
String weatherStatus = "Not read yet";
int weatherHttpCode = 0;

unsigned long lastWeatherRequest = 0;

const unsigned long WEATHER_INTERVAL = 60000;


// =====================================================
// GOOGLE SHEET -> PWM
// =====================================================

bool googleSheetConfigured() {
  return
    String(GOOGLE_SHEET_URL).indexOf("PASTE_") == -1 &&
    String(GOOGLE_SHEET_TOKEN).indexOf("PASTE_") == -1;
}

String safeStatusText(const char* text) {
  String value = (text != nullptr) ? String(text) : "Unknown error";
  // The value is sent inside JSON; prevent an Apps Script message from
  // invalidating the dashboard response.
  value.replace("\"", "'");
  value.replace("\r", " ");
  value.replace("\n", " ");
  return value;
}


void updateFromGoogleSheet() {

  lastGoogleSheetRequest = millis();

  if (!googleSheetConfigured()) {
    sheetAvailable = false;
    sheetStatus = "URL or token is not configured";
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Google Sheet update skipped: router offline.");
    sheetAvailable = false;
    sheetStatus = "Router is offline";
    return;
  }

  Serial.println();
  Serial.println("================================");
  Serial.println("READING GOOGLE SHEET PWM");
  Serial.println("================================");

  WiFiClientSecure client;

  // Development/test mode.
  // Certificate verification should be enabled for production.
  client.setInsecure();

  // Google Apps Script can send a multi-kilobyte TLS handshake record.
  // RX must hold that record; keep TX smaller to preserve ESP8266 heap.
  client.setBufferSizes(8192, 1024);
  client.setTimeout(20000);

  HTTPClient http;

  // Google web apps may redirect.
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  http.setRedirectLimit(10);
  http.setReuse(false);
  // Keep HTTP/1.1 enabled. Apps Script ContentService returns a redirect
  // to script.googleusercontent.com; forcing HTTP/1.0 can drop that chain.
  http.useHTTP10(false);
  http.setTimeout(20000);

  String requestUrl =
    String(GOOGLE_SHEET_URL) +
    "?token=" +
    String(GOOGLE_SHEET_TOKEN);

  if (!http.begin(client, requestUrl)) {
    Serial.println("Google Sheet HTTP begin failed.");
    sheetAvailable = false;
    sheetStatus = "HTTPS connection could not start";
    return;
  }

  http.addHeader(
    "User-Agent",
    "ESP8266-IoT-GoogleSheet-PWM/1.0"
  );

  int httpCode = http.GET();
  sheetHttpCode = httpCode;

  Serial.print("Google Sheet HTTP Response: ");
  Serial.println(httpCode);

  if (httpCode < 0) {
    char sslErrorMessage[100] = {0};
    int sslError = client.getLastSSLError(
      sslErrorMessage,
      sizeof(sslErrorMessage)
    );
    Serial.print("Google Sheet TLS error: ");
    Serial.print(sslError);
    Serial.print(" - ");
    Serial.println(sslErrorMessage);
  }

  if (httpCode == HTTP_CODE_OK) {

    String payload =
      http.getString();

    Serial.print("Google Sheet response: ");
    Serial.println(payload);

    DynamicJsonDocument doc(256);

    DeserializationError error =
      deserializeJson(doc, payload);

    if (error) {

      Serial.print(
        "Google Sheet JSON parsing failed: "
      );

      Serial.println(
        error.c_str()
      );

      sheetAvailable = false;
      sheetStatus = String("Invalid JSON: ") + error.c_str();
      http.end();
      return;
    }

    bool ok =
      doc["ok"] | false;

    if (!ok) {

      const char* message =
        doc["error"] | "Unknown error";

      Serial.print(
        "Google Sheet error: "
      );

      Serial.println(message);

      sheetAvailable = false;
      // Show the Apps Script error in the dashboard as well as Serial Monitor.
      sheetStatus = String("Apps Script: ") + safeStatusText(message);
      http.end();
      return;
    }

    float rawValue =
      doc["value"] | -1.0;

    if (
      rawValue < 0.0 ||
      rawValue > 100.0
    ) {

      Serial.println(
        "Google Sheet value outside 0-100."
      );

      sheetAvailable = false;
      sheetStatus = "Value must be between 0 and 100";
      http.end();
      return;
    }

    sheetBrightness =
      constrain(
        (int)round(rawValue),
        0,
        100
      );

    sheetAvailable = true;
    sheetStatus = "Last read successful";

    Serial.print(
      "Google Sheet PWM value: "
    );

    Serial.print(
      sheetBrightness
    );

    Serial.println("%");
  }
  else {

    Serial.print(
      "Google Sheet request failed: "
    );

    Serial.println(
      http.errorToString(httpCode)
    );

    sheetAvailable = false;
    sheetStatus = http.errorToString(httpCode);
  }

  http.end();
}


void checkGoogleSheetUpdate() {

  // Do not perform a slow external HTTPS request while
  // MANUAL or WEATHER mode is active. This keeps the web
  // dashboard responsive and prevents mode/heartbeat controls
  // from being blocked by Google Apps Script.
  if (controlMode != "SHEET") {
    return;
  }

  if (!googleSheetConfigured()) {
    return;
  }

  unsigned long now = millis();

  if (
    now - lastGoogleSheetRequest >=
    GOOGLE_SHEET_INTERVAL
  ) {

    updateFromGoogleSheet();
  }
}


// =====================================================
// 13. WEATHER CODE -> TEXT
// =====================================================

String weatherDescription(int code) {

  switch (code) {

    case 0:
      return "Clear Sky";

    case 1:
      return "Mainly Clear";

    case 2:
      return "Partly Cloudy";

    case 3:
      return "Overcast";

    case 45:
    case 48:
      return "Fog";

    case 51:
    case 53:
    case 55:
      return "Drizzle";

    case 61:
    case 63:
    case 65:
      return "Rain";

    case 71:
    case 73:
    case 75:
      return "Snow";

    case 80:
    case 81:
    case 82:
      return "Rain Showers";

    case 95:
      return "Thunderstorm";

    case 96:
    case 99:
      return "Thunderstorm with Hail";

    default:
      return "Unknown";
  }
}


// =====================================================
// 14. WEATHER -> AUTOMATIC BRIGHTNESS
// =====================================================

int getWeatherBrightness() {

  if (!weatherAvailable) {

    // Hava durumu yoksa güvenli varsayılan
    return 20;
  }


  switch (weatherCode) {

    // Clear Sky
    case 0:
      return 90;

    // Mainly Clear
    case 1:
      return 80;

    // Partly Cloudy
    case 2:
      return 70;

    // Overcast
    case 3:
      return 50;

    // Fog
    case 45:
    case 48:
      return 40;

    // Drizzle
    case 51:
    case 53:
    case 55:
      return 40;

    // Rain
    case 61:
    case 63:
    case 65:
      return 30;

    // Snow
    case 71:
    case 73:
    case 75:
      return 40;

    // Rain Showers
    case 80:
    case 81:
    case 82:
      return 30;

    // Thunderstorm
    case 95:
    case 96:
    case 99:
      return 20;

    default:
      return 20;
  }
}


// =====================================================
// 15. BRIGHTNESS -> PWM
// =====================================================

int brightnessToDuty(int brightnessPercent) {

  int brightness = map(
    brightnessPercent,
    0,
    100,
    0,
    PWM_RANGE
  );

  // NodeMCU built-in LED active-low
  return PWM_RANGE - brightness;
}


// =====================================================
// 16. ACTUAL MAXIMUM BRIGHTNESS
// =====================================================

int getMaximumBrightness() {

  // GOOGLE SHEET MODE
  // Remote PWM control must work even when no
  // phone is connected to the ESP8266 AP.
  if (controlMode == "SHEET") {
    return sheetBrightness;
  }

  // WEATHER MODE
  if (controlMode == "WEATHER") {

    return getWeatherBrightness();
  }


  // MANUAL MODE
  return selectedBrightness;
}


// =====================================================
// 17. HEARTBEAT SPEED
// =====================================================

void setHeartbeatSpeed(String speed) {

  if (speed == "SLOW") {

    heartbeatSpeed = "SLOW";

    FADE_DELAY = 20;
    HEART_DELAY = 150;
    BEAT_PAUSE = 1000;
  }

  else if (speed == "FAST") {

    heartbeatSpeed = "FAST";

    FADE_DELAY = 5;
    HEART_DELAY = 50;
    BEAT_PAUSE = 250;
  }

  else {

    heartbeatSpeed = "NORMAL";

    FADE_DELAY = 10;
    HEART_DELAY = 100;
    BEAT_PAUSE = 600;
  }


  Serial.print(
    "Heartbeat Speed: "
  );

  Serial.println(
    heartbeatSpeed
  );
}


// =====================================================
// 18. ROUTER RECONNECT
// =====================================================

void checkRouterConnection() {

  unsigned long now = millis();

  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  if (now - lastWiFiReconnectAttempt < WIFI_CHECK_INTERVAL) {
    return;
  }

  lastWiFiReconnectAttempt = now;

  Serial.println();
  Serial.println("Router connection lost.");
  Serial.println("Trying to reconnect...");

  WiFi.begin(
    ROUTER_SSID,
    ROUTER_PASSWORD
  );
}


// =====================================================
// 19. WEATHER API
// =====================================================

void getWeather() {

  lastWeatherRequest = millis();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Weather update skipped: router offline.");
    weatherStatus = "Router is offline";
    return;
  }

  weatherStale = true;

  Serial.println();
  Serial.println("================================");
  Serial.println("REQUESTING WEATHER DATA");
  Serial.println("================================");

  const char* WEATHER_HOST = "api.open-meteo.com";
  IPAddress weatherIP;

  int dnsResult = WiFi.hostByName(WEATHER_HOST, weatherIP);

  if (dnsResult != 1) {
    Serial.println("DNS FAILED.");
    Serial.println("Keeping last known weather data.");
    weatherStatus = "DNS lookup failed";
    return;
  }

  WiFiClient tcpClient;

  if (!tcpClient.connect(weatherIP, 443)) {
    Serial.println("TCP 443 FAILED.");
    Serial.println("Keeping last known weather data.");
    weatherStatus = "TCP port 443 unavailable";
    tcpClient.stop();
    return;
  }

  Serial.println("TCP 443 OK.");
  tcpClient.stop();

  WiFiClientSecure client;
  client.setInsecure();
  client.setBufferSizes(512, 512);
  client.setTimeout(8000);

  HTTPClient http;

  if (!http.begin(client, WEATHER_URL)) {
    Serial.println("HTTP connection could not be started.");
    weatherStatus = "HTTPS connection could not start";
    return;
  }

  http.useHTTP10();
  http.setTimeout(8000);
  http.setReuse(false);
  http.addHeader("User-Agent", "ESP8266-IoT-Weather/1.0");

  int httpCode = http.GET();
  weatherHttpCode = httpCode;

  Serial.print("HTTP Response Code: ");
  Serial.println(httpCode);

  if (httpCode == HTTP_CODE_OK) {

    String payload = http.getString();
    DynamicJsonDocument doc(2048);

    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {

      JsonObject current = doc["current"];

      weatherTemperature = current["temperature_2m"];
      weatherHumidity = current["relative_humidity_2m"];
      weatherWindSpeed = current["wind_speed_10m"];
      weatherCode = current["weather_code"];

      const char* timeValue = current["time"];
      weatherObservationTime =
        (timeValue != nullptr) ? String(timeValue) : "No data";

      weatherCondition = weatherDescription(weatherCode);
      weatherAvailable = true;
      weatherStale = false;
      weatherStatus = "Last update successful";

      Serial.println("WEATHER UPDATE OK");
      Serial.print("Temperature: ");
      Serial.print(weatherTemperature);
      Serial.println(" °C");
    }
    else {
      Serial.print("JSON parsing failed: ");
      Serial.println(error.c_str());
      weatherStatus = String("Invalid API JSON: ") + error.c_str();
    }
  }
  else {
    Serial.print("Weather request failed: ");
    Serial.println(http.errorToString(httpCode));
    weatherStatus = http.errorToString(httpCode);
  }

  http.end();

  Serial.print("FREE HEAP: ");
  Serial.println(ESP.getFreeHeap());
}


// =====================================================
// 20. WEATHER UPDATE CHECK
// =====================================================

void checkWeatherUpdate() {

  unsigned long now = millis();

  // Always respect the 60-second API interval, even if
  // the previous request failed.
  if (now - lastWeatherRequest < WEATHER_INTERVAL) {
    return;
  }

  getWeather();
}



// =====================================================
// 21. LIVE STATUS API
// =====================================================

// This page stays in flash memory (PROGMEM), not in the small ESP8266 heap.
// Do not build a large dashboard with String concatenation at request time.
const char DASHBOARD_PAGE[] PROGMEM = R"html(
<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>ESP8266 Smart Hub</title><style>
:root{--ink:#14213d;--muted:#64748b;--card:#ffffffdc;--blue:#2563eb}*{box-sizing:border-box}body{margin:0;padding:14px;font-family:Arial,sans-serif;color:var(--ink);background:linear-gradient(135deg,#c7e7ff,#f3e8ff 48%,#bbf7d0);min-height:100vh}.shell{max-width:760px;margin:auto}.hero,.section{background:var(--card);border:1px solid #fff;border-radius:24px;box-shadow:0 12px 30px #1e3a8a24}.hero{padding:24px;text-align:center;background:linear-gradient(135deg,#0f172a,#2563eb 58%,#06b6d4);color:#fff}.hero h1{margin:0;font-size:22px}.weather{font-size:44px;font-weight:bold;margin:12px 0 4px}.sub{opacity:.82;font-size:13px}.section{padding:17px;margin-top:14px}h2{font-size:17px;margin:0 0 12px}.grid{display:grid;grid-template-columns:repeat(2,1fr);gap:9px}.item{padding:12px;border-radius:15px;background:#f8fafc}.label{font-size:10px;font-weight:bold;letter-spacing:.6px;color:var(--muted);text-transform:uppercase}.value{font-size:16px;font-weight:bold;margin-top:5px;overflow-wrap:anywhere}.modes,.speeds{display:grid;grid-template-columns:repeat(3,1fr);gap:8px}button{border:0;border-radius:14px;padding:14px 8px;font-weight:bold;color:#fff;background:#334155;cursor:pointer;min-height:48px}button:active{transform:scale(.97)}button.active{background:linear-gradient(135deg,#2563eb,#06b6d4);box-shadow:0 7px 16px #2563eb55}.speeds button.active{background:linear-gradient(135deg,#ef4444,#f97316)}input[type=range]{width:100%;accent-color:#2563eb;height:32px}.selected{display:flex;justify-content:space-between;margin:6px 0;font-weight:bold}.pill{display:inline-block;padding:6px 10px;border-radius:99px;background:#dbeafe;color:#1d4ed8;font-size:12px;font-weight:bold}.status{margin-top:10px;font-size:12px;color:var(--muted)}@media(max-width:430px){.grid{grid-template-columns:1fr}.modes,.speeds{grid-template-columns:1fr}.weather{font-size:38px}}
</style><style>
body{background:linear-gradient(135deg,#67e8f9,#c4b5fd 48%,#f9a8d4);background-size:220% 220%;animation:sky 12s ease infinite}@keyframes sky{50%{background-position:100% 50%}}.section:nth-of-type(2){background:linear-gradient(135deg,#dbeafe,#e0f2fe)}.section:nth-of-type(3){background:linear-gradient(135deg,#fef3c7,#ffedd5)}.section:nth-of-type(4){background:linear-gradient(135deg,#ffe4e6,#ffedd5)}.section:nth-of-type(5){background:linear-gradient(135deg,#dcfce7,#ccfbf1)}.section:nth-of-type(6){background:linear-gradient(135deg,#e0f2fe,#e0e7ff)}.section:nth-of-type(7){background:linear-gradient(135deg,#f3e8ff,#fae8ff)}.section:nth-of-type(2) h2{color:#1d4ed8}.section:nth-of-type(3) h2{color:#b45309}.section:nth-of-type(4) h2{color:#e11d48}.section:nth-of-type(5) h2{color:#047857}.section:nth-of-type(6) h2{color:#0369a1}.section:nth-of-type(7) h2{color:#7e22ce}.item{box-shadow:0 5px 12px #1e293b15}.item:nth-child(3n+1){background:linear-gradient(135deg,#fff,#eff6ff)}.item:nth-child(3n+2){background:linear-gradient(135deg,#fff,#fdf2f8)}.item:nth-child(3n){background:linear-gradient(135deg,#fff,#f0fdf4)}#manual{background:linear-gradient(135deg,#2563eb,#4f46e5)}#weather{background:linear-gradient(135deg,#0891b2,#0ea5e9)}#sheet{background:linear-gradient(135deg,#7c3aed,#c026d3)}#slow{background:linear-gradient(135deg,#0f766e,#14b8a6)}#normal{background:linear-gradient(135deg,#ea580c,#f59e0b)}#fast{background:linear-gradient(135deg,#dc2626,#f43f5e)}button.active{outline:3px solid #fff;filter:saturate(1.25);transform:translateY(-2px)}
</style></head><body><main class="shell"><section class="hero"><h1>🌐 ESP8266 SMART HUB</h1><div id="weatherIcon" style="font-size:42px">🌤️</div><div id="condition">Waiting for weather</div><div class="weather"><span id="temp">--</span> °C</div><div class="sub">Mersin · <span id="observation">No observation yet</span></div></section>
<section class="section"><h2>🎛️ Control Mode <span class="pill" id="mode">MANUAL</span></h2><div class="modes"><button id="manual" onclick="setMode('MANUAL')">MANUAL</button><button id="weather" onclick="setMode('WEATHER')">🌤 WEATHER</button><button id="sheet" onclick="setMode('SHEET')">📊 SHEET</button></div><div class="status" id="sheetState">Google Sheet: waiting</div></section>
<section class="section"><h2>💡 LED Control</h2><div class="selected"><span>Manual selection</span><span id="selected">--%</span></div><input id="brightness" type="range" min="0" max="100" value="50" oninput="changeBrightness(this.value)"><div class="selected"><span>Actual output</span><span id="output">--%</span></div></section>
<section class="section"><h2>❤️ Heartbeat <span class="pill" id="heartbeat">NORMAL</span></h2><div class="speeds"><button id="slow" onclick="setSpeed('SLOW')">SLOW</button><button id="normal" onclick="setSpeed('NORMAL')">NORMAL</button><button id="fast" onclick="setSpeed('FAST')">FAST</button></div></section>
<section class="section"><h2>📡 Network Details</h2><div class="grid"><div class="item"><div class="label">Router</div><div class="value" id="router">--</div></div><div class="item"><div class="label">Signal</div><div class="value" id="rssi">--</div></div><div class="item"><div class="label">STA IP</div><div class="value" id="ip">--</div></div><div class="item"><div class="label">Gateway / Channel</div><div class="value"><span id="gateway">--</span> / <span id="channel">--</span></div></div><div class="item"><div class="label">Router MAC</div><div class="value" id="mac">--</div></div><div class="item"><div class="label">AP clients</div><div class="value" id="clients">--</div></div></div></section>
<section class="section"><h2>🌦️ Weather Details</h2><div class="grid"><div class="item"><div class="label">Humidity</div><div class="value" id="humidity">--</div></div><div class="item"><div class="label">Wind</div><div class="value" id="wind">--</div></div><div class="item"><div class="label">Condition / Code</div><div class="value"><span id="weatherCondition">--</span> / <span id="weatherCode">--</span></div></div><div class="item"><div class="label">API status</div><div class="value" id="weatherState">--</div></div></div></section>
<section class="section"><h2>📊 Google Sheet Details</h2><div class="grid"><div class="item"><div class="label">Remote PWM</div><div class="value" id="sheetValue">--</div></div><div class="item"><div class="label">HTTP / Status</div><div class="value"><span id="sheetHttp">--</span> · <span id="sheetDetail">--</span></div></div></div></section></main>
<script>
let timer;const byId=id=>document.getElementById(id);function activeMode(value){['manual','weather','sheet'].forEach(x=>byId(x).classList.toggle('active',x===value.toLowerCase()))}function activeSpeed(value){['slow','normal','fast'].forEach(x=>byId(x).classList.toggle('active',x===value.toLowerCase()))}function setMode(v){activeMode(v);fetch('/mode?value='+v,{cache:'no-store'}).then(refresh)}function setSpeed(v){activeSpeed(v);fetch('/speed?value='+v,{cache:'no-store'}).then(refresh)}function changeBrightness(v){byId('selected').textContent=v+'%';clearTimeout(timer);timer=setTimeout(()=>fetch('/set?brightness='+v,{cache:'no-store'}).then(refresh),90)}function text(id,value){byId(id).textContent=value}function refresh(){fetch('/status?t='+Date.now(),{cache:'no-store'}).then(r=>r.json()).then(d=>{text('mode',d.mode);text('heartbeat',d.heartbeat);activeMode(d.mode);activeSpeed(d.heartbeat);text('selected',d.manualBrightness+'%');byId('brightness').value=d.manualBrightness;text('output',d.brightness+'%');text('router',d.routerConnected?'ONLINE':'OFFLINE');text('rssi',d.routerConnected?d.rssi+' dBm':'--');text('ip',d.staIP);text('gateway',d.gateway);text('channel',d.channel||'--');text('mac',d.routerMac);text('clients',d.apClients);text('temp',d.weatherAvailable?Number(d.temperature).toFixed(1):'--');text('condition',d.weatherAvailable?d.condition:'Weather unavailable');text('weatherCondition',d.condition);text('weatherCode',d.weatherCode);text('humidity',d.weatherAvailable?d.humidity+'%':'--');text('wind',d.weatherAvailable?Number(d.windSpeed).toFixed(1)+' km/h':'--');text('observation',d.observationTime);text('weatherState',(d.weatherHttpCode||'--')+' · '+d.weatherStatus);text('sheetValue',d.sheetBrightness+'%');text('sheetHttp',d.sheetHttpCode||'--');text('sheetDetail',d.sheetStatus);text('sheetState',d.sheetAvailable?'Google Sheet: connected · '+d.sheetBrightness+'%':'Google Sheet: '+d.sheetStatus);const c=Number(d.weatherCode);text('weatherIcon',c===0?'☀️':c>=61&&c<=82?'🌧️':c>=95?'⛈️':'🌤️')}).catch(()=>text('router','Status unavailable'))}refresh();setInterval(refresh,2500);
</script></body></html>)html";

void handleStatus() {

  int connectedDevices = WiFi.softAPgetStationNum();
  bool routerConnected = (WiFi.status() == WL_CONNECTED);
  int maximumBrightness = getMaximumBrightness();

  String json;
  json.reserve(1050);
  json += "{";

  json += "\"apClients\":";
  json += connectedDevices;
  json += ",";

  json += "\"routerConnected\":";
  json += routerConnected ? "true" : "false";
  json += ",";

  json += "\"brightness\":";
  json += maximumBrightness;
  json += ",";

  json += "\"manualBrightness\":";
  json += selectedBrightness;
  json += ",";

  json += "\"mode\":\"";
  json += controlMode;
  json += "\",";

  json += "\"sheetAvailable\":";
  json += sheetAvailable ? "true" : "false";
  json += ",";

  json += "\"sheetBrightness\":";
  json += sheetBrightness;
  json += ",";

  json += "\"sheetHttpCode\":";
  json += sheetHttpCode;
  json += ",";

  json += "\"sheetStatus\":\"";
  json += sheetStatus;
  json += "\",";

  json += "\"rssi\":";
  json += routerConnected ? WiFi.RSSI() : 0;
  json += ",";

  json += "\"channel\":";
  json += routerConnected ? WiFi.channel() : 0;
  json += ",";

  json += "\"routerMac\":\"";
  json += routerConnected ? WiFi.BSSIDstr() : "NONE";
  json += "\",";

  json += "\"staIP\":\"";
  json += routerConnected ? WiFi.localIP().toString() : "NONE";
  json += "\",";

  json += "\"gateway\":\"";
  json += routerConnected ? WiFi.gatewayIP().toString() : "NONE";
  json += "\",";

  json += "\"weatherAvailable\":";
  json += weatherAvailable ? "true" : "false";
  json += ",";

  json += "\"weatherStale\":";
  json += weatherStale ? "true" : "false";
  json += ",";

  json += "\"temperature\":";
  json += weatherTemperature;
  json += ",";

  json += "\"humidity\":";
  json += weatherHumidity;
  json += ",";

  json += "\"windSpeed\":";
  json += weatherWindSpeed;
  json += ",";

  json += "\"weatherCode\":";
  json += weatherCode;
  json += ",";

  json += "\"condition\":\"";
  json += weatherCondition;
  json += "\",";

  json += "\"observationTime\":\"";
  json += weatherObservationTime;
  json += "\",";

  json += "\"weatherHttpCode\":";
  json += weatherHttpCode;
  json += ",";

  json += "\"weatherStatus\":\"";
  json += weatherStatus;
  json += "\",";

  json += "\"heartbeat\":\"";
  json += heartbeatSpeed;
  json += "\"";

  json += "}";

  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  server.sendHeader("Pragma", "no-cache");
  server.send(200, "application/json", json);
}


// =====================================================
// 22. WEB PAGE
// =====================================================
void handleRoot() {

  // Serve the compact PROGMEM page.  The former page builder below is kept
  // only as reference and must not run: it exhausts ESP8266 heap memory.
  server.send_P(200, PSTR("text/html"), DASHBOARD_PAGE);
  return;

  int connectedDevices = WiFi.softAPgetStationNum();
  int maximumBrightness = getMaximumBrightness();

  String html;
  html.reserve(18000);

  Serial.print("WEB PAGE - Free heap before build: ");
  Serial.println(ESP.getFreeHeap());

  html += "<!DOCTYPE html>";
  html += "<html>";
  html += "<head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1, viewport-fit=cover'>";
  html += "<title>ESP8266 Smart Hub</title>";

  // ===================================================
  // VISUAL CSS
  // ===================================================

  html += "<style>";

  html += ":root{";
  html += "--ink:#14213d;";
  html += "--muted:#687386;";
  html += "--card:rgba(255,255,255,.82);";
  html += "--border:rgba(255,255,255,.72);";
  html += "--shadow:0 12px 35px rgba(31,41,55,.10);";
  html += "--radius:24px;";
  html += "}";

  html += "*{box-sizing:border-box;}";

  html += "html{scroll-behavior:smooth;}";

  html += "body{";
  html += "font-family:Arial,sans-serif;";
  html += "margin:0;";
  html += "padding:14px;";
  html += "min-height:100vh;";
  html += "color:var(--ink);";
  html += "background:linear-gradient(145deg,#eef5ff,#f8fbff 45%,#eef8f4);";
  html += "transition:background .5s ease;";
  html += "}";

  html += "body.theme-clear{background:linear-gradient(145deg,#fff1a8,#fff8df 45%,#e8f6ff);}";
  html += "body.theme-mainly-clear{background:linear-gradient(145deg,#ffe7a6,#f5f8e8 45%,#e8f6ff);}";
  html += "body.theme-cloudy{background:linear-gradient(145deg,#d8e8f5,#eff4f7 45%,#e6edf6);}";
  html += "body.theme-overcast{background:linear-gradient(145deg,#c7d0d9,#edf1f4 45%,#e0e7ed);}";
  html += "body.theme-rain{background:linear-gradient(145deg,#b6d8f1,#e6f1fb 45%,#d7e9f8);}";
  html += "body.theme-storm{background:linear-gradient(145deg,#4a4e80,#7c78a9 45%,#28334b);color:#fff;}";

  html += ".shell{max-width:980px;margin:0 auto;}";

  html += ".topbar{";
  html += "display:flex;";
  html += "justify-content:space-between;";
  html += "align-items:center;";
  html += "gap:12px;";
  html += "padding:8px 4px 14px;";
  html += "}";

  html += ".brand{font-weight:900;letter-spacing:.7px;font-size:18px;}";
  html += ".brand-sub{font-size:12px;color:var(--muted);margin-top:2px;}";

  html += ".header-status{";
  html += "display:inline-flex;";
  html += "align-items:center;";
  html += "gap:6px;";
  html += "padding:8px 12px;";
  html += "border-radius:999px;";
  html += "font-size:12px;";
  html += "font-weight:800;";
  html += "background:#dcfce7;";
  html += "color:#166534;";
  html += "}";

  html += ".header-status.local{background:#fff7d6;color:#8a5a00;}";
  html += ".header-status.offline{background:#ffe1e1;color:#a31616;}";

  html += ".hero{";
  html += "position:relative;";
  html += "overflow:hidden;";
  html += "padding:30px 20px 24px;";
  html += "border-radius:32px;";
  html += "box-shadow:var(--shadow);";
  html += "backdrop-filter:blur(8px);";
  html += "}";

  html += ".hero-glow{";
  html += "position:absolute;";
  html += "width:180px;height:180px;";
  html += "border-radius:50%;";
  html += "right:-40px;top:-55px;";
  html += "background:rgba(255,255,255,.32);";
  html += "filter:blur(8px);";
  html += "}";

  html += ".hero-icon{font-size:78px;line-height:1;margin-top:2px;}";
  html += ".hero-condition{font-size:22px;font-weight:900;letter-spacing:1.3px;text-transform:uppercase;margin-top:10px;}";
  html += ".hero-temp{font-size:64px;font-weight:900;line-height:1.08;margin:8px 0;}";
  html += ".hero-place{font-size:14px;opacity:.78;}";
  html += ".hero-metrics{display:flex;justify-content:center;gap:10px;flex-wrap:wrap;margin-top:16px;}";
  html += ".metric-pill{padding:10px 14px;border-radius:999px;background:rgba(255,255,255,.48);font-weight:700;font-size:13px;}";
  html += ".metric-pill span{font-weight:500;opacity:.9;}";

  html += ".section{";
  html += "margin-top:16px;";
  html += "padding:20px;";
  html += "border-radius:var(--radius);";
  html += "background:var(--card);";
  html += "border:1px solid var(--border);";
  html += "box-shadow:var(--shadow);";
  html += "backdrop-filter:blur(8px);";
  html += "}";

  html += ".section-title{";
  html += "display:flex;";
  html += "align-items:center;";
  html += "gap:10px;";
  html += "font-size:20px;";
  html += "font-weight:900;";
  html += "margin:0 0 14px;";
  html += "}";

  html += ".section-sub{font-size:12px;color:var(--muted);margin-top:-8px;margin-bottom:14px;}";

  html += ".status-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:12px;margin-top:16px;}";

  html += ".status-card{";
  html += "padding:18px;";
  html += "border-radius:22px;";
  html += "text-align:center;";
  html += "box-shadow:0 8px 22px rgba(31,41,55,.08);";
  html += "transition:transform .2s ease, box-shadow .2s ease, background .3s ease;";
  html += "}";

  html += ".status-card:hover{transform:translateY(-2px);box-shadow:0 12px 26px rgba(31,41,55,.11);}";

  html += ".status-icon{font-size:34px;line-height:1;margin-bottom:8px;}";
  html += ".status-label{font-size:13px;font-weight:900;letter-spacing:.7px;margin-bottom:7px;}";
  html += ".status-value{font-size:21px;font-weight:900;}";
  html += ".status-note{font-size:12px;margin-top:4px;opacity:.78;}";

  html += ".status-online{background:linear-gradient(135deg,#dcfce7,#c9f4d2);color:#166534;}";
  html += ".status-offline{background:linear-gradient(135deg,#ffe4e4,#ffcaca);color:#991b1b;}";
  html += ".status-check{background:linear-gradient(135deg,#eef2ff,#e0e7ff);color:#3730a3;}";

  html += ".control-head{display:flex;justify-content:space-between;align-items:center;gap:12px;flex-wrap:wrap;}";
  html += ".value-badge{display:inline-flex;align-items:center;gap:6px;padding:8px 12px;border-radius:999px;background:#f1f5f9;font-weight:900;font-size:13px;}";

  html += ".mode-toggle{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;padding:6px;background:#e9eef5;border-radius:18px;}";
  html += ".mode-btn{width:100%;margin:0;border:none!important;background:transparent!important;color:#526074!important;box-shadow:none!important;min-height:52px;}";
  html += ".mode-btn.active{background:linear-gradient(135deg,#1d4ed8,#2563eb)!important;color:#fff!important;box-shadow:0 8px 18px rgba(37,99,235,.26)!important;}";
  html += ".mode-btn#sheetBtn.active{background:linear-gradient(135deg,#7c3aed,#a855f7)!important;box-shadow:0 8px 18px rgba(124,58,237,.24)!important;}";
  html += ".mode-btn#weatherBtn.active{background:linear-gradient(135deg,#06b6d4,#0ea5e9)!important;box-shadow:0 8px 18px rgba(14,165,233,.28)!important;}";
  html += ".mode-btn:active,.speed-btn:active{transform:scale(.97);}";

  html += ".led-panel{display:grid;grid-template-columns:120px 1fr;gap:20px;align-items:center;}";
  html += ".led-gauge{";
  html += "width:112px;height:112px;border-radius:50%;";
  html += "display:grid;place-items:center;";
  html += "background:conic-gradient(#3b82f6 0deg, #3b82f6 0deg, #dbeafe 0deg);";
  html += "box-shadow:inset 0 0 0 10px rgba(255,255,255,.65),0 10px 24px rgba(59,130,246,.18);";
  html += "}";
  html += ".led-gauge-inner{width:78px;height:78px;border-radius:50%;background:rgba(255,255,255,.96);display:grid;place-items:center;font-weight:900;font-size:20px;color:#1e3a8a;}";
  html += ".led-caption{font-size:12px;color:var(--muted);margin-top:2px;}";
  html += ".led-live{display:flex;align-items:center;justify-content:center;gap:8px;margin-top:8px;font-size:12px;font-weight:800;color:#475569;}";
  html += ".led-dot{width:12px;height:12px;border-radius:50%;background:#bfdbfe;box-shadow:0 0 0 0 rgba(59,130,246,0);transition:.2s;}";
  html += ".led-dot.on{box-shadow:0 0 0 7px rgba(59,130,246,.13),0 0 18px rgba(59,130,246,.65);background:#3b82f6;}";
  html += ".led-panel{background:linear-gradient(135deg,rgba(239,246,255,.75),rgba(248,250,252,.95));border:1px solid rgba(148,163,184,.15);border-radius:22px;padding:18px;}";
  html += ".quick-btn{background:linear-gradient(135deg,#0f172a,#334155);}";
  html += ".quick-btn:active{transform:scale(.98);}";
  html += ".heartbeat-layout{background:linear-gradient(135deg,rgba(255,241,242,.85),rgba(255,247,237,.95));border:1px solid rgba(244,63,94,.10);border-radius:22px;padding:18px;}";
  html += ".heart-orbit{width:138px;height:138px;margin:0 auto;display:grid;place-items:center;border-radius:50%;background:radial-gradient(circle at center,#fff 0 34%,rgba(255,255,255,.55) 35% 49%,rgba(244,63,94,.08) 50% 72%,rgba(244,63,94,.02) 73%);box-shadow:inset 0 0 24px rgba(244,63,94,.10),0 10px 24px rgba(244,63,94,.10);}";
  html += ".heartbeat-meta{display:flex;justify-content:center;gap:8px;flex-wrap:wrap;margin-top:8px;}";
  html += ".heartbeat-live{display:inline-flex;align-items:center;gap:6px;padding:7px 11px;border-radius:999px;background:#fff1f2;color:#be123c;font-size:12px;font-weight:900;}";
  html += ".beat-dot{width:8px;height:8px;border-radius:50%;background:#fb7185;animation:beatDot 1.1s ease-in-out infinite;}";
  html += "@keyframes beatDot{0%,100%{transform:scale(.8);opacity:.55}50%{transform:scale(1.2);opacity:1}}";

  html += ".range-wrap{padding-top:4px;}";
  html += "#brightnessSlider{width:100%;height:30px;accent-color:#3b82f6;}";
  html += ".range-scale{display:flex;justify-content:space-between;font-size:11px;color:var(--muted);margin-top:-4px;}";

  html += ".quick-row{display:flex;justify-content:center;gap:8px;flex-wrap:wrap;margin-top:12px;}";
  html += ".quick-btn{min-width:90px;}";

  html += ".heartbeat-layout{display:grid;grid-template-columns:150px 1fr;gap:22px;align-items:center;}";
  html += ".heart-wrap{text-align:center;}";
  html += ".heart{font-size:78px;line-height:1;display:inline-block;animation:heartbeatPulse 1.55s ease-in-out infinite;filter:drop-shadow(0 8px 10px rgba(239,68,68,.18));}";
  html += "@keyframes heartbeatPulse{0%,100%{transform:scale(1)}15%{transform:scale(1.16)}30%{transform:scale(1)}45%{transform:scale(1.10)}60%{transform:scale(1)} }";
  html += ".speed-chip{display:inline-flex;padding:7px 12px;border-radius:999px;background:#fff1f2;color:#be123c;font-weight:900;font-size:12px;}";
  html += ".speed-row{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;}";
  html += ".speed-btn{margin:0;min-width:0;width:100%;background:#fff;color:#475569;border:1px solid #dbe3ee!important;box-shadow:none!important;}";
  html += ".speed-btn.active{background:linear-gradient(135deg,#ef4444,#f97316);color:#fff;border-color:transparent!important;box-shadow:0 8px 18px rgba(239,68,68,.24)!important;}";

  html += ".detail-grid{display:grid;grid-template-columns:repeat(2,1fr);gap:10px;}";
  html += ".detail-item{padding:14px;border-radius:16px;background:rgba(248,250,252,.88);}";
  html += ".detail-label{font-size:11px;text-transform:uppercase;letter-spacing:.6px;color:var(--muted);}";
  html += ".detail-value{font-size:16px;font-weight:900;margin-top:5px;word-break:break-word;}";

  html += ".pwm-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:10px;}";
  html += ".pwm-card{padding:14px;border-radius:16px;background:linear-gradient(135deg,#eef2ff,#f8fafc);text-align:center;}";
  html += ".pwm-num{font-size:22px;font-weight:900;color:#4338ca;}";
  html += ".pwm-label{font-size:11px;color:var(--muted);margin-top:4px;text-transform:uppercase;}";

  html += "button{";
  html += "font-size:16px;padding:13px 16px;";
  html += "border:none;border-radius:14px;";
  html += "background:#0f172a;color:#fff;";
  html += "min-height:48px;cursor:pointer;";
  html += "transition:transform .15s ease, box-shadow .15s ease, background .15s ease;";
  html += "}";
  html += "button:hover{transform:translateY(-1px);box-shadow:0 8px 18px rgba(15,23,42,.14);}";
  html += ".muted{color:var(--muted);}";

  html += "@media(max-width:760px){";
  html += ".status-grid{grid-template-columns:1fr;}";
  html += ".led-panel{grid-template-columns:1fr;}";
  html += ".led-gauge{margin:0 auto;}";
  html += ".heartbeat-layout{grid-template-columns:1fr;}";
  html += ".heart-orbit{width:122px;height:122px;}";
  html += ".speed-row{grid-template-columns:1fr 1fr;}";
  html += ".detail-grid{grid-template-columns:1fr;}";
  html += ".pwm-grid{grid-template-columns:1fr 1fr 1fr;}";
  html += ".hero-temp{font-size:50px;}";
  html += ".hero-icon{font-size:62px;}";
  html += "}";

  html += "@media(max-width:480px){";
  html += "body{padding:8px;}";
  html += ".section{padding:16px;}";
  html += ".topbar{align-items:flex-start;}";
  html += ".brand{font-size:16px;}";
  html += ".header-status{font-size:11px;padding:7px 9px;}";
  html += ".hero{padding:24px 14px 20px;border-radius:26px;}";
  html += ".hero-condition{font-size:18px;}";
  html += ".hero-temp{font-size:44px;}";
  html += ".metric-pill{font-size:12px;padding:8px 10px;}";
  html += ".speed-row{grid-template-columns:1fr;}";
  html += ".pwm-grid{grid-template-columns:1fr;}";
  html += "}";

  html += "</style>";
  html += "</head>";

  html += "<body>";

  html += "<div class='shell'>";

  // ===================================================
  // TOP BAR
  // ===================================================

  html += "<div class='topbar'>";
  html += "<div>";
  html += "<div class='brand'>🌐 ESP8266 SMART HUB</div>";
  html += "<div class='brand-sub'>IoT Control & Monitoring Panel</div>";
  html += "</div>";
  html += "<div id='systemHeaderStatus' class='header-status'>● SYSTEM ONLINE</div>";
  html += "</div>";

  // ===================================================
  // WEATHER HERO
  // ===================================================

  html += "<div id='weatherHero' class='hero'>";

  html += "<div class='hero-glow'></div>";

  html += "<div id='weatherIcon' class='hero-icon'>🌤️</div>";

  html += "<div id='weatherCondition' class='hero-condition'>";
  html += weatherAvailable ? weatherCondition : "WAITING FOR WEATHER";
  html += "</div>";

  html += "<div id='weatherTemp' class='hero-temp'>";
  html += weatherAvailable ? String(weatherTemperature,1) + " °C" : "-- °C";
  html += "</div>";

  html += "<div class='hero-place'>Mersin • Live weather</div>";

  html += "<div class='hero-metrics'>";

  html += "<div class='metric-pill'>💧 <span id='weatherHumidityHero'>";
  html += weatherAvailable ? String(weatherHumidity) + "%" : "--";
  html += "</span></div>";

  html += "<div class='metric-pill'>💨 <span id='weatherWindHero'>";
  html += weatherAvailable ? String(weatherWindSpeed,1) + " km/h" : "--";
  html += "</span></div>";

  html += "<div class='metric-pill'>🕒 <span id='weatherObservationHero'>";
  html += weatherAvailable ? weatherObservationTime : "No data";
  html += "</span></div>";

  html += "</div>";
  html += "</div>";

  // ===================================================
  // STATUS SUMMARY
  // ===================================================

  html += "<div class='status-grid'>";

  html += "<div class='status-card status-online'>";
  html += "<div class='status-icon'>📶</div>";
  html += "<div class='status-label'>LOCAL WIFI</div>";
  html += "<div class='status-value'>CONNECTED</div>";
  html += "<div class='status-note'>Clients: <span id='wifiCardClients'>";
  html += connectedDevices;
  html += "</span></div>";
  html += "</div>";

  html += "<div id='routerCard' class='status-card ";
  html += (WiFi.status() == WL_CONNECTED) ? "status-online" : "status-offline";
  html += "'>";
  html += "<div class='status-icon'>🌐</div>";
  html += "<div class='status-label'>ROUTER</div>";
  html += "<div id='routerCardStatus' class='status-value'>";
  html += (WiFi.status() == WL_CONNECTED) ? "ONLINE" : "OFFLINE";
  html += "</div>";
  html += "<div class='status-note'>RSSI: <span id='routerCardRSSI'>--</span> dBm</div>";
  html += "</div>";

  html += "<div id='weatherCard' class='status-card ";
  if (!weatherAvailable) html += "status-offline";
  else if (weatherStale) html += "status-check";
  else html += "status-online";
  html += "'>";
  html += "<div class='status-icon'>🌤️</div>";
  html += "<div class='status-label'>WEATHER API</div>";
  html += "<div id='weatherCardStatus' class='status-value'>";
  if (!weatherAvailable) html += "OFFLINE";
  else if (weatherStale) html += "UPDATING";
  else html += "ONLINE";
  html += "</div>";
  html += "<div class='status-note'><span id='weatherCardTemp'>--</span> °C</div>";
  html += "</div>";

  html += "</div>";

  // ===================================================
  // CONTROL MODE
  // ===================================================

  html += "<div class='section'>";
  html += "<div class='control-head'>";
  html += "<div>";
  html += "<div class='section-title'>🎛️ Control Mode</div>";
  html += "<div class='section-sub'>Choose how the LED brightness is controlled.</div>";
  html += "</div>";
  html += "<div class='value-badge'>Mode: <span id='dashboardMode'>" + controlMode + "</span></div>";
  html += "</div>";

  html += "<div class='mode-toggle'>";

  html += "<button id='manualBtn' class='mode-btn";
  if (controlMode == "MANUAL") html += " active";
  html += "' onclick='setMode(\"MANUAL\")'>MANUAL</button>";

  html += "<button id='weatherBtn' class='mode-btn";
  if (controlMode == "WEATHER") html += " active";
  html += "' onclick='setMode(\"WEATHER\")'>🌤 WEATHER</button>";

  html += "<button id='sheetBtn' class='mode-btn";
  if (controlMode == "SHEET") html += " active";
  html += "' onclick='setMode(\"SHEET\")'>📊 GOOGLE SHEET</button>";

  html += "</div>";

  html += "<div style='margin-top:12px;text-align:center;'>";
  html += "<div class='value-badge'>Google Sheet: <span id='sheetRemoteStatus'>";
  if (!googleSheetConfigured()) {
    html += "NOT CONFIGURED";
  } else if (sheetAvailable) {
    html += String(sheetBrightness);
    html += "%";
  } else {
    html += "WAITING";
  }
  html += "</span></div>";
  html += "</div>";

  html += "</div>";
  html += "</div>";

  // ===================================================
  // LED CONTROL
  // ===================================================

  html += "<div class='section'>";

  html += "<div class='section-title'>💡 LED Control</div>";
  html += "<div class='section-sub'>Fine control of PWM brightness in manual mode.</div>";

  html += "<div class='led-panel'>";

  html += "<div>";
  html += "<div id='ledGauge' class='led-gauge'>";
  html += "<div class='led-gauge-inner'><span id='ledGaugeValue'>";
  html += maximumBrightness;
  html += "%</span></div>";
  html += "</div>";
  html += "<div class='led-caption'>Current output</div>";
  html += "<div class='led-live'><span id='ledDot' class='led-dot'></span><span id='ledLiveText'>LIVE OUTPUT</span></div>";
  html += "</div>";

  html += "<div class='range-wrap'>";

  html += "<div class='value-badge'>Selected: <span id='brightnessValue'>";
  html += selectedBrightness;
  html += "%</span></div>";

  html += "<input id='brightnessSlider' type='range' min='0' max='100' value='";
  html += selectedBrightness;
  html += "' oninput='sliderChanged(this.value)'>";

  html += "<div class='range-scale'><span>0%</span><span>50%</span><span>100%</span></div>";

  html += "<div class='quick-row'>";
  html += "<button class='quick-btn' onclick='sendBrightness(20)'>20%</button>";
  html += "<button class='quick-btn' onclick='sendBrightness(50)'>50%</button>";
  html += "<button class='quick-btn' onclick='sendBrightness(90)'>90%</button>";
  html += "</div>";

  html += "</div>";
  html += "</div>";
  html += "</div>";

  // ===================================================
  // HEARTBEAT
  // ===================================================

  html += "<div class='section'>";

  html += "<div class='section-title'>❤️ Heartbeat</div>";
  html += "<div class='section-sub'>The built-in LED follows a heartbeat-style PWM animation.</div>";

  html += "<div class='heartbeat-layout'>";

  html += "<div class='heart-wrap'>";
  html += "<div class='heart-orbit'>";
  html += "<div id='heartbeatHeart' class='heart'>❤️</div>";
  html += "</div>";
  html += "<div class='heartbeat-meta'>";
  html += "<div id='heartbeatChip' class='speed-chip'>" + heartbeatSpeed + " SPEED</div>";
  html += "<div class='heartbeat-live'><span class='beat-dot'></span> LIVE</div>";
  html += "</div>";
  html += "</div>";

  html += "<div>";
  html += "<div class='value-badge'>Current speed: <span id='dashboardHeartbeat'>";
  html += heartbeatSpeed;
  html += "</span></div>";

  html += "<div class='speed-row' style='margin-top:12px;'>";

  html += "<button id='slowBtn' class='speed-btn";
  if (heartbeatSpeed == "SLOW") html += " active";
  html += "' onclick='setSpeed(\"SLOW\")'>SLOW</button>";

  html += "<button id='normalBtn' class='speed-btn";
  if (heartbeatSpeed == "NORMAL") html += " active";
  html += "' onclick='setSpeed(\"NORMAL\")'>NORMAL</button>";

  html += "<button id='fastBtn' class='speed-btn";
  if (heartbeatSpeed == "FAST") html += " active";
  html += "' onclick='setSpeed(\"FAST\")'>FAST</button>";

  html += "</div>";
  html += "</div>";

  html += "</div>";
  html += "</div>";

  // ===================================================
  // NETWORK DETAILS
  // ===================================================

  html += "<div class='section'>";
  html += "<div class='section-title'>📡 Network Details</div>";
  html += "<div class='detail-grid'>";

  html += "<div class='detail-item'><div class='detail-label'>AP SSID</div><div class='detail-value'>" + String(AP_SSID) + "</div></div>";
  html += "<div class='detail-item'><div class='detail-label'>AP IP</div><div class='detail-value'><span id='apIP'>" + WiFi.softAPIP().toString() + "</span></div></div>";
  html += "<div class='detail-item'><div class='detail-label'>Router</div><div class='detail-value'>" + String(ROUTER_SSID) + "</div></div>";
  html += "<div class='detail-item'><div class='detail-label'>STA IP</div><div class='detail-value'><span id='staIP'>" + ((WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "NONE") + "</span></div></div>";
  html += "<div class='detail-item'><div class='detail-label'>Gateway</div><div class='detail-value'><span id='gatewayIP'>" + ((WiFi.status() == WL_CONNECTED) ? WiFi.gatewayIP().toString() : "NONE") + "</span></div></div>";
  html += "<div class='detail-item'><div class='detail-label'>RSSI</div><div class='detail-value'><span id='networkRSSI'>";
  html += (WiFi.status() == WL_CONNECTED) ? String(WiFi.RSSI()) : "--";
  html += "</span> dBm</div></div>";
  html += "<div class='detail-item'><div class='detail-label'>Wi-Fi Channel</div><div class='detail-value'><span id='networkChannel'>" + String((WiFi.status() == WL_CONNECTED) ? WiFi.channel() : 0) + "</span></div></div>";
  html += "<div class='detail-item'><div class='detail-label'>Router MAC</div><div class='detail-value'><span id='routerMac'>" + ((WiFi.status() == WL_CONNECTED) ? WiFi.BSSIDstr() : "NONE") + "</span></div></div>";
  html += "<div class='detail-item'><div class='detail-label'>AP Clients</div><div class='detail-value'><span id='networkClients'>" + String(connectedDevices) + "</span></div></div>";

  html += "</div>";
  html += "</div>";

  // ===================================================
  // WEATHER DETAILS
  // ===================================================

  html += "<div class='section'>";
  html += "<div class='section-title'>🌦️ Weather Details</div>";
  html += "<div class='detail-grid'>";

  html += "<div class='detail-item'><div class='detail-label'>Temperature</div><div class='detail-value'><span id='detailTemperature'>";
  html += weatherAvailable ? String(weatherTemperature,1) : "--";
  html += "</span> °C</div></div>";

  html += "<div class='detail-item'><div class='detail-label'>Humidity</div><div class='detail-value'><span id='detailHumidity'>";
  html += weatherAvailable ? String(weatherHumidity) : "--";
  html += "</span> %</div></div>";

  html += "<div class='detail-item'><div class='detail-label'>Wind</div><div class='detail-value'><span id='detailWindSpeed'>";
  html += weatherAvailable ? String(weatherWindSpeed,1) : "--";
  html += "</span> km/h</div></div>";

  html += "<div class='detail-item'><div class='detail-label'>Condition</div><div class='detail-value' id='detailCondition'>";
  html += weatherAvailable ? weatherCondition : "No data";
  html += "</div></div>";

  html += "<div class='detail-item'><div class='detail-label'>Weather Code</div><div class='detail-value' id='detailWeatherCode'>";
  html += weatherAvailable ? String(weatherCode) : "--";
  html += "</div></div>";

  html += "<div class='detail-item'><div class='detail-label'>Last Observation</div><div class='detail-value' id='detailObservationTime'>";
  html += weatherAvailable ? weatherObservationTime : "No data";
  html += "</div></div>";

  html += "<div class='detail-item'><div class='detail-label'>API Status</div><div class='detail-value' id='weatherApiStatus'>" + weatherStatus + "</div></div>";
  html += "<div class='detail-item'><div class='detail-label'>API HTTP Code</div><div class='detail-value' id='weatherHttpCode'>" + String(weatherHttpCode) + "</div></div>";

  html += "</div>";
  html += "</div>";

  html += "<div class='section'>";
  html += "<div class='section-title'>📊 Google Sheet Details</div>";
  html += "<div class='detail-grid'>";
  html += "<div class='detail-item'><div class='detail-label'>Remote PWM Value</div><div class='detail-value'><span id='sheetDetailBrightness'>" + String(sheetBrightness) + "</span>%</div></div>";
  html += "<div class='detail-item'><div class='detail-label'>Connection Status</div><div class='detail-value' id='sheetDetailStatus'>" + sheetStatus + "</div></div>";
  html += "<div class='detail-item'><div class='detail-label'>HTTP Code</div><div class='detail-value' id='sheetHttpCode'>" + String(sheetHttpCode) + "</div></div>";
  html += "<div class='detail-item'><div class='detail-label'>Polling Interval</div><div class='detail-value'>10 seconds</div></div>";
  html += "</div>";
  html += "</div>";

  // ===================================================
  // PWM
  // ===================================================

  html += "<div class='section'>";
  html += "<div class='section-title'>⚡ PWM Output</div>";
  html += "<div class='section-sub'>Electrical drive information for the built-in LED.</div>";

  html += "<div class='pwm-grid'>";

  html += "<div class='pwm-card'><div class='pwm-num'>" + String(PWM_FREQUENCY) + "</div><div class='pwm-label'>Hz</div></div>";
  html += "<div class='pwm-card'><div class='pwm-num'>10-bit</div><div class='pwm-label'>Resolution</div></div>";
  html += "<div class='pwm-card'><div id='pwmGaugeValue' class='pwm-num'>" + String(maximumBrightness) + "%</div><div class='pwm-label'>Brightness</div></div>";

  html += "</div>";
  html += "</div>";

  // ===================================================
  // JAVASCRIPT
  // ===================================================

  html += "<script>";

  html += "let brightnessTimer=null;";

  html += "function sliderChanged(value){";
  html += "document.getElementById('brightnessValue').innerText=value+'%';";
  html += "updateLedGauge(Number(value));";
  html += "clearTimeout(brightnessTimer);";
  html += "brightnessTimer=setTimeout(function(){sendBrightness(value);},50);";
  html += "}";

  html += "function sendBrightness(value){";
  html += "fetch('/set?brightness='+value).then(()=>updateSystemStatus()).catch(e=>console.log(e));";
  html += "}";

  html += "function updateLedGauge(value){";
  html += "const safe=Math.max(0,Math.min(100,value));";
  html += "const angle=safe*3.6;";
  html += "const color=safe<30?'#60a5fa':(safe<70?'#3b82f6':'#2563eb');";
  html += "document.getElementById('ledGauge').style.background='conic-gradient('+color+' 0deg,'+color+' '+angle+'deg,#dbeafe '+angle+'deg,#dbeafe 360deg)';";
  html += "document.getElementById('ledGaugeValue').innerText=safe+'%';";
  html += "const dot=document.getElementById('ledDot');";
  html += "dot.classList.toggle('on',safe>0);";
  html += "document.getElementById('ledLiveText').innerText=safe>0?'LIVE OUTPUT':'LED OFF';";
  html += "}";

  html += "function applyMode(mode){";
  html += "document.getElementById('dashboardMode').innerText=mode;";
  html += "document.getElementById('manualBtn').classList.toggle('active',mode==='MANUAL');";
  html += "document.getElementById('weatherBtn').classList.toggle('active',mode==='WEATHER');";
  html += "document.getElementById('sheetBtn').classList.toggle('active',mode==='SHEET');";
  html += "}";

  html += "function applyHeartbeat(speed){";
  html += "document.getElementById('dashboardHeartbeat').innerText=speed;";
  html += "document.getElementById('heartbeatChip').innerText=speed+' SPEED';";
  html += "['SLOW','NORMAL','FAST'].forEach(function(s){";
  html += "const id=s==='SLOW'?'slowBtn':(s==='NORMAL'?'normalBtn':'fastBtn');";
  html += "document.getElementById(id).classList.toggle('active',s===speed);";
  html += "});";
  html += "const durations={SLOW:'2.2s',NORMAL:'1.55s',FAST:'0.8s'};";
  html += "document.getElementById('heartbeatHeart').style.animationDuration=durations[speed]||'1.55s';";
  html += "}";

  html += "function applyWeatherTheme(code,available){";
  html += "document.body.className='';";
  html += "const hero=document.getElementById('weatherHero');";
  html += "const icon=document.getElementById('weatherIcon');";
  html += "hero.className='hero';";
  html += "if(!available){icon.innerText='🌤️';return;}";
  html += "code=Number(code);";
  html += "if(code===0){icon.innerText='☀️';document.body.classList.add('theme-clear');}";
  html += "else if(code===1){icon.innerText='🌤️';document.body.classList.add('theme-mainly-clear');}";
  html += "else if(code===2){icon.innerText='⛅';document.body.classList.add('theme-cloudy');}";
  html += "else if(code===3){icon.innerText='☁️';document.body.classList.add('theme-overcast');}";
  html += "else if(code===45||code===48){icon.innerText='🌫️';document.body.classList.add('theme-cloudy');}";
  html += "else if(code>=51&&code<=55){icon.innerText='🌦️';document.body.classList.add('theme-rain');}";
  html += "else if(code>=61&&code<=65){icon.innerText='🌧️';document.body.classList.add('theme-rain');}";
  html += "else if(code>=71&&code<=75){icon.innerText='❄️';document.body.classList.add('theme-mainly-clear');}";
  html += "else if(code>=80&&code<=82){icon.innerText='🌦️';document.body.classList.add('theme-rain');}";
  html += "else if(code>=95){icon.innerText='⛈️';document.body.classList.add('theme-storm');}";
  html += "else{icon.innerText='🌤️';}";
  html += "}";

  html += "let statusRequestRunning=false;";

  html += "function setMode(mode){";
  html += "applyMode(mode);";
  html += "fetch('/mode?value='+encodeURIComponent(mode),{cache:'no-store'}).then(r=>{if(!r.ok)throw new Error('Mode request failed');return updateSystemStatus();}).catch(e=>console.log(e));";
  html += "}";

  html += "function setSpeed(speed){";
  html += "applyHeartbeat(speed);";
  html += "fetch('/speed?value='+encodeURIComponent(speed),{cache:'no-store'}).then(r=>{if(!r.ok)throw new Error('Speed request failed');return updateSystemStatus();}).catch(e=>console.log(e));";
  html += "}";

  html += "function sliderChanged(value){";
  html += "document.getElementById('brightnessValue').innerText=value+'%';";
  html += "updateLedGauge(Number(value));";
  html += "clearTimeout(brightnessTimer);";
  html += "brightnessTimer=setTimeout(function(){sendBrightness(value);},50);";
  html += "}";

  html += "function sendBrightness(value){";
  html += "document.getElementById('brightnessSlider').value=value;";
  html += "document.getElementById('brightnessValue').innerText=value+'%';";
  html += "updateLedGauge(Number(value));";
  html += "fetch('/set?brightness='+value,{cache:'no-store'}).then(()=>updateSystemStatus()).catch(e=>console.log(e));";
  html += "}";

  html += "function updateSystemStatus(){";
  html += "if(statusRequestRunning)return;";
  html += "statusRequestRunning=true;";
  html += "fetch('/status?t='+Date.now(),{cache:'no-store'})";
  html += ".then(r=>{if(!r.ok)throw new Error('HTTP '+r.status);return r.json();})";
  html += ".then(data=>{";

  html += "document.getElementById('systemHeaderStatus').innerText=data.routerConnected?'● SYSTEM ONLINE':'● LOCAL MODE';";
  html += "document.getElementById('systemHeaderStatus').className='header-status '+(data.routerConnected?'':'local');";
  html += "document.getElementById('wifiCardClients').innerText=data.apClients;";

  html += "const rc=document.getElementById('routerCard');";
  html += "document.getElementById('routerCardStatus').innerText=data.routerConnected?'ONLINE':'OFFLINE';";
  html += "document.getElementById('routerCardRSSI').innerText=data.routerConnected?data.rssi:'--';";
  html += "rc.classList.remove('status-online','status-offline','status-check');";
  html += "rc.classList.add(data.routerConnected?'status-online':'status-offline');";
  html += "document.getElementById('staIP').innerText=data.staIP;";
  html += "document.getElementById('gatewayIP').innerText=data.gateway;";
  html += "document.getElementById('networkRSSI').innerText=data.routerConnected?data.rssi:'--';";
  html += "document.getElementById('networkChannel').innerText=data.routerConnected?data.channel:'--';";
  html += "document.getElementById('routerMac').innerText=data.routerMac;";
  html += "document.getElementById('networkClients').innerText=data.apClients;";

  html += "const wc=document.getElementById('weatherCard');";
  html += "let weatherState='OFFLINE';";
  html += "if(data.weatherAvailable&&data.weatherStale)weatherState='UPDATING';";
  html += "else if(data.weatherAvailable)weatherState='ONLINE';";
  html += "document.getElementById('weatherCardStatus').innerText=weatherState;";
  html += "document.getElementById('weatherCardTemp').innerText=data.weatherAvailable?Number(data.temperature).toFixed(1):'--';";
  html += "wc.classList.remove('status-online','status-offline','status-check');";
  html += "wc.classList.add(!data.weatherAvailable?'status-offline':(data.weatherStale?'status-check':'status-online'));";

  html += "if(data.weatherAvailable){";
  html += "document.getElementById('detailTemperature').innerText=Number(data.temperature).toFixed(1);";
  html += "document.getElementById('detailHumidity').innerText=data.humidity;";
  html += "document.getElementById('detailWindSpeed').innerText=Number(data.windSpeed).toFixed(1);";
  html += "document.getElementById('detailCondition').innerText=data.condition;";
  html += "document.getElementById('detailWeatherCode').innerText=data.weatherCode;";
  html += "document.getElementById('detailObservationTime').innerText=data.observationTime;";
  html += "document.getElementById('weatherCondition').innerText=data.condition;";
  html += "document.getElementById('weatherTemp').innerText=Number(data.temperature).toFixed(1)+' °C';";
  html += "document.getElementById('weatherHumidityHero').innerText=data.humidity+'%';";
  html += "document.getElementById('weatherWindHero').innerText=Number(data.windSpeed).toFixed(1)+' km/h';";
  html += "document.getElementById('weatherObservationHero').innerText=data.observationTime;";
  html += "applyWeatherTheme(data.weatherCode,true);";
  html += "}";

  html += "document.getElementById('weatherApiStatus').innerText=data.weatherStatus;";
  html += "document.getElementById('weatherHttpCode').innerText=data.weatherHttpCode||'--';";

  html += "document.getElementById('pwmGaugeValue').innerText=data.brightness+'%';";
  html += "document.getElementById('brightnessSlider').value=data.manualBrightness;";
  html += "document.getElementById('brightnessValue').innerText=data.manualBrightness+'%';";
  html += "updateLedGauge(Number(data.brightness));";
  html += "document.getElementById('dashboardMode').innerText=data.mode;";
  html += "applyMode(data.mode);";
  html += "applyHeartbeat(data.heartbeat);";
  html += "document.getElementById('sheetRemoteStatus').innerText=data.sheetAvailable?data.sheetBrightness+'%':(data.mode==='SHEET'?'WAITING':'READY');";
  html += "document.getElementById('sheetDetailBrightness').innerText=data.sheetBrightness;";
  html += "document.getElementById('sheetDetailStatus').innerText=data.sheetStatus;";
  html += "document.getElementById('sheetHttpCode').innerText=data.sheetHttpCode||'--';";

  html += "}).catch(error=>{console.log('Status update error:',error);}).finally(()=>{statusRequestRunning=false;});";
  html += "}";

  html += "updateSystemStatus();";
  html += "setInterval(updateSystemStatus,1500);";

  html += "</script>";

  html += "</div>";
  html += "</body>";
  html += "</html>";

  server.send(200, "text/html", html);
}

// =====================================================
// 23. BRIGHTNESS HANDLER
// =====================================================

void handleBrightness() {

  if (server.hasArg("brightness")) {

    int newBrightness =
      server.arg("brightness").toInt();

    // 0-100 arasında sınırlandır
    newBrightness =
      constrain(newBrightness, 0, 100);

    selectedBrightness =
      newBrightness;

    // Manuel değer seçildiği için
    // otomatik olarak MANUAL moda geçiyoruz.
    controlMode = "MANUAL";

    Serial.print(
      "Brightness changed to: "
    );

    Serial.print(
      selectedBrightness
    );

    Serial.println("%");
  }

  // JavaScript fetch() kullandığımız için
  // sayfayı yeniden yönlendirmeye gerek yok.
  server.send(
    200,
    "text/plain",
    "OK"
  );
}


// =====================================================
// 23. MODE HANDLER
// =====================================================

void handleMode() {

  if (
    server.hasArg("value")
  ) {

    String mode =
      server.arg(
        "value"
      );


    if (
      mode == "MANUAL" ||
      mode == "WEATHER" ||
      mode == "SHEET"
    ) {

      controlMode =
        mode;

      if (controlMode == "SHEET") {
        // Force the first Google Sheet read immediately after
        // entering SHEET mode.
        lastGoogleSheetRequest = 0;
      }

      if (controlMode == "WEATHER") {
        // Request fresh weather data as soon as the mode is selected.
        lastWeatherRequest = 0;
      }

      Serial.print(
        "Control Mode: "
      );

      Serial.println(
        controlMode
      );
    }
  }


  server.send(
    200,
    "text/plain",
    "OK"
  );
}


// =====================================================
// 24. SPEED HANDLER
// =====================================================

void handleSpeed() {

  if (
    server.hasArg("value")
  ) {

    String speed =
      server.arg(
        "value"
      );


    if (
      speed == "SLOW" ||
      speed == "NORMAL" ||
      speed == "FAST"
    ) {

      setHeartbeatSpeed(
        speed
      );
    }
  }


  server.send(
    200,
    "text/plain",
    "OK"
  );
}


// =====================================================
// 26. NON-BLOCKING HEARTBEAT
// =====================================================

enum HeartbeatPhase {
  HB_FIRST_UP,
  HB_FIRST_DOWN,
  HB_FIRST_PAUSE,
  HB_SECOND_UP,
  HB_SECOND_DOWN,
  HB_FINAL_PAUSE
};

HeartbeatPhase heartbeatPhase = HB_FIRST_UP;
int heartbeatBrightness = MIN_BRIGHTNESS;
unsigned long heartbeatLastStep = 0;
unsigned long heartbeatPhaseStarted = 0;
String lastHeartbeatMode = "";

void resetHeartbeatState() {
  heartbeatPhase = HB_FIRST_UP;
  heartbeatBrightness = MIN_BRIGHTNESS;
  heartbeatLastStep = millis();
  heartbeatPhaseStarted = millis();
}

void heartbeatTick() {

  if (lastHeartbeatMode != controlMode) {
    lastHeartbeatMode = controlMode;
    resetHeartbeatState();
  }

  // Google Sheet mode: direct PWM, no heartbeat animation.
  if (controlMode == "SHEET") {
    analogWrite(LED_PIN, brightnessToDuty(sheetBrightness));
    return;
  }

  unsigned long now = millis();
  int maxBrightness = getMaximumBrightness();

  if (maxBrightness < MIN_BRIGHTNESS) {
    analogWrite(LED_PIN, brightnessToDuty(0));
    return;
  }

  int stepDelay = FADE_DELAY;

  switch (heartbeatPhase) {

    case HB_FIRST_UP:
      if (now - heartbeatLastStep >= (unsigned long)stepDelay) {
        heartbeatLastStep = now;
        heartbeatBrightness += 2;
        if (heartbeatBrightness >= maxBrightness) {
          heartbeatBrightness = maxBrightness;
          heartbeatPhase = HB_FIRST_DOWN;
        }
        analogWrite(LED_PIN, brightnessToDuty(heartbeatBrightness));
      }
      break;

    case HB_FIRST_DOWN:
      if (now - heartbeatLastStep >= (unsigned long)stepDelay) {
        heartbeatLastStep = now;
        heartbeatBrightness -= 2;
        if (heartbeatBrightness <= MIN_BRIGHTNESS) {
          heartbeatBrightness = MIN_BRIGHTNESS;
          heartbeatPhase = HB_FIRST_PAUSE;
          heartbeatPhaseStarted = now;
        }
        analogWrite(LED_PIN, brightnessToDuty(heartbeatBrightness));
      }
      break;

    case HB_FIRST_PAUSE:
      if (now - heartbeatPhaseStarted >= (unsigned long)HEART_DELAY) {
        heartbeatPhase = HB_SECOND_UP;
        heartbeatLastStep = now;
      }
      break;

    case HB_SECOND_UP:
      if (now - heartbeatLastStep >= (unsigned long)stepDelay) {
        heartbeatLastStep = now;
        heartbeatBrightness += 2;
        if (heartbeatBrightness >= maxBrightness) {
          heartbeatBrightness = maxBrightness;
          heartbeatPhase = HB_SECOND_DOWN;
        }
        analogWrite(LED_PIN, brightnessToDuty(heartbeatBrightness));
      }
      break;

    case HB_SECOND_DOWN:
      if (now - heartbeatLastStep >= (unsigned long)stepDelay) {
        heartbeatLastStep = now;
        heartbeatBrightness -= 2;
        if (heartbeatBrightness <= MIN_BRIGHTNESS) {
          heartbeatBrightness = MIN_BRIGHTNESS;
          heartbeatPhase = HB_FINAL_PAUSE;
          heartbeatPhaseStarted = now;
        }
        analogWrite(LED_PIN, brightnessToDuty(heartbeatBrightness));
      }
      break;

    case HB_FINAL_PAUSE:
      if (now - heartbeatPhaseStarted >= (unsigned long)BEAT_PAUSE) {
        heartbeatPhase = HB_FIRST_UP;
        heartbeatLastStep = now;
      }
      break;
  }
}


// =====================================================
// 27. SETUP
// =====================================================

void setup() {

  delay(1000);


  Serial.begin(115200);


  Serial.println();

  Serial.println(
    "================================"
  );

  Serial.println(
    "ESP8266 WEATHER CONTROL SYSTEM"
  );

  Serial.println(
    "================================"
  );


  // ---------------------------------------------------
  // LED
  // ---------------------------------------------------

  pinMode(
    LED_PIN,
    OUTPUT
  );


  // ---------------------------------------------------
  // PWM
  // ---------------------------------------------------

  analogWriteFreq(
    PWM_FREQUENCY
  );

  analogWriteRange(
    PWM_RANGE
  );


  // ---------------------------------------------------
  // AP + STA
  // ---------------------------------------------------

  WiFi.mode(
    WIFI_AP_STA
  );

  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);


  // ---------------------------------------------------
  // ACCESS POINT
  // ---------------------------------------------------

  WiFi.softAP(
    AP_SSID,
    AP_PASSWORD
  );


  Serial.print(
    "AP IP: "
  );

  Serial.println(
    WiFi.softAPIP()
  );


  // ---------------------------------------------------
  // ROUTER
  // ---------------------------------------------------

  Serial.println();

  Serial.println(
    "Connecting to Router..."
  );


  WiFi.begin(
    ROUTER_SSID,
    ROUTER_PASSWORD
  );


  unsigned long startTime =
    millis();


  while (
    WiFi.status() != WL_CONNECTED &&
    millis() - startTime < 20000
  ) {

    delay(500);

    Serial.print(".");
  }


  Serial.println();


  if (
    WiFi.status() == WL_CONNECTED
  ) {

    Serial.println(
      "ROUTER CONNECTED ✅"
    );


    Serial.print(
      "STA IP: "
    );

    Serial.println(
      WiFi.localIP()
    );


    Serial.print(
      "RSSI: "
    );

    Serial.print(
      WiFi.RSSI()
    );

    Serial.println(
      " dBm"
    );

  }

  else {

    Serial.println(
      "ROUTER NOT CONNECTED ❌"
    );
  }


  // ---------------------------------------------------
  // WEB SERVER
  // ---------------------------------------------------

  server.on(
    "/",
    handleRoot
  );


  server.on(
    "/set",
    handleBrightness
  );


  server.on(
    "/mode",
    handleMode
  );


  server.on(
    "/speed",
    handleSpeed
  );

  server.on("/status", handleStatus);

  server.begin();


  Serial.println(
    "HTTP SERVER STARTED"
  );


  // ---------------------------------------------------
  // INITIAL REMOTE UPDATE SCHEDULE
  // ---------------------------------------------------

  lastWeatherRequest = millis() - WEATHER_INTERVAL;
  lastGoogleSheetRequest = millis() - GOOGLE_SHEET_INTERVAL;

  Serial.print("FREE HEAP: ");
  Serial.println(ESP.getFreeHeap());

  Serial.println(
    "SYSTEM READY"
  );
}


// =====================================================
// 28. LOOP
// =====================================================

void loop() {

  server.handleClient();

  checkRouterConnection();

  checkWeatherUpdate();

  checkGoogleSheetUpdate();

  heartbeatTick();
}
