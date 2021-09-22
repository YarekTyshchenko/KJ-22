#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <WebServer.h>
#endif
#include <time.h>
#include <AutoConnect.h>

#include <AceTime.h>
#include <sntp.h>
#include <SPI.h>

#define LOAD D4
#define BLANK D3

using namespace ace_time;

static const char AUX_TIMEZONE[] PROGMEM = R"(
{
  "title": "TimeZone",
  "uri": "/timezone",
  "menu": true,
  "element": [
    {
      "name": "caption",
      "type": "ACText",
      "value": "Sets the time zone to get the current local time.",
      "style": "font-family:Arial;font-weight:bold;text-align:center;margin-bottom:10px;color:DarkSlateBlue"
    },
    {
      "name": "timezone",
      "type": "ACSelect",
      "label": "Select TimeZone",
      "option": []
    },
    {
      "type": "ACElement",
      "value": "<script type='text/javascript'>
window.onload=function(){
var xhr=new XMLHttpRequest();
xhr.onreadystatechange=function(){
if(this.readyState==4&&xhr.status==200){
var currentZone=xhr.getResponseHeader('x-current-zone-id');
var e=document.getElementById('timezone');
for(var i of this.responseText.trim().split('\\n').sort()){
var [name, id]=i.trim().split('|');
e.appendChild(new Option(name, id));
}
for(var v = 0;v<e.options.length;v++){
var o=e.options[v];
if(o.value==currentZone) o.selected=true;
}
}
}
xhr.open('GET', '/stream_timezones');
xhr.responseType='text';
xhr.send();
}
</script>"
    },
    {
      "name": "start",
      "type": "ACSubmit",
      "value": "Set TimeZone",
      "uri": "/set_timezone"
    }
  ]
}
)";

#if defined(ARDUINO_ARCH_ESP8266)
ESP8266WebServer Server;
#elif defined(ARDUINO_ARCH_ESP32)
WebServer Server;
#endif

AutoConnect       Portal(Server);
AutoConnectConfig Config;
AutoConnectAux    Timezone;

uint32_t zoneId = zonedb::kZoneIdEtc_UTC;
static BasicZoneManager<1> zoneManager(zonedb::kZoneRegistrySize, zonedb::kZoneRegistry);

clock::NtpClock ntpClock("pool.ntp.org");
clock::SystemClockLoop systemClock(&ntpClock, nullptr, 60);

void rootPage() {
  String  content =
    "<html>"
    "<head>"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "<script type=\"text/javascript\">"
    "setTimeout(\"location.reload()\", 1000);"
    "</script>"
    "</head>"
    "<body>"
    "<h2 align=\"center\" style=\"color:blue;margin:20px;\">Last Sync time:</h2>"
    "<h3 align=\"center\" style=\"color:gray;margin:10px;\">{{DateTime}}</h3>"
    "<p style=\"text-align:center;\">Reload the page to update the time.</p>"
    "<p></p><p style=\"padding-top:15px;text-align:center\">" AUTOCONNECT_LINK(COG_24) "</p>"
    "</body>"
    "</html>";
  
  TimeZone zone = zoneManager.createForZoneId(zoneId);
  
  acetime_t lastSyncTime = systemClock.getLastSyncTime();
  auto zdt = ZonedDateTime::forEpochSeconds(lastSyncTime, zone);
  if (zdt.isError()) {
    Server.send(500, "text/html", F("Error creating ZonedDateTime"));
    return;
  }

  ace_common::PrintStr<64> dateTime;
  zdt.printTo(dateTime);
  
  content.replace("{{DateTime}}", dateTime.getCstr());
  
  Server.send(200, "text/html", content);
}

acetime_t now;
void showClock() {
  acetime_t newNow = systemClock.getNow();
  if (now == newNow) {
    return;
  }
  now = newNow;
  //systemClock.getLastSyncTime();

  TimeZone zone = zoneManager.createForZoneId(zoneId);
  auto zdt = ZonedDateTime::forEpochSeconds(now, zone);
  if (zdt.isError()) {
    Serial.println(F("Error creating ZonedDateTime"));
    return;
  }
  
  displayNumber(zdt.hour() * 100 + zdt.minute());
}

void setBrightness() {
  String brightness = Server.arg("brightness");
  int a = brightness.toInt();
  analogWrite(BLANK, a);
  displayNumber(a);
  Server.send(302, "text/plain", String(a));
}

void setTimezonePage() {
  // Retrieve the value of AutoConnectElement with arg function of WebServer class.
  // Values are accessible with the element name.
  String tz = Server.arg("timezone");
  tz.trim();
  uint32_t selectedZoneId = tz.toInt();
  if (0) {
    Server.send(500, "text/html", F("Invalid Zone ID"));
    return;
  }
  // Check zone is valid
  auto selectedZone = zoneManager.createForZoneId(selectedZoneId);
  if (selectedZone.isError()) {
    Server.send(500, "text/html", F("Zone ID not found in registrar"));
    return;
  }
  // Save it in memory
  zoneId = selectedZone.getZoneId();
  EEPROM.put(0, zoneId);
  EEPROM.commit();
  
  // Redirect to Root
  Server.sendHeader("Location", String("http://") + Server.client().localIP().toString() + String("/"));
  Server.send(302, "text/plain", "");
  Server.client().flush();
  Server.client().stop();
}

void streamTimezones() {
  // Send current timezone in a header
  Server.sendHeader("x-current-zone-id", String(zoneId));
  
  // TODO: What to do if this returns false?
  Server.chunkedResponseModeStart(200, "text/plain");
  ace_common::PrintStr<128> printer;
  for(uint16_t n = 0; n < zonedb::kZoneRegistrySize; n++) {
    BasicZone basicZone(zonedb::kZoneRegistry[n]);

    // Format: Zone_Name|<zone_id>\n
    basicZone.printNameTo(printer);
    printer.print('|');
    printer.print(basicZone.zoneId());
    printer.println();

    Server.sendContent(printer.getCstr());
    printer.flush();
  }
  Server.chunkedResponseFinalize();
}

/*
 . a .
 b   c
 . d .
 e   f
 . g . h
*/
//               1             2            3
//  0123 4567  8901 2345  6789 0123  4567 8901  2345 6789
//  0000 0000  0000 0000  0000 0000  0000 0000  0000 0000
//1 cabf  hGe         gd
//2            gfba hG          ced  
//3                        eGg         hf acdb
//4                                  be         Ghgf cd a

const byte bulbPattern[] = {
 //hgfedcba
  B01110111, // 0
  B00100100, // 1
  B01011101, // 2
  B01101101, // 3
  B00101110, // 4
  B01101011, // 5
  B01111011, // 6
  B00100101, // 7
  B01111111, // 8
  B01101111, // 9
};
uint8_t pattern1[] = {1, 2, 0, 15, 7, 3, 14, 5, 6};
uint8_t pattern2[] = {11, 10, 21, 23, 22, 9, 8, 12, 13};
uint8_t pattern3[] = {28, 31, 29, 30, 17, 27, 19, 26, 18};
uint8_t pattern4[] = {39, 24, 36, 37, 25, 35, 34, 33, 32};

void displayNumber(long number) {
  byte buffer[5] = {
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
  };

  int thousands = number / 1000;
  int hundreds = number / 100 % 10;
  int tens = number / 10 % 10;
  int units = number % 10;

  for (int i = 0; i <= 7; i++) {
    if (!bitRead(bulbPattern[thousands], i)) {
      int bit = pattern1[i];
      bitClear(buffer[bit / 8], bit % 8);
    }
    if (!bitRead(bulbPattern[hundreds], i)) {
      int bit = pattern2[i];
      bitClear(buffer[bit / 8], bit % 8);
    }
    if (!bitRead(bulbPattern[tens], i)) {
      int bit = pattern3[i];
      bitClear(buffer[bit / 8], bit % 8);
    }
    if (!bitRead(bulbPattern[units], i)) {
      int bit = pattern4[i];
      bitClear(buffer[bit / 8], bit % 8);
    }
  }
  spiWrite(buffer);
}

void spiWrite(byte buffer[]) {
  digitalWrite(LOAD, LOW);
  SPI.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0));
  SPI.transfer(buffer[0]);
  SPI.transfer(buffer[1]);
  SPI.transfer(buffer[2]);
  SPI.transfer(buffer[3]);
  SPI.transfer(buffer[4]);
  SPI.endTransaction();
  digitalWrite(LOAD, HIGH);
}

void setup() {
  SPI.begin();
  // Init LOAD
  pinMode(LOAD, OUTPUT);
  digitalWrite(LOAD, HIGH);
  // Init BLANK
  pinMode(BLANK, OUTPUT);
  // analogWrite(BLANK, 230);

  int n = 0;
  while (n <= 9) {
    delay(250);
    displayNumber(1111 * n++);
  }

  displayNumber(2);
  Serial.begin(115200);
  displayNumber(3);
  Serial.println();
  
  
  ntpClock.setup();
  displayNumber(4);
  systemClock.setup();
  displayNumber(5);
  // Set clock to 0 to prevent issues with Zones before sync.
  systemClock.setNow(0);

  // Enable saved past credential by autoReconnect option,
  // even once it is disconnected.
  Config.boundaryOffset = sizeof(uint32_t);
  Config.autoReconnect = true;
  Config.ticker = true;
  Portal.config(Config);

  displayNumber(6);
  
  // EEPROM Config
  EEPROM.begin(sizeof(uint32_t));
  uint32_t storedZoneId;
  EEPROM.get(0, storedZoneId);
  auto selectedZone = zoneManager.createForZoneId(storedZoneId);
  if (!selectedZone.isError()) {
    Serial.println(storedZoneId);
    zoneId = storedZoneId;
  } else {
    Serial.println(F("Invalid Zone ID from EEPROM"));
  }

  // Load aux. page
  Timezone.load(AUX_TIMEZONE);
  Portal.join(Timezone);        // Register aux. page

  // Behavior a root path of ESP8266WebServer.
  Server.on("/", rootPage);
  // Set NTP server trigger handler
  Server.on("/set_timezone", setTimezonePage);
  Server.on("/stream_timezones", streamTimezones);
  Server.on("/set_brightness", setBrightness);

  // Establish a connection with an autoReconnect option.
  if (Portal.begin()) {
    Serial.println("WiFi connected: " + WiFi.localIP().toString());
  }
  displayNumber(7);
}

void loop() {
  Portal.handleClient();
  systemClock.loop();

  showClock();
}
