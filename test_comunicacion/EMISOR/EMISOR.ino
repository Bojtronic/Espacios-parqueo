#include <esp_now.h>
#include <WiFi.h>

uint8_t direccionReceptor[] = {0xXX,0xXX,0xXX,0xXX,0xXX,0xXX}; // PONER MAC AQUÍ

const int boton = 25;

typedef struct struct_message {
  bool estado;
} struct_message;

struct_message mensaje;

void setup() {
  Serial.begin(115200);

  pinMode(boton, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error inicializando ESP-NOW");
    return;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, direccionReceptor, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Error agregando peer");
    return;
  }
}

void loop() {
  bool estadoBoton = digitalRead(boton) == LOW;

  mensaje.estado = estadoBoton;
  esp_now_send(direccionReceptor, (uint8_t *) &mensaje, sizeof(mensaje));

  delay(100);
}