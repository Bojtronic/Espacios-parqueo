#include <esp_now.h>
#include <WiFi.h>

const int led = 26;

typedef struct struct_message {
  bool estado;
} struct_message;

struct_message mensaje;

void OnDataRecv(const esp_now_recv_info * info, const uint8_t *incomingData, int len) {
  memcpy(&mensaje, incomingData, sizeof(mensaje));

  digitalWrite(led, mensaje.estado ? HIGH : LOW);
}

void setup() {
  Serial.begin(115200);
  pinMode(led, OUTPUT);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error inicializando ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {}