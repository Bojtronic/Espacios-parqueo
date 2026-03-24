#include <esp_now.h>
#include <WiFi.h>
#include <SPI.h>
#include <EthernetENC.h>

// ==========================
// CONFIGURACIÓN ETHERNET
// ==========================
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xFE, 0xFE, 0xED };

IPAddress server(192, 168, 18, 52);

IPAddress ip(192, 168, 18, 60);
IPAddress dnsServer(192, 168, 18, 1);
IPAddress gateway(192, 168, 18, 1);
IPAddress subnet(255, 255, 255, 0);

EthernetClient client;

// ==========================
// ESP-NOW
// ==========================
const int led = 26;

// Variables compartidas
volatile bool nuevoDato = false;
volatile bool estadoRecibido = false;

// Buffer seguro
bool estadoProcesado = false;

typedef struct struct_message {
  bool estado;
} struct_message;

struct_message mensaje;

// ==========================
// CALLBACK ESP-NOW (SEGURO)
// ==========================
void OnDataRecv(const esp_now_recv_info * info, const uint8_t *incomingData, int len) {

  if (len != sizeof(struct_message)) {
    Serial.println("Paquete inválido");
    return;
  }

  struct_message temp;
  memcpy(&temp, incomingData, sizeof(temp));

  estadoRecibido = temp.estado;
  nuevoDato = true;
}

// ==========================
// ENVÍO HTTP
// ==========================
void enviarEthernet(bool estado) {

  if (!client.connect(server, 3000)) {
    Serial.println("Error conectando");
    client.stop(); // limpiar
    return;
  }

  char json[12];
  snprintf(json, sizeof(json), "{\"v\":%d}", estado ? 1 : 0);

  client.println("POST /api/datos HTTP/1.1");
  client.println("Host: 192.168.18.52:3000");
  client.println("Content-Type: application/json");
  client.print("Content-Length: ");
  client.println(strlen(json));
  client.println("Connection: close");
  client.println();

  client.print(json);

  // CLAVE: leer respuesta (libera buffers)
  unsigned long timeout = millis();

  while (client.connected() && millis() - timeout < 200) {
    while (client.available()) {
      client.read(); // descartamos datos
      timeout = millis();
    }
  }

  client.stop(); // cerrar limpio
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
  SPI.begin(18, 19, 23);
  Ethernet.init(5);

  Serial.println("Inicializando Ethernet...");

  Ethernet.begin(mac, ip, gateway, gateway, subnet);
  
  /*
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
  */

  Serial.print("IP local: ");
  Serial.println(Ethernet.localIP());
}

// ==========================
// LOOP PRINCIPAL
// ==========================
void loop() {

  if (nuevoDato) {

    // Copia segura
    noInterrupts();
    estadoProcesado = estadoRecibido;
    nuevoDato = false;
    interrupts();

    Serial.print("Estado recibido: ");
    Serial.println(estadoProcesado ? "ENCENDIDO" : "APAGADO");

    // LED inmediato (NO BLOQUEANTE)
    digitalWrite(led, estadoProcesado ? HIGH : LOW);

    // Enviar después (puede bloquear, pero ya no afecta recepción)
    enviarEthernet(estadoProcesado);
  }
}
