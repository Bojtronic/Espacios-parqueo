#include <esp_now.h>
#include <esp_task_wdt.h>
#include <WiFi.h>
#include <SPI.h>
#include <EthernetENC.h>

// ==========================
// CONFIGURACIÓN ETHERNET
// ==========================
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xFE, 0xFE, 0xED };

// IP del servidor
IPAddress server(192, 168, 18, 52);

// Configuración estática
IPAddress ip(192, 168, 18, 60);
IPAddress dnsServer(192, 168, 18, 1);
IPAddress gateway(192, 168, 18, 1);
IPAddress subnet(255, 255, 255, 0);

EthernetClient client;

unsigned long ultimaVerificacionETH = 0;
unsigned long timeconn = 10UL * 60UL * 1000UL; // minutos*60*1000

// ==========================
// ESP-NOW
// ==========================
const int led = 26;

// Variables compartidas (ISR)
volatile bool nuevoDato = false;
volatile bool estadoRecibido = false;

// Buffer de procesamiento (fuera de ISR)
bool estadoProcesado = false;

// ==========================
// SPI
// ==========================
#define VSPI_MISO 19
#define VSPI_MOSI 23
#define VSPI_SCLK 18
#define VSPI_CS   17 // 5

typedef struct struct_message {
  bool estado;
} struct_message;

// ==========================
// CALLBACK ESP-NOW
// ==========================
void OnDataRecv(const esp_now_recv_info * info, const uint8_t *incomingData, int len) {

  // Validación de tamaño
  if (len != sizeof(struct_message)) return;

  struct_message temp;

  // Copia segura local (rápida)
  memcpy(&temp, incomingData, sizeof(temp));

  // Actualizar flags (mínimo trabajo en ISR)
  estadoRecibido = temp.estado;
  nuevoDato = true;
}

// ==========================
// ENVÍO HTTP
// ==========================
void enviarEthernet(bool estado) {

  unsigned long start = millis();

  client.stop(); // limpiar siempre antes de conectar

  // Intentar conexión
  if (!client.connect(server, 3000)) {
    Serial.println("Error de conexión");
    return;
  }

  // JSON mínimo
  char json[12];
  snprintf(json, sizeof(json), "{\"v\":%d}", estado ? 1 : 0);

  // Cabecera HTTP
  client.println("POST /api/datos HTTP/1.1");
  client.println("Host: 192.168.18.52:3000");
  client.println("Content-Type: application/json");
  client.print("Content-Length: ");
  client.println(strlen(json));
  client.println("Connection: close");
  client.println();

  // Enviar cuerpo
  client.print(json);

  // liberar buffers leyendo respuesta
  unsigned long timeout = millis();

  while (client.connected() && millis() - timeout < 100) {
    while (client.available()) {
      client.read(); // descartamos datos
      timeout = millis();
    }
  }

  client.stop(); // cerrar conexión
}

// ==========================
// SETUP
// ==========================
void setup() {
  Serial.begin(115200);

  esp_task_wdt_add(NULL);

  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);

  WiFi.mode(WIFI_STA);

  // Inicializar ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("ESP-NOW listo");

  // ==========================
  // SPI + Ethernet
  // ==========================
  SPI.begin(VSPI_SCLK, VSPI_MISO, VSPI_MOSI, VSPI_CS);
  Ethernet.init(VSPI_CS);

  Serial.println("Inicializando Ethernet...");

  // Configuración directa (más estable que DHCP)
  Ethernet.begin(mac, ip, dnsServer, gateway, subnet);

  Serial.print("IP local: ");
  Serial.println(Ethernet.localIP());
}

// ==========================
// LOOP PRINCIPAL
// ==========================
void loop() {

  // Si llegó un dato por ESP-NOW
  if (nuevoDato) {

    // Copia segura (evita corrupción)
    noInterrupts();
    estadoProcesado = estadoRecibido;
    nuevoDato = false;
    interrupts();

    // Debug
    Serial.print("Estado recibido: ");
    Serial.println(estadoProcesado ? "ENCENDIDO" : "APAGADO");

    // Encender LED
    digitalWrite(led, estadoProcesado ? HIGH : LOW);

    // Envío por Ethernet (más lento)
    enviarEthernet(estadoProcesado);
  }


  if ((Ethernet.linkStatus() == LinkOFF) && ((millis() - ultimaVerificacionETH) > timeconn)) {
    Serial.println("NO hay conexion Ethernet");
    //ESP.restart();
    Ethernet.begin(mac, ip, dnsServer, gateway, subnet);
    ultimaVerificacionETH = millis();
    return;
  }

  // Mantener vivo el watchdog
  esp_task_wdt_reset();

  // delay() es un bloqueo activo que consume ciclos de CPU, 
  // mientras que vTaskDelay() libera la CPU.
  vTaskDelay(10);
}
