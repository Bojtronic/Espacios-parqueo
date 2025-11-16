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
const unsigned long MIN_TIME = 300;  // tiempo mínimo de detección simultánea
const unsigned long STEP_TIMEOUT = 3000; // timeout para secuencias largas

// ESTADOS DE LA SECUENCIA
enum State {
  IDLE,
  ENTRY_STAGE1,      // S1 → S2
  ENTRY_STAGE2,      // ambos S1 y S2 activos
  ENTRY_STAGE3,      // S2 solo, S1 ya no
  ENTRY_STAGE4,      // S3 → S4
  ENTRY_STAGE5,      // solo S4 → confirmar entrada
  EXIT_STAGE1,       // S4 → S3
  EXIT_STAGE2,       // ambos S4 y S3 activos
  EXIT_STAGE3,       // solo S3
  EXIT_STAGE4,       // S2 → S1
  EXIT_STAGE5        // solo S1 → confirmar salida
};

State currentState = IDLE;

unsigned long t1 = 0, t2 = 0, t3 = 0, t4 = 0;
unsigned long stageStart = 0;

// LECTURA ULTRASONIDO
long getDistance(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  long dur = pulseIn(echo, HIGH, 30000);
  if(dur == 0) return 1000;
  return dur * 0.034 / 2;
}

bool s1, s2, s3, s4;

// -------------------------------------------------------------
void setup() {
  Serial.begin(115200);

  pinMode(TRIG1, OUTPUT); pinMode(ECHO1, INPUT);
  pinMode(TRIG2, OUTPUT); pinMode(ECHO2, INPUT);
  pinMode(TRIG3, OUTPUT); pinMode(ECHO3, INPUT);
  pinMode(TRIG4, OUTPUT); pinMode(ECHO4, INPUT);

  Serial.println("Sistema iniciado");
}

// -------------------------------------------------------------
void loop() {
  // Leer sensores
  s1 = (getDistance(TRIG1, ECHO1) < DETECT_DIST);
  s2 = (getDistance(TRIG2, ECHO2) < DETECT_DIST);
  s3 = (getDistance(TRIG3, ECHO3) < DETECT_DIST);
  s4 = (getDistance(TRIG4, ECHO4) < DETECT_DIST);

  unsigned long now = millis();

  // Timeout general
  if (currentState != IDLE && (now - stageStart > STEP_TIMEOUT)) {
    Serial.println(">> Secuencia cancelada por timeout");
    currentState = IDLE;
  }

  switch(currentState) {

    // *************************************
    // *************   IDLE   **************
    // *************************************
    case IDLE:
      if (s1 && !s2) { 
        currentState = ENTRY_STAGE1;
        stageStart = now;
        Serial.println("Entrada: S1 activo primero");
      }
      else if (s4 && !s3) {
        currentState = EXIT_STAGE1;
        stageStart = now;
        Serial.println("Salida: S4 activo primero");
      }
      break;

    // ================== ENTRADA =====================
    case ENTRY_STAGE1:  // S1 → S2
      if (s1 && s2) {
        currentState = ENTRY_STAGE2;
        stageStart = now;
        Serial.println("Entrada: S1 y S2 activos simultáneamente");
      }
      break;

    case ENTRY_STAGE2:  // ambos activos
      if (!s1 && s2) {
        currentState = ENTRY_STAGE3;
        stageStart = now;
        Serial.println("Entrada: S1 se libera, S2 sigue");
      }
      break;

    case ENTRY_STAGE3:  // S3 → S4
      if (s3 && !s4) {
        currentState = ENTRY_STAGE4;
        stageStart = now;
        Serial.println("Entrada: S3 activo");
      }
      break;

    case ENTRY_STAGE4:
      if (s3 && s4) {
        currentState = ENTRY_STAGE5;
        stageStart = now;
        Serial.println("Entrada: S3 y S4 activos simultáneamente");
      }
      break;

    case ENTRY_STAGE5:
      if (!s3 && s4) {
        Serial.println("Entrada: Confirmada → espacio -1");
        if (totalSpaces > 0) totalSpaces--;
        Serial.print("Espacios disponibles: ");
        Serial.println(totalSpaces);

        currentState = IDLE;
      }
      break;

    // ================== SALIDA =====================
    case EXIT_STAGE1:  // S4 → S3
      if (s4 && s3) {
        currentState = EXIT_STAGE2;
        stageStart = now;
        Serial.println("Salida: S4 y S3 simultáneos");
      }
      break;

    case EXIT_STAGE2:
      if (!s4 && s3) {
        currentState = EXIT_STAGE3;
        stageStart = now;
        Serial.println("Salida: S4 se libera, S3 sigue");
      }
      break;

    case EXIT_STAGE3:  // S2 → S1
      if (s2 && !s1) {
        currentState = EXIT_STAGE4;
        stageStart = now;
        Serial.println("Salida: S2 activo");
      }
      break;

    case EXIT_STAGE4:
      if (s2 && s1) {
        currentState = EXIT_STAGE5;
        stageStart = now;
        Serial.println("Salida: S2 y S1 simultáneos");
      }
      break;

    case EXIT_STAGE5:
      if (!s2 && s1) {
        Serial.println("Salida: Confirmada → espacio +1");
        if (totalSpaces < 11) totalSpaces++;
        Serial.print("Espacios disponibles: ");
        Serial.println(totalSpaces);

        currentState = IDLE;
      }
      break;
  }

  delay(120);
}
