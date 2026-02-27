#include <WiFi.h>

void setup(){
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
}

void loop(){
  Serial.println("MAC Address:");
  Serial.println(WiFi.macAddress());
  Serial.println("");
  delay(10000);
}