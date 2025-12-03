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

const float DETECT_DIST = 20.0;       // cm
const uint8_t SAMPLES = 5;            // lecturas por sensor
const uint16_t SAMPLE_DELAY = 10;     // ms

// Reinicio por inactividad (ms). Permite pausas largas; ajustar si se desea.
// Actualmente 30 minutos = 30 * 60 * 1000 ms
const unsigned long RESET_TIMEOUT_MS = 30UL * 60UL * 1000UL;

// ======================= ESTADOS =======================
enum State {
  IDLE,
  ENTRY_S1, ENTRY_S2, ENTRY_S3, ENTRY_S4, ENTRY_S5, ENTRY_S6,
  EXIT_S1, EXIT_S2, EXIT_S3, EXIT_S4, EXIT_S5, EXIT_S6
};

State state = IDLE;

// ======================= VARIABLES SENSOR =======================
bool s1 = false, s2 = false, s3 = false, s4 = false;
bool prev_s1 = false, prev_s2 = false, prev_s3 = false, prev_s4 = false;

unsigned long lastActiveMillis = 0;

// ===============================================================
// ================== FUNCIONES DE SENSORES ======================
// ===============================================================

long getDistanceRaw(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  long dur = pulseIn(echo, HIGH, 30000);
  if (dur == 0) return 1000;
  return dur * 0.034 / 2;
}

bool readSensorFiltered(int trig, int echo) {
  uint8_t hits = 0;
  for (uint8_t i = 0; i < SAMPLES; i++) {
    long d = getDistanceRaw(trig, echo);
    if (d < DETECT_DIST) hits++;
    delay(SAMPLE_DELAY);
  }
  return (hits >= ((SAMPLES / 2) + 1));
}

// ===============================================================
// ================== ENVÍO AL SERVIDOR ==========================
// ===============================================================

void sendToServer(const String &movimiento, int espacios) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado!");
    return;
  }

  HTTPClient http;
  http.begin(API_URL);
  http.addHeader("Content-Type", "application/json");

  String body = "{\"movimiento\":\"" + movimiento +
                "\",\"espacios\":" + String(espacios) + "}";

  Serial.println("Enviando: " + body);

  int httpCode = http.POST(body);
  Serial.println("Respuesta servidor: " + String(httpCode));

  http.end();
}

// ===============================================================
// ========================= SETUP ===============================
// ===============================================================

void setup() {
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");

  pinMode(TRIG1, OUTPUT); pinMode(ECHO1, INPUT);
  pinMode(TRIG2, OUTPUT); pinMode(ECHO2, INPUT);
  pinMode(TRIG3, OUTPUT); pinMode(ECHO3, INPUT);
  pinMode(TRIG4, OUTPUT); pinMode(ECHO4, INPUT);

  lastActiveMillis = millis();

  Serial.println("--------------- Sistema iniciado ---------------");
}

// ===============================================================
// ================== FUNCIONES DE SECUENCIA =====================
// ===============================================================

void endEntryOk() {
  Serial.println("✔ ENTRADA CONFIRMADA");
  if (totalSpaces > 0) totalSpaces--;
  sendToServer("ENTRADA", totalSpaces);
  state = IDLE;
}

void endExitOk() {
  Serial.println("✔ SALIDA CONFIRMADA");
  if (totalSpaces < 11) totalSpaces++;
  sendToServer("SALIDA", totalSpaces);
  state = IDLE;
}

// ===============================================================
// ========================== LOOP ===============================
// ===============================================================

void loop() {

  // Lectura filtrada: AHORA SI SON BOOLEANOS VERDADEROS
  s1 = readSensorFiltered(TRIG1, ECHO1);
  s2 = readSensorFiltered(TRIG2, ECHO2);
  s3 = readSensorFiltered(TRIG3, ECHO3);
  s4 = readSensorFiltered(TRIG4, ECHO4);

  // Actualizar último momento de actividad si algún sensor está activo
  if (s1 || s2 || s3 || s4) {
    lastActiveMillis = millis();
  } else {
    // Si no hay actividad por mucho tiempo, reiniciamos la FSM para evitar quedarse pegado
    if ((millis() - lastActiveMillis) > RESET_TIMEOUT_MS) {
      if (state != IDLE) {
        Serial.println(">>> Reinicio por inactividad prolongada");
        state = IDLE;
      }
      // actualizamos lastActiveMillis para evitar repetir el mensaje
      lastActiveMillis = millis();
    }
  }

  // DEBUG
  /*
  Serial.printf("\nS1=%d  S2=%d  S3=%d  S4=%d   state=%d   Spaces=%d\n",
                s1, s2, s3, s4, state, totalSpaces);
  */

  // Detectar flancos ascendentes
  bool rise_s1 = (s1 && !prev_s1);
  bool rise_s2 = (s2 && !prev_s2);
  bool rise_s3 = (s3 && !prev_s3);
  bool rise_s4 = (s4 && !prev_s4);

  switch (state) {

    // ===================== IDLE =====================
    case IDLE:
      // iniciar ENTRADA únicamente cuando S1 sube (inicio real)
      if (rise_s1) {
        state = ENTRY_S1;
        Serial.println("Inicio ENTRADA → S1 (flanco)");
      }
      // iniciar SALIDA únicamente cuando S4 sube (inicio real)
      else if (rise_s4) {
        state = EXIT_S1;
        Serial.println("Inicio SALIDA → S4 (flanco)");
      }
      break;

    // ===================== ENTRADA =====================
    // Avanzamos solamente hacia delante con condiciones claras. No hacemos rollback inmediato.
    case ENTRY_S1:
      // esperamos solapamiento s1 && s2 (auto largo)
      if (s1 && s2) {
        state = ENTRY_S2;
        Serial.println("Entrada → S1&S2");
      }
      break;

    case ENTRY_S2:
      // esperamos s1 caer y s2 mantenerse
      if (!s1 && s2) {
        state = ENTRY_S3;
        Serial.println("Entrada → S2 (solo)");
      }
      break;

    case ENTRY_S3:
      // esperamos detección en la zona 3 (arribo después de 20m)
      if (rise_s3) {
        // si s3 aparece, se avanza. si además s4 está activo, go to ENTRY_S4 directly
        if (s3 && s4) {
          state = ENTRY_S5;
          Serial.println("Entrada → S3&S4 (llegada con overlap)");
        } else {
          state = ENTRY_S4;
          Serial.println("Entrada → S3 detectado");
        }
      }
      break;

    case ENTRY_S4:
      // si ambos se solapan en la zona 3-4
      if (s3 && s4) {
        state = ENTRY_S5;
        Serial.println("Entrada → S3&S4 (overlap)");
      }
      break;

    case ENTRY_S5:
      // esperamos s3 caer y s4 mantenerse
      if (!s3 && s4) {
        state = ENTRY_S6;
        Serial.println("Entrada → S4 (solo)");
      }
      break;

    case ENTRY_S6:
      // cuando ambos sensores 3&4 quedan libres -> confirmamos entrada
      if (!s3 && !s4) {
        endEntryOk();
      }
      break;

    // ===================== SALIDA =====================
    case EXIT_S1:
      // esperar solapamiento s4 && s3
      if (s4 && s3) {
        state = EXIT_S2;
        Serial.println("Salida → S4&S3");
      }
      break;

    case EXIT_S2:
      // esperar s4 caer y s3 mantenerse
      if (!s4 && s3) {
        state = EXIT_S3;
        Serial.println("Salida → S3 (solo)");
      }
      break;

    case EXIT_S3:
      // esperar detección en zona 2 (después de 20m)
      if (rise_s2) {
        if (s2 && s1) {
          state = EXIT_S5;
          Serial.println("Salida → S2&S1 (llegada con overlap)");
        } else {
          state = EXIT_S4;
          Serial.println("Salida → S2 detectado");
        }
      }
      break;

    case EXIT_S4:
      if (s2 && s1) {
        state = EXIT_S5;
        Serial.println("Salida → S2&S1 (overlap)");
      }
      break;

    case EXIT_S5:
      // esperar s2 caer y s1 mantenerse
      if (!s2 && s1) {
        state = EXIT_S6;
        Serial.println("Salida → S1 (solo)");
      }
      break;

    case EXIT_S6:
      // cuando ambos sensores 2&1 quedan libres -> confirmamos salida
      if (!s2 && !s1) {
        endExitOk();
      }
      break;
  }

  // guardar lecturas previas
  prev_s1 = s1;
  prev_s2 = s2;
  prev_s3 = s3;
  prev_s4 = s4;

  delay(20);
}
