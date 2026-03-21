#include <esp_now.h>
#include <WiFi.h>
#include <SPI.h>
#include <EthernetENC.h>

// ==========================
// CONFIGURACIÓN ETHERNET
// ==========================
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// IP del servidor Node.js
IPAddress server(192, 168, 18, 52);

// IP estática (fallback)
IPAddress ip(192, 168, 18, 60);
IPAddress dnsServer(192, 168, 18, 1);
IPAddress gateway(192, 168, 18, 1);
IPAddress subnet(255, 255, 255, 0);

EthernetClient client;

// ==========================
// ESP-NOW
// ==========================
const int led = 26;

bool nuevoDato = false;
bool estadoRecibido = false;
bool ultimoEstadoEnviado = false;

typedef struct struct_message {
  bool estado;
} struct_message;

struct_message mensaje;

// ==========================
// CALLBACK ESP-NOW
// ==========================
void OnDataRecv(const esp_now_recv_info * info, const uint8_t *incomingData, int len) {
  memcpy(&mensaje, incomingData, sizeof(mensaje));

  estadoRecibido = mensaje.estado;
  nuevoDato = true;
}

// ==========================
// ENVÍO HTTP
// ==========================
void enviarEthernet(bool estado) {

  if (!client.connect(server, 3000)) {
    Serial.println("Error conectando al servidor");
    return;
  }

  char json[64];
  snprintf(json, sizeof(json),
           "{\"mensaje\":\"LED\",\"valor\":%d}",
           estado ? 1 : 0);

  client.println("POST /api/datos HTTP/1.1");
  client.println("Host: 192.168.18.52:3000");
  client.println("Content-Type: application/json");
  client.print("Content-Length: ");
  client.println(strlen(json));
  client.println("Connection: close");
  client.println();

  client.print(json);

  Serial.print("Enviado: ");
  Serial.println(json);

  delay(10);
  client.stop();
}

// ==========================
// SETUP
// ==========================
void setup() {
  Serial.begin(115200);

  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("ESP-NOW listo");

  // ==========================
  // SPI + Ethernet
  // ==========================
  SPI.begin(18, 19, 23, 5);
  Ethernet.init(5);

  Serial.println("Inicializando Ethernet...");

  if (Ethernet.begin(mac)) {
    Serial.println("DHCP OK!");
  } else {
    Serial.println("Fallo DHCP → usando IP estática");

    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("ERROR: ENC28J60 no detectado");
      while (true) delay(1);
    }

    if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Cable Ethernet desconectado");
    }

    Ethernet.begin(mac, ip, dnsServer, gateway, subnet);
    Serial.println("IP estática configurada");
  }

  delay(2000);

  Serial.print("IP local: ");
  Serial.println(Ethernet.localIP());
}

// ==========================
// LOOP
// ==========================
void loop() {

  if (nuevoDato) {

    nuevoDato = false;

    Serial.print("Estado: ");
    Serial.println(estadoRecibido ? "ENCENDIDO" : "APAGADO");

    digitalWrite(led, estadoRecibido ? HIGH : LOW);

    if (estadoRecibido != ultimoEstadoEnviado) {

      Serial.println("Cambio detectado → enviando");

      enviarEthernet(estadoRecibido);

      ultimoEstadoEnviado = estadoRecibido;
    }
  }
}
