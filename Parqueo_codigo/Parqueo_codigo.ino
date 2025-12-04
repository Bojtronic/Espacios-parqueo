#include "secrets.h"
#include <WiFi.h>
#include <HTTPClient.h>

// ======================= PINES =======================
#define TRIG1 16
#define ECHO1 36
#define TRIG2 17
#define ECHO2 39
#define TRIG3 18
#define ECHO3 34
#define TRIG4 23
#define ECHO4 35

// ======================= PARÁMETROS =======================
int totalSpaces = 11;

const float DETECT_DIST = 20.0;   // cm
const uint8_t SAMPLES = 5;        // lecturas por sensor
const uint16_t SAMPLE_DELAY = 10; // ms

// Reinicio por inactividad (ms). Permite pausas largas.
const unsigned long RESET_TIMEOUT_MS = 30UL * 60UL * 1000UL;

// ======================= ESTADOS =======================
enum State
{
  IDLE,
  ENTRY_S1,
  ENTRY_S2,
  ENTRY_S3,
  ENTRY_S4,
  ENTRY_S5,
  ENTRY_S6,
  EXIT_S1,
  EXIT_S2,
  EXIT_S3,
  EXIT_S4,
  EXIT_S5,
  EXIT_S6
};

State state = IDLE;

// ======================= VARIABLES SENSOR =======================
bool s1 = false, s2 = false, s3 = false, s4 = false;
bool prev_s1 = false, prev_s2 = false, prev_s3 = false, prev_s4 = false;

unsigned long lastActiveMillis = 0;

// =======================================================================
// ============================= FUNCIONES ===============================
// =======================================================================

long getDistanceRaw(int trig, int echo)
{
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  long dur = pulseIn(echo, HIGH, 30000);
  if (dur == 0)
    return 1000;
  return dur * 0.034 / 2;
}

bool readSensorFiltered(int trig, int echo)
{
  uint8_t hits = 0;
  for (uint8_t i = 0; i < SAMPLES; i++)
  {
    long d = getDistanceRaw(trig, echo);
    if (d < DETECT_DIST)
      hits++;
    delay(SAMPLE_DELAY);
  }
  return (hits >= ((SAMPLES / 2) + 1));
}

// =======================================================================
// ========================= ENVÍO SERVIDOR ===============================
// =======================================================================

void sendToServer(const String &movimiento, int espacios)
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi desconectado!");
    return;
  }

  HTTPClient http;
  http.begin(API_URL);
  http.addHeader("Content-Type", "application/json");

  String body = "{\"movimiento\":\"" + movimiento + "\",\"espacios\":" + String(espacios) + "}";

  Serial.println("Enviando: " + body);

  int httpCode = http.POST(body);
  Serial.println("Respuesta servidor: " + String(httpCode));

  http.end();
}

// =======================================================================
// =============================== ESTADOS ================================
// =======================================================================

void logStateChange(State newState)
{
  Serial.print("STATE → ");
  Serial.print(state);
  Serial.print("   ==>   ");
  Serial.println(newState);
}

void endEntryOk()
{
  Serial.println("✔ ENTRADA CONFIRMADA (FIN SECUENCIA)");
  if (totalSpaces > 0)
    totalSpaces--;
  sendToServer("ENTRADA", totalSpaces);
  logStateChange(IDLE);
  state = IDLE;
}

void endExitOk()
{
  Serial.println("✔ SALIDA CONFIRMADA (FIN SECUENCIA)");
  if (totalSpaces < 11)
    totalSpaces++;
  sendToServer("SALIDA", totalSpaces);
  logStateChange(IDLE);
  state = IDLE;
}

// =======================================================================
// =============================== SETUP =================================
// =======================================================================

void setup()
{
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");

  pinMode(TRIG1, OUTPUT);
  pinMode(ECHO1, INPUT);
  pinMode(TRIG2, OUTPUT);
  pinMode(ECHO2, INPUT);
  pinMode(TRIG3, OUTPUT);
  pinMode(ECHO3, INPUT);
  pinMode(TRIG4, OUTPUT);
  pinMode(ECHO4, INPUT);

  lastActiveMillis = millis();

  Serial.println("--------------- Sistema iniciado ---------------");
}

// =======================================================================
// =============================== LOOP ==================================
// =======================================================================

void loop()
{

  // Lecturas
  s1 = readSensorFiltered(TRIG1, ECHO1);
  s2 = readSensorFiltered(TRIG2, ECHO2);
  s3 = readSensorFiltered(TRIG3, ECHO3);
  s4 = readSensorFiltered(TRIG4, ECHO4);

  // DETALLES DE SENSORES (ON/OFF)
  if (s1 != prev_s1)
    Serial.println(String("S1 ") + (s1 ? "ON" : "OFF"));
  if (s2 != prev_s2)
    Serial.println(String("S2 ") + (s2 ? "ON" : "OFF"));
  if (s3 != prev_s3)
    Serial.println(String("S3 ") + (s3 ? "ON" : "OFF"));
  if (s4 != prev_s4)
    Serial.println(String("S4 ") + (s4 ? "ON" : "OFF"));

  // Actualizar tiempo activo
  if (s1 || s2 || s3 || s4)
  {
    lastActiveMillis = millis();
  }
  else if ((millis() - lastActiveMillis) > RESET_TIMEOUT_MS)
  {
    if (state != IDLE)
    {
      Serial.println(">>> Reinicio por inactividad prolongada");
      logStateChange(IDLE);
      state = IDLE;
    }
    lastActiveMillis = millis();
  }

  // Flancos (subidas)
  bool rise_s1 = (s1 && !prev_s1);
  bool rise_s2 = (s2 && !prev_s2);
  bool rise_s3 = (s3 && !prev_s3);
  bool rise_s4 = (s4 && !prev_s4);

  if (rise_s1)
    Serial.println("FLANCO ↑ S1");
  if (rise_s2)
    Serial.println("FLANCO ↑ S2");
  if (rise_s3)
    Serial.println("FLANCO ↑ S3");
  if (rise_s4)
    Serial.println("FLANCO ↑ S4");

  // ===================================================================
  // ============================= FSM =================================
  // ===================================================================

  switch (state)
  {

  // ===================================================================
  // IDLE
  // ===================================================================
  case IDLE:
    if (s1)
    {
      Serial.println("Inicio ENTRADA → s1");
      logStateChange(ENTRY_S1);
      state = ENTRY_S1;
    }
    else if (s4)
    {
      Serial.println("Inicio SALIDA → s4");
      logStateChange(EXIT_S1);
      state = EXIT_S1;
    }
    break;

    // ===================================================================
    // =========================== ENTRADA ===============================
    // ===================================================================

  case ENTRY_S1: // s1
    if (s1 && s2)
    {
      Serial.println("Entrando: s1 → s1&s2");
      logStateChange(ENTRY_S2);
      state = ENTRY_S2;
    }
    else if (!s1)
    { // retroceso completo
      Serial.println("RETROCESO: s1 → IDLE");
      logStateChange(IDLE);
      state = IDLE;
    }
    break;

  case ENTRY_S2: // s1 & s2
    if (!s1 && s2)
    {
      Serial.println("Entrando: s1&s2 → s2");
      logStateChange(ENTRY_S3);
      state = ENTRY_S3;
    }
    else if (s1 && !s2)
    {
      Serial.println("Retroceso: s1&s2 → s1");
      logStateChange(ENTRY_S1);
      state = ENTRY_S1;
    }
    break;

  case ENTRY_S3: // s2
    if (s3 && !s4)
    {
      Serial.println("Entrando: s2 → s3");
      logStateChange(ENTRY_S4);
      state = ENTRY_S4;
    }
    else if (s1 && s2)
    {
      Serial.println("Retroceso: s2 → s1&s2");
      logStateChange(ENTRY_S2);
      state = ENTRY_S2;
    }
    break;

  case ENTRY_S4: // s3
    if (s3 && s4)
    {
      Serial.println("Entrando: s3 → s3&s4");
      logStateChange(ENTRY_S5);
      state = ENTRY_S5;
    }
    else if (!s3 && s2)
    {
      Serial.println("Retroceso: s3 → s2");
      logStateChange(ENTRY_S3);
      state = ENTRY_S3;
    }
    break;

  case ENTRY_S5: // s3 & s4
    if (!s3 && s4)
    {
      Serial.println("Entrando: s3&s4 → s4");
      logStateChange(ENTRY_S6);
      state = ENTRY_S6;
    }
    else if (s3 && !s4)
    {
      Serial.println("Retroceso: s3&s4 → s3");
      logStateChange(ENTRY_S4);
      state = ENTRY_S4;
    }
    break;

  case ENTRY_S6: // solo s4 (fin de entrada)
    if (!s4)
    {
      endEntryOk();
      state = IDLE;
    }
    else if (s3 && s4)
    {
      Serial.println("Retroceso: s4 → s3&s4");
      logStateChange(ENTRY_S5);
      state = ENTRY_S5;
    }
    break;

    // ===================================================================
    // ============================ SALIDA ===============================
    // ===================================================================

  case EXIT_S1: // s4
    if (s4 && s3)
    {
      Serial.println("Saliendo: s4 → s4&s3");
      logStateChange(EXIT_S2);
      state = EXIT_S2;
    }
    else if (!s4)
    {
      Serial.println("Retroceso: s4 → IDLE");
      logStateChange(IDLE);
      state = IDLE;
    }
    break;

  case EXIT_S2: // s4 & s3
    if (!s4 && s3)
    {
      Serial.println("Saliendo: s4&s3 → s3");
      logStateChange(EXIT_S3);
      state = EXIT_S3;
    }
    else if (s4 && !s3)
    {
      Serial.println("Retroceso: s4&s3 → s4");
      logStateChange(EXIT_S1);
      state = EXIT_S1;
    }
    break;

  case EXIT_S3: // s3
    if (s2 && !s1)
    {
      Serial.println("Saliendo: s3 → s2");
      logStateChange(EXIT_S4);
      state = EXIT_S4;
    }
    else if (s4 && s3)
    {
      Serial.println("Retroceso: s3 → s4&s3");
      logStateChange(EXIT_S2);
      state = EXIT_S2;
    }
    break;

  case EXIT_S4: // s2
    if (s1 && s2)
    {
      Serial.println("Saliendo: s2 → s1&s2");
      logStateChange(EXIT_S5);
      state = EXIT_S5;
    }
    else if (!s2 && s3)
    {
      Serial.println("Retroceso: s2 → s3");
      logStateChange(EXIT_S3);
      state = EXIT_S3;
    }
    break;

  case EXIT_S5: // s1 & s2
    if (s1 && !s2)
    {
      Serial.println("Saliendo: s1&s2 → s1");
      logStateChange(EXIT_S6);
      state = EXIT_S6;
    }
    else if (s2 && !s1)
    {
      Serial.println("Retroceso: s1&s2 → s2");
      logStateChange(EXIT_S4);
      state = EXIT_S4;
    }
    break;

  case EXIT_S6: // solo s1 (fin de salida)
    if (!s1)
    {
      endExitOk();
      state = IDLE;
    }
    else if (s1 && s2)
    {
      Serial.println("Retroceso: s1 → s1&s2");
      logStateChange(EXIT_S5);
      state = EXIT_S5;
    }
    break;
  }

  // Guardar estados previos
  prev_s1 = s1;
  prev_s2 = s2;
  prev_s3 = s3;
  prev_s4 = s4;

  delay(20);
}
