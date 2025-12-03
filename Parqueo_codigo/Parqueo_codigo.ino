#include "secrets.h"
#include <WiFi.h>
#include <HTTPClient.h>

#define TRIG1 16
#define ECHO1 36
#define TRIG2 17
#define ECHO2 39
#define TRIG3 18
#define ECHO3 34
#define TRIG4 23
#define ECHO4 35

int totalSpaces = 11;
const float DETECT_DIST = 10.0;

enum State {
  IDLE,
  ENTRY_STAGE1,
  ENTRY_STAGE2,
  ENTRY_STAGE3,
  ENTRY_STAGE4,
  ENTRY_STAGE5,
  EXIT_STAGE1,
  EXIT_STAGE2,
  EXIT_STAGE3,
  EXIT_STAGE4,
  EXIT_STAGE5
};

State currentState = IDLE;

bool s1, s2, s3, s4;

// -------------------- ENVIAR POST AL SERVIDOR ---------------------------
void sendToServer(String movimiento, int espacios) {
  if (WiFi.status() != WL_CONNECTED) {
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

// -------------------- SENSOR ULTRASONICO ---------------------------
long getDistance(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  long dur = pulseIn(echo, HIGH, 30000);
  if (dur == 0) return 1000;
  return dur * 0.034 / 2;
}

// -------------------- SETUP ---------------------------
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

  Serial.println("Sistema iniciado");
}

// -------------------- LOOP PRINCIPAL ---------------------------
void loop() {

  s1 = (getDistance(TRIG1, ECHO1) < DETECT_DIST);
  s2 = (getDistance(TRIG2, ECHO2) < DETECT_DIST);
  s3 = (getDistance(TRIG3, ECHO3) < DETECT_DIST);
  s4 = (getDistance(TRIG4, ECHO4) < DETECT_DIST);

  bool invalidSequence = false;

  switch(currentState) {

    case IDLE:
      if (s1 && !s2) currentState = ENTRY_STAGE1;
      else if (s4 && !s3) currentState = EXIT_STAGE1;
      else if ((s2 && !s1) || (s3 && !s4))
        invalidSequence = true;
      break;

    // ================== ENTRADA =====================
    case ENTRY_STAGE1:
      if (s1 && s2) currentState = ENTRY_STAGE2;
      else if (!s1 && !s2) invalidSequence = true;
      break;

    case ENTRY_STAGE2:
      if (!s1 && s2) currentState = ENTRY_STAGE3;
      else if (s1 && !s2) invalidSequence = true;
      break;

    case ENTRY_STAGE3:
      if (s3 && !s4) currentState = ENTRY_STAGE4;
      else if (!s2) invalidSequence = true;
      break;

    case ENTRY_STAGE4:
      if (s3 && s4) currentState = ENTRY_STAGE5;
      else if (!s3) invalidSequence = true;
      break;

    case ENTRY_STAGE5:
      if (!s3 && s4) {
        Serial.println("Entrada confirmada!");
        if (totalSpaces > 0) totalSpaces--;
        sendToServer("ENTRADA", totalSpaces);
        currentState = IDLE;
      } else if (!s4) invalidSequence = true;
      break;

    // ================== SALIDA =====================
    case EXIT_STAGE1:
      if (s4 && s3) currentState = EXIT_STAGE2;
      else if (!s4) invalidSequence = true;
      break;

    case EXIT_STAGE2:
      if (!s4 && s3) currentState = EXIT_STAGE3;
      else if (s4 && !s3) invalidSequence = true;
      break;

    case EXIT_STAGE3:
      if (s2 && !s1) currentState = EXIT_STAGE4;
      else if (!s3) invalidSequence = true;
      break;

    case EXIT_STAGE4:
      if (s2 && s1) currentState = EXIT_STAGE5;
      else if (!s2) invalidSequence = true;
      break;

    case EXIT_STAGE5:
      if (!s2 && s1) {
        Serial.println("Salida confirmada!");
        if (totalSpaces < 11) totalSpaces++;
        sendToServer("SALIDA", totalSpaces);
        currentState = IDLE;
      } else if (!s1) invalidSequence = true;
      break;
  }

  // REINICIO POR SECUENCIA IMPOSIBLE
  if (invalidSequence) {
    Serial.println(">> Secuencia cancelada por patrón inválido");
    currentState = IDLE;
  }

  delay(120);
}
