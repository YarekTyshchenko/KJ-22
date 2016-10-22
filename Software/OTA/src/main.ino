#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
// #include <WiFiManager.h>
#include <FS.h>

// WiFiManager wifiManager;
const char* ssid = "SKY8FE4D";
const char* password = "XXXXXXX";

void setup() {
  ESP.wdtDisable();
  Serial.begin(115200);
  Serial.println("Booting");
  //wifiManager.startConfigPortal();
  //wifiManager.autoConnect(ssid, password);
  //wifiManager.setConfigPortalTimeout(180);
  //wifiManager.autoConnect();
  //WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
    SPIFFS.end();
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\n", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.printf("Sketch size: %u\n", ESP.getSketchSize());
	Serial.printf("Free size: %u\n", ESP.getFreeSketchSpace());
  Serial.printf("Flash Chip ID: 0x%X\n", ESP.getFlashChipId());
  Serial.printf("Flash Chip size: %u\n", ESP.getFlashChipSize());
  Serial.printf("Flash Chip Real size: %u\n", ESP.getFlashChipRealSize());
  Serial.printf("Flash Chip mode: %u\n", ESP.getFlashChipMode());
  Serial.print("Reset Reason: ");
  Serial.println(ESP.getResetReason());

  Serial.println("Starting recovery loop");
  unsigned long time = millis();
  while (time + 10000 > millis()) {
    loop();
  }
  Serial.println("Recovery loop finished");
  otherSetup();
  Serial.println("Done with experimental Routine");
}




void restoreWireless() {
  Serial.println("Restoring wireless");
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  Serial.println("Connected");
}

void otherSetup() {
  bool result = SPIFFS.begin();
  Serial.println("SPIFFS opened: " + (result)?"true":"false");
  File f = openCredentialFile();
  String c = "";
  bool connected = false;
  while(true) {
    if (!getNextCredential(&c, f)) break;
    if (c) {
      Serial.println(c);
      if (tryWireless(c)) {
        connected = true;
        break;
      }
    }
  }
  Serial.println("Done with credential loop");
  if (!connected) restoreWireless();
}

File openCredentialFile() {
  File f = SPIFFS.open("/creds.txt", "r");
  if (!f) {
    Serial.println("file open failed");
  }
  return f;
}

bool getNextCredential(String* c, File f) {
  String cred = f.readStringUntil('\n');
  if (!cred.length()) return false;
  Serial.println("Got line: "+ cred);
  //parseCredential(c, cred);
  *c = cred;
  return true;
}

bool tryWireless(String cred) {
  Serial.println("Trying "+cred);
  int l = cred.indexOf(':');
  Serial.printf("Separator at position (%u)\n", l);
  if (l < 0) return false;
  String ssid = cred.substring(0, l+1);
  Serial.printf("SSID is (%u) chars long\n", ssid.length());
  String pass = cred.substring(l+1);
  Serial.printf("Pass is (%u) chars long\n", pass.length());

  char ssid_c[ssid.length()];
  char pass_c[pass.length()];
  ssid.toCharArray(ssid_c, ssid.length());
  pass.toCharArray(pass_c, pass.length());

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid_c, pass_c);
  ESP.wdtFeed();
  while (true) {
    Serial.print("Connecting: ");
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.println("Connection Failed! Rebooting...");
      return false;
    } else {
      Serial.println("Connected!");
      // Test internet connectivity
      return true;
    }
  }
}

// void parseCredential(Credential* c, String cred) {
//   Credential c2 = {ssid, pass};
//   c->ssid = ssid;
// }


void loop() {
  ArduinoOTA.handle();
  yield();
  ESP.wdtFeed();
}
