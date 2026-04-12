#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FirebaseESP8266.h>
#include <EEPROM.h>
#include <time.h>

#define EEPROM_SIZE 128

FirebaseConfig config;
FirebaseAuth auth;
FirebaseData firebaseData;
FirebaseData firebaseHistoryData;
FirebaseData firebaseControlData;

ESP8266WebServer server(80);

const char* FIREBASE_HOST = "smart-parkovisko-default-rtdb.europe-west1.firebasedatabase.app";
const char* FIREBASE_LEGACY_TOKEN = "drhYKaWQRRKrp5FK2y65F2VRsPHVKc1qNH52qNSU";

const char* AP_PASSWORD = "Parkovisko2026";
const char* PORTAL_USER = "admin";
const char* PORTAL_PASS = "password";

String apSSID = "";

bool apMode = false;
bool firebaseReady = false;

String lastGoodSSID = "";
String lastGoodPASS = "";

String currentAttemptSSID = "";
String currentAttemptPASS = "";

bool localConfigConnectInProgress = false;
String localConfigStatus = "idle";
String localConfigMessage = "";
unsigned long localConfigStartTime = 0;

String lastLoggedSensorState[8];

unsigned long wifiConnectStart = 0;
unsigned long lastHeartbeat = 0;
unsigned long lastConfigCheck = 0;

const unsigned long WIFI_TIMEOUT = 15000;
const unsigned long HEARTBEAT_INTERVAL = 10000;
const unsigned long CONFIG_CHECK_INTERVAL = 5000;

const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

const int LASTGOOD_SSID_ADDR = 0;
const int LASTGOOD_PASS_ADDR = 32;

void startAP();
void connectToWiFi(const String& ssid, const String& pass);
void initFirebase();
void initTime();
void handleConfigPage();
void handleSave();
void handleStatus();
void processLocalConfigConnection();
void readArduino();
void updateHeartbeat();
void checkRemoteCommands();
void updateESPStatus(const String& state, const String& message = "", const String& targetSSID = "");
void logHistory(const String& type, const String& message, const String& requestedSSID = "", const String& resultSSID = "");
void logSensorHistory(const String& spot, const String& event, const String& state, int distance);
void clearRemoteAPModeRequest();
void saveLastGoodWiFi(const String& ssid, const String& pass);
void loadWiFi();
void clearLastGoodWiFi();
String getFormattedTime();
unsigned long getUnixTimestamp();
bool isAuthenticated();
bool isValidWiFiInput(const String& ssid, const String& pass);
bool isValidSpot(String spot);
int getSpotIndex(const String& spot);

void writeEEPROMString(int start, const String& value, int maxLen) {
  for (int i = 0; i < maxLen; i++) {
    EEPROM.write(start + i, i < value.length() ? value[i] : 0);
  }
}

String readEEPROMString(int start, int maxLen) {
  char buffer[65];
  if (maxLen > 64) maxLen = 64;

  for (int i = 0; i < maxLen; i++) {
    buffer[i] = EEPROM.read(start + i);
  }

  buffer[maxLen] = '\0';
  String result = String(buffer);
  result.trim();
  return result;
}

void saveLastGoodWiFi(const String& ssid, const String& pass) {
  writeEEPROMString(LASTGOOD_SSID_ADDR, ssid, 32);
  writeEEPROMString(LASTGOOD_PASS_ADDR, pass, 32);
  EEPROM.commit();

  lastGoodSSID = ssid;
  lastGoodPASS = pass;
}

void loadWiFi() {
  lastGoodSSID = readEEPROMString(LASTGOOD_SSID_ADDR, 32);
  lastGoodPASS = readEEPROMString(LASTGOOD_PASS_ADDR, 32);
}

void clearLastGoodWiFi() {
  writeEEPROMString(LASTGOOD_SSID_ADDR, "", 32);
  writeEEPROMString(LASTGOOD_PASS_ADDR, "", 32);
  EEPROM.commit();

  lastGoodSSID = "";
  lastGoodPASS = "";
}

bool isAuthenticated() {
  if (!server.authenticate(PORTAL_USER, PORTAL_PASS)) {
    server.requestAuthentication();
    return false;
  }
  return true;
}

bool isValidWiFiInput(const String& ssid, const String& pass) {
  if (ssid.length() == 0 || ssid.length() > 31) return false;
  if (pass.length() > 63) return false;
  return true;
}

bool isValidSpot(String spot) {
  return (
    spot == "spot1" || spot == "spot2" || spot == "spot3" || spot == "spot4" ||
    spot == "spot5" || spot == "spot6" || spot == "spot7" || spot == "spot8"
  );
}

int getSpotIndex(const String& spot) {
  if (spot == "spot1") return 0;
  if (spot == "spot2") return 1;
  if (spot == "spot3") return 2;
  if (spot == "spot4") return 3;
  if (spot == "spot5") return 4;
  if (spot == "spot6") return 5;
  if (spot == "spot7") return 6;
  if (spot == "spot8") return 7;
  return -1;
}

void initTime() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);

  time_t now = time(nullptr);
  int retries = 0;

  while (now < 100000 && retries < 20) {
    delay(300);
    now = time(nullptr);
    retries++;
  }
}

unsigned long getUnixTimestamp() {
  time_t now = time(nullptr);
  if (now < 100000) return 0;
  return (unsigned long)now;
}

String getFormattedTime() {
  time_t now = time(nullptr);
  if (now < 100000) return "";

  struct tm* timeinfo = localtime(&now);
  if (!timeinfo) return "";

  char buffer[25];
  snprintf(
    buffer,
    sizeof(buffer),
    "%04d-%02d-%02d %02d:%02d:%02d",
    timeinfo->tm_year + 1900,
    timeinfo->tm_mon + 1,
    timeinfo->tm_mday,
    timeinfo->tm_hour,
    timeinfo->tm_min,
    timeinfo->tm_sec
  );

  return String(buffer);
}

void initFirebase() {
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_LEGACY_TOKEN;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  firebaseReady = true;

  Firebase.setString(firebaseData, "/system/device_status", "online");
  Firebase.setString(firebaseData, "/system/wifi_status", "connected");
  Firebase.setString(firebaseData, "/system/ip", WiFi.localIP().toString());
}

void updateESPStatus(const String& state, const String& message, const String& targetSSID) {
  if (!firebaseReady || WiFi.status() != WL_CONNECTED) return;

  FirebaseJson json;
  json.set("state", state);
  json.set("message", message);
  json.set("connectedTo", WiFi.SSID());
  json.set("ip", WiFi.localIP().toString());
  json.set("lastSeen", getFormattedTime());
  json.set("timestamp", getUnixTimestamp());
  json.set("targetSSID", targetSSID);
  json.set("apMode", false);

  Firebase.setJSON(firebaseData, "/esp_config/status", json);
}

void logHistory(const String& type, const String& message, const String& requestedSSID, const String& resultSSID) {
  if (!firebaseReady || WiFi.status() != WL_CONNECTED) return;

  FirebaseJson json;
  json.set("type", type);
  json.set("message", message);
  json.set("requestedSSID", requestedSSID);
  json.set("resultSSID", resultSSID);
  json.set("deviceSSID", WiFi.SSID());
  json.set("ip", WiFi.localIP().toString());
  json.set("time", getFormattedTime());
  json.set("timestamp", getUnixTimestamp());

  Firebase.pushJSON(firebaseHistoryData, "/esp_config/history", json);
}

void logSensorHistory(const String& spot, const String& event, const String& state, int distance) {
  if (!firebaseReady || WiFi.status() != WL_CONNECTED) return;

  FirebaseJson json;
  json.set("spot", spot);
  json.set("event", event);
  json.set("state", state);
  json.set("distance", distance);
  json.set("timestamp", getUnixTimestamp());
  json.set("time", getFormattedTime());

  Firebase.pushJSON(firebaseHistoryData, "/sensor_history", json);
}

void clearRemoteAPModeRequest() {
  if (!firebaseReady || WiFi.status() != WL_CONNECTED) return;
  Firebase.setBool(firebaseData, "/esp_config/request_ap_mode", false);
}

void startAP() {
  WiFi.disconnect(true);
  delay(300);

  WiFi.mode(WIFI_AP);
  apSSID = "Smart-Parkovisko-Setup";
  WiFi.softAP(apSSID.c_str(), AP_PASSWORD);

  apMode = true;
  firebaseReady = false;

  localConfigConnectInProgress = false;
  if (localConfigStatus != "failed") {
    localConfigStatus = "idle";
    localConfigMessage = "";
  }

  server.stop();

  server.on("/", HTTP_GET, handleConfigPage);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/status", HTTP_GET, handleStatus);
  server.onNotFound(handleConfigPage);

  server.begin();
}

void connectToWiFi(const String& ssid, const String& pass) {
  server.stop();

  WiFi.disconnect(true);
  delay(300);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  currentAttemptSSID = ssid;
  currentAttemptPASS = pass;
  wifiConnectStart = millis();

  apMode = false;
  firebaseReady = false;
}

void handleStatus() {
  if (!isAuthenticated()) return;

  String json = "{";
  json += "\"status\":\"" + localConfigStatus + "\",";
  json += "\"message\":\"" + localConfigMessage + "\"";
  json += "}";

  server.send(200, "application/json; charset=UTF-8", json);
}

void handleConfigPage() {
  if (!isAuthenticated()) return;

  String banner = "";

  if (localConfigStatus == "failed") {
    banner =
      "<div style='background:#8b0000;color:#fff;padding:12px;margin:0 auto 20px auto;max-width:360px;border-radius:8px;'>"
      "Nepodarilo sa pripojit na WiFi. Skuste znova."
      "</div>";
  }

  String page;
  page.reserve(3800);

  page += "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  page += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  page += "<title>ESP WiFi Setup</title></head>";
  page += "<body style='font-family:Arial;background:#111;color:white;text-align:center;padding:30px 15px;'>";
  page += "<div style='max-width:420px;margin:0 auto;background:#1b1b1b;padding:24px;border-radius:12px;'>";
  page += "<h2>ESP WiFi Setup</h2>";
  page += banner;
  page += "<p>SSID hotspotu: <b>" + apSSID + "</b></p>";
  page += "<p>Otvor v prehliadaci: <b>http://192.168.4.1</b></p>";
  page += "<form action='/save' method='POST'>";
  page += "<input name='ssid' maxlength='31' placeholder='WiFi SSID' style='padding:10px;width:220px;border:none;border-radius:8px;'><br><br>";
  page += "<input name='pass' type='password' maxlength='63' placeholder='WiFi Password' style='padding:10px;width:220px;border:none;border-radius:8px;'><br><br>";
  page += "<input type='submit' value='Connect' style='padding:10px 20px;border:none;border-radius:8px;background:#2d89ef;color:white;'>";
  page += "</form>";
  page += "<div id='statusBox' style='margin-top:20px;font-size:14px;color:#ccc;min-height:20px;'></div>";
  page += "</div>";
  page += "<script>";
  page += "setInterval(async function(){";
  page += "try{";
  page += "const r=await fetch('/status',{cache:'no-store'});";
  page += "if(!r.ok)return;";
  page += "const d=await r.json();";
  page += "const box=document.getElementById('statusBox');";
  page += "if(d.status==='connecting') box.innerHTML='Pripajam sa na WiFi...';";
  page += "else if(d.status==='failed') box.innerHTML='Nepodarilo sa pripojit na WiFi. Skuste znova.';";
  page += "else box.innerHTML='';";
  page += "}catch(e){}";
  page += "},1000);";
  page += "</script>";
  page += "</body></html>";

  server.send(200, "text/html; charset=UTF-8", page);
}

void handleSave() {
  if (!isAuthenticated()) return;

  String ssid = server.arg("ssid");
  String pass = server.arg("pass");

  ssid.trim();
  pass.trim();

  if (!isValidWiFiInput(ssid, pass)) {
    localConfigStatus = "failed";
    localConfigMessage = "Neplatne WiFi udaje.";

    server.send(
      400,
      "text/html; charset=UTF-8",
      "<html><body style='font-family:Arial;background:#111;color:white;text-align:center;padding-top:40px;'>"
      "<h2>Neplatne WiFi udaje.</h2>"
      "<p>Skontroluj SSID a heslo.</p>"
      "<p><a href='/' style='color:#4da3ff;'>Spat</a></p>"
      "</body></html>"
    );
    return;
  }

  currentAttemptSSID = ssid;
  currentAttemptPASS = pass;

  localConfigConnectInProgress = true;
  localConfigStatus = "connecting";
  localConfigMessage = "Pripajam sa na WiFi...";
  localConfigStartTime = millis();

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  server.send(
    200,
    "text/html; charset=UTF-8",
    "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<title>Poziadavka odoslana</title></head>"
    "<body style='font-family:Arial;background:#111;color:white;text-align:center;padding:40px 20px;'>"
    "<div style='max-width:420px;margin:0 auto;background:#1b1b1b;padding:24px;border-radius:12px;'>"
    "<h2>Poziadavka odoslana</h2>"
    "<p>Ak sa ESP uspesne pripoji na WiFi, automaticky vas odpoji.</p>"
    "<p>Ak sa nepripoji, stranka sa automaticky obnovi.</p>"
    "<div id='status' style='margin-top:20px;color:#ccc;min-height:20px;'></div>"
    "</div>"
    "<script>"
    "setInterval(async function(){"
    "  try{"
    "    const r = await fetch('/status', { cache:'no-store' });"
    "    if(!r.ok) return;"
    "    const d = await r.json();"
    "    const box = document.getElementById('status');"
    "    if(d.status === 'failed'){"
    "      box.innerText = 'Nepodarilo sa pripojit na WiFi. Obnovujem stranku...';"
    "      setTimeout(function(){ window.location.href='/'; }, 1200);"
    "    } else {"
    "      box.innerText = 'Cakam na vysledok pripojenia...';"
    "    }"
    "  }catch(e){}"
    "}, 1000);"
    "</script>"
    "</body></html>"
  );
}

void processLocalConfigConnection() {
  if (!localConfigConnectInProgress) return;

  if (WiFi.status() == WL_CONNECTED) {
    saveLastGoodWiFi(currentAttemptSSID, currentAttemptPASS);

    localConfigConnectInProgress = false;
    localConfigStatus = "idle";
    localConfigMessage = "";

    delay(500);
    WiFi.softAPdisconnect(true);
    apMode = false;
    firebaseReady = false;
    return;
  }

  if (millis() - localConfigStartTime > WIFI_TIMEOUT) {
    WiFi.disconnect(true);
    delay(200);

    localConfigConnectInProgress = false;
    localConfigStatus = "failed";
    localConfigMessage = "Nepodarilo sa pripojit na WiFi. Skuste znova.";
  }
}

void readArduino() {
  if (!Serial.available()) return;

  String msg = Serial.readStringUntil('\n');
  msg.trim();

  if (msg.length() == 0 || msg.length() > 40) return;

  int firstSep = msg.indexOf(':');
  int secondSep = msg.indexOf(':', firstSep + 1);

  if (firstSep <= 0 || secondSep <= 0) return;

  String spot = msg.substring(0, firstSep);
  String state = msg.substring(firstSep + 1, secondSep);
  int distance = msg.substring(secondSep + 1).toInt();

  if (!isValidSpot(spot)) return;
  if (state != "Free" && state != "Occupied" && state != "Error") return;
  if (distance < -1 || distance > 400) return;

  unsigned long unixTime = getUnixTimestamp();
  String basePath = "/parking_spots/" + spot;

  FirebaseJson json;
  json.set("distance", distance);
  json.set("state", state);
  json.set("timestamp", unixTime);
  json.set("sensorError", state == "Error");

  if (Firebase.setJSON(firebaseData, basePath, json)) {
    int spotIndex = getSpotIndex(spot);

    if (spotIndex >= 0) {
      String previousState = lastLoggedSensorState[spotIndex];

      if (previousState != state) {
        if (state == "Error" && previousState != "Error") {
          logSensorHistory(spot, "sensor_error_started", state, distance);
        }

        if (previousState == "Error" && state != "Error") {
          logSensorHistory(spot, "sensor_error_resolved", state, distance);
        }

        lastLoggedSensorState[spotIndex] = state;
      }
    }

    Firebase.setString(firebaseData, "/system/last_update", spot);
    Firebase.setString(firebaseData, "/system/last_state", state);
  } else {
    Firebase.setString(firebaseData, "/system/firebase_error", firebaseData.errorReason());
  }
}

void updateHeartbeat() {
  if (millis() - lastHeartbeat < HEARTBEAT_INTERVAL) return;
  lastHeartbeat = millis();

  updateESPStatus("connected", "Zariadenie je online.", "");
}

void checkRemoteCommands() {
  if (!firebaseReady || WiFi.status() != WL_CONNECTED) return;
  if (millis() - lastConfigCheck < CONFIG_CHECK_INTERVAL) return;

  lastConfigCheck = millis();

  bool requestAP = false;
  if (Firebase.getBool(firebaseControlData, "/esp_config/request_ap_mode")) {
    requestAP = firebaseControlData.boolData();
  }

  if (requestAP) {
    clearRemoteAPModeRequest();
    startAP();
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(50);

  for (int i = 0; i < 8; i++) {
    lastLoggedSensorState[i] = "Unknown";
  }

  EEPROM.begin(EEPROM_SIZE);
  loadWiFi();

  if (lastGoodSSID.length() > 0) {
    connectToWiFi(lastGoodSSID, lastGoodPASS);
  } else {
    startAP();
  }
}

void loop() {
  server.handleClient();
  processLocalConfigConnection();

  if (!apMode && !localConfigConnectInProgress && WiFi.status() != WL_CONNECTED) {
    if (millis() - wifiConnectStart > WIFI_TIMEOUT) {
      clearLastGoodWiFi();
      startAP();
    }
  }

  if (!firebaseReady && !apMode && !localConfigConnectInProgress && WiFi.status() == WL_CONNECTED) {
    initTime();
    initFirebase();
    updateESPStatus("connected", "Zariadenie je online.", "");
  }

  if (firebaseReady && WiFi.status() == WL_CONNECTED) {
    readArduino();
    checkRemoteCommands();
    updateHeartbeat();
  }
}