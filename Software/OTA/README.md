Code Dump:
```

// Webserver:
ESP8266WebServer server(80);

void handleRoot() {
  server.send(200, "text/plain", "hello from esp8266!");
}

void setupWebserver() {
  server.on("/", [](){
    server.send(200, "text/plain", "this works really well");
  });
  server.on("/main.css", [](){
    server.send(200, "text/plain", "");
  });
  server.begin();
}

server.handleClient();

// Wifi Scanner
void scan() {
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
     {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.print(WiFi.encryptionType(i));
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
      delay(10);
     }
  }
  Serial.println("");
}

```
