#include <WiFi.h>
#include <esp_now.h>
#include <esp_task_wdt.h>

// ESP-IDF Ethernet
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "driver/spi_master.h"

// LwIP
#include "lwip/sockets.h"
#include "lwip/netdb.h"

// ==========================
// SPI (ENC28J60)
// ==========================
#define PIN_MISO 19
#define PIN_MOSI 23
#define PIN_SCLK 18
#define PIN_CS   17   

// ==========================
// RED
// ==========================
#define SERVER_IP   "192.168.18.52"
#define SERVER_PORT 3000

// ==========================
// LED
// ==========================
#define LED_PIN 26

// ==========================
// VARIABLES GLOBALES
// ==========================
static esp_eth_handle_t eth_handle = NULL;
static bool eth_connected = false;

// ESP-NOW
volatile bool nuevoDato = false;
volatile bool estadoRecibido = false;

bool estadoProcesado = false;

// Cola simple
volatile bool hayPendiente = false;
bool estadoPendiente = false;

// ==========================
// ESTRUCTURA ESP-NOW
// ==========================
typedef struct struct_message {
  bool estado;
} struct_message;

// ==========================
// CALLBACK ESP-NOW
// ==========================
void OnDataRecv(const esp_now_recv_info * info, const uint8_t *incomingData, int len) {

  if (len != sizeof(struct_message)) return;

  struct_message temp;
  memcpy(&temp, incomingData, sizeof(temp));

  estadoRecibido = temp.estado;
  nuevoDato = true;
}

// ==========================
// EVENTOS ETHERNET
// ==========================
static void eth_event_handler(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data) {

  switch (event_id) {

    case ETHERNET_EVENT_CONNECTED:
      Serial.println("Ethernet LINK UP");
      break;

    case ETHERNET_EVENT_DISCONNECTED:
      Serial.println("Ethernet LINK DOWN");
      eth_connected = false;
      break;

    case IP_EVENT_ETH_GOT_IP:
      Serial.println("Ethernet GOT IP");
      eth_connected = true;
      break;
  }
}

// ==========================
// INICIALIZAR ETHERNET
// ==========================
void initEthernet() {

  esp_netif_init();
  esp_event_loop_create_default();

  esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL);
  esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &eth_event_handler, NULL);

  esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
  esp_netif_t *eth_netif = esp_netif_new(&cfg);

  spi_bus_config_t buscfg;
  buscfg.miso_io_num = PIN_MISO;
  buscfg.mosi_io_num = PIN_MOSI;
  buscfg.sclk_io_num = PIN_SCLK;
  buscfg.quadwp_io_num = -1;
  buscfg.quadhd_io_num = -1;
  buscfg.max_transfer_sz = 0;

  spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);

  spi_device_interface_config_t devcfg = {
    .command_bits = 0,
    .address_bits = 0,
    .mode = 0,
    .clock_speed_hz = 8 * 1000 * 1000,
    .spics_io_num = PIN_CS,
    .queue_size = 10
  };

  eth_enc28j60_config_t enc28j60_config = ETH_ENC28J60_DEFAULT_CONFIG(SPI2_HOST, &devcfg);
  eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
  eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();

  esp_eth_mac_t *mac = esp_eth_mac_new_enc28j60(&enc28j60_config, &mac_config);
  esp_eth_phy_t *phy = esp_eth_phy_new_enc28j60(&phy_config);

  esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);

  esp_eth_driver_install(&config, &eth_handle);

  esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle));

  esp_eth_start(eth_handle);

  Serial.println("Ethernet inicializado, esperando IP...");
}

// ==========================
// ENVÍO HTTP
// ==========================
void enviarHTTP(bool estado) {

  if (!eth_connected) {
    Serial.println("Ethernet no listo");
    return;
  }

  struct sockaddr_in dest_addr;
  dest_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(SERVER_PORT);

  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

  if (sock < 0) {
    Serial.println("Socket error");
    return;
  }

  // Timeout
  struct timeval timeout;
  timeout.tv_sec = 2;
  timeout.tv_usec = 0;
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

  if (connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) != 0) {
    Serial.println("Error connect");
    close(sock);
    return;
  }

  char payload[128];

  snprintf(payload, sizeof(payload),
    "POST /api/datos HTTP/1.1\r\n"
    "Host: " SERVER_IP ":3000\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: %d\r\n"
    "Connection: close\r\n\r\n"
    "{\"v\":%d}",
    estado ? 7 : 7,
    estado ? 1 : 0
  );

  int err = send(sock, payload, strlen(payload), 0);
  if (err < 0) {
    Serial.println("Error en send");
  }

  char rx_buffer[64];
  recv(sock, rx_buffer, sizeof(rx_buffer) - 1, MSG_DONTWAIT);

  close(sock);

  Serial.println("HTTP enviado OK");
}

// ==========================
// SETUP
// ==========================
void setup() {

  Serial.begin(115200);

  // Watchdog
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = 5000,
    .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
    .trigger_panic = true
  };

  esp_task_wdt_init(&wdt_config);
  esp_task_wdt_add(NULL);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  WiFi.mode(WIFI_STA);

  // ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("ESP-NOW listo");

  // Ethernet
  initEthernet();
}

// ==========================
// LOOP
// ==========================
void loop() {

  // Procesar ESP-NOW
  if (nuevoDato) {

    noInterrupts();
    estadoProcesado = estadoRecibido;
    nuevoDato = false;
    interrupts();

    digitalWrite(LED_PIN, estadoProcesado ? HIGH : LOW);

    estadoPendiente = estadoProcesado;
    hayPendiente = true;
  }

  // Enviar por Ethernet
  if (hayPendiente) {

    hayPendiente = false;

    vTaskDelay(10); // dar tiempo a WiFi

    enviarHTTP(estadoPendiente);
  }

  // Watchdog
  esp_task_wdt_reset();

  vTaskDelay(1);
}
