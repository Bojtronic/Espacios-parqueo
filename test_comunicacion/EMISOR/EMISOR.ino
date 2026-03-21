#include <esp_now.h>
#include <WiFi.h>

uint8_t direccionReceptor[] = {0x20, 0xE7, 0xC8, 0xAC, 0x05, 0x70};

const int boton = 25;

typedef struct struct_message {
  bool estado;
} struct_message;

struct_message mensaje;

// Estado anterior del botón
bool estadoAnterior = HIGH;

// firma para core 3.x
void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.print("Estado de envio: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK" : "FALLO");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(boton, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error inicializando ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, direccionReceptor, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Error agregando peer");
    return;
  }

  Serial.println("Emisor listo");
}

void loop() {
  bool estadoActual = digitalRead(boton);

  // 🔽 Flanco de bajada: botón presionado
  if (estadoAnterior == HIGH && estadoActual == LOW) {

    mensaje.estado = true;

    Serial.println("Botón PRESIONADO → enviando TRUE");

    esp_now_send(direccionReceptor, (uint8_t *) &mensaje, sizeof(mensaje));
  }

  // 🔼 Flanco de subida: botón soltado
  if (estadoAnterior == LOW && estadoActual == HIGH) {

    mensaje.estado = false;

    Serial.println("Botón SOLTADO → enviando FALSE");

    esp_now_send(direccionReceptor, (uint8_t *) &mensaje, sizeof(mensaje));
  }

  estadoAnterior = estadoActual;

  delay(10); // debounce básico
}