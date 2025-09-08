#define TRIG1 16
#define ECHO1 36
#define TRIG2 17
#define ECHO2 39
#define TRIG3 18
#define ECHO3 34
#define TRIG4 23
#define ECHO4 35

int totalSpaces = 11;   // Espacios iniciales
const float DETECT_DIST = 10.0; // 10 cm
const unsigned long DETECT_TIME = 1000; // 1 segundo

// Estados para el control de secuencia
enum ParkingState {
  IDLE,
  ENTRY_DETECTED,
  ENTRY_CONFIRMED,
  EXIT_DETECTED,
  EXIT_CONFIRMED
};

ParkingState currentState = IDLE;

// Tiempos de detección
unsigned long entryDetectionTime = 0;
unsigned long exitDetectionTime = 0;
unsigned long lastStateChange = 0;

// Variables para seguimiento de detecciones
bool sensor1Active = false;
bool sensor2Active = false;
bool sensor3Active = false;
bool sensor4Active = false;

// Variable para llevar el control del último conteo mostrado
int lastDisplayedSpaces = 11;

// ------------------- Función para medir distancia -------------------
long getDistanceCM(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000); // 30 ms timeout
  if (duration == 0) {
    return 1000; // Valor alto si no hay detección
  }
  long distance = duration * 0.034 / 2;          // cm
  return distance;
}

// ------------------- Setup -------------------
void setup() {
  Serial.begin(115200);

  pinMode(TRIG1, OUTPUT); pinMode(ECHO1, INPUT);
  pinMode(TRIG2, OUTPUT); pinMode(ECHO2, INPUT);
  pinMode(TRIG3, OUTPUT); pinMode(ECHO3, INPUT);
  pinMode(TRIG4, OUTPUT); pinMode(ECHO4, INPUT);

  Serial.println("Sistema de parqueo iniciado");
  Serial.print("Espacios disponibles: ");
  Serial.println(totalSpaces);
}

// ------------------- Loop principal -------------------
void loop() {
  // Leer sensores
  long d1 = getDistanceCM(TRIG1, ECHO1);
  long d2 = getDistanceCM(TRIG2, ECHO2);
  long d3 = getDistanceCM(TRIG3, ECHO3);
  long d4 = getDistanceCM(TRIG4, ECHO4);

  unsigned long now = millis();

  // Actualizar estado de sensores
  sensor1Active = (d1 < DETECT_DIST);
  sensor2Active = (d2 < DETECT_DIST);
  sensor3Active = (d3 < DETECT_DIST);
  sensor4Active = (d4 < DETECT_DIST);

  // Máquina de estados para controlar el flujo de vehículos
  switch (currentState) {
    case IDLE:
      // Esperando detección en entrada o salida
      if (sensor1Active && sensor2Active) {
        entryDetectionTime = now;
        currentState = ENTRY_DETECTED;
        Serial.println(">> Vehículo detectado en ENTRADA (sensores 1 y 2)");
      } else if (sensor4Active && sensor3Active) {
        exitDetectionTime = now;
        currentState = EXIT_DETECTED;
        Serial.println(">> Vehículo detectado en SALIDA (sensores 4 y 3)");
      }
      break;

    case ENTRY_DETECTED:
      // Verificar que se mantenga la detección por al menos 1 segundo
      if (now - entryDetectionTime >= DETECT_TIME) {
        if (sensor3Active && sensor4Active) {
          currentState = ENTRY_CONFIRMED;
          Serial.println(">> Vehículo confirmado ingresando al parqueo (sensores 3 y 4)");
        } else if (!sensor1Active && !sensor2Active) {
          // El vehículo no continuó hacia el parqueo
          currentState = IDLE;
          Serial.println(">> Vehículo no ingresó al parqueo");
        }
      } else if (!sensor1Active || !sensor2Active) {
        // La detección se interrumpió antes del tiempo mínimo
        currentState = IDLE;
      }
      break;

    case ENTRY_CONFIRMED:
      // Esperar a que el vehículo complete su ingreso
      if (!sensor3Active && !sensor4Active) {
        // El vehículo ha ingresado completamente al parqueo
        if (totalSpaces > 0) {
          totalSpaces--;
        }
        // Mostrar espacios disponibles
        if (lastDisplayedSpaces != totalSpaces) {
          Serial.print("Auto ingresó. Espacios disponibles: ");
          Serial.println(totalSpaces);
          lastDisplayedSpaces = totalSpaces;
        }
        currentState = IDLE;
      }
      break;

    case EXIT_DETECTED:
      // Verificar que se mantenga la detección por al menos 1 segundo
      if (now - exitDetectionTime >= DETECT_TIME) {
        if (sensor2Active && sensor1Active) {
          currentState = EXIT_CONFIRMED;
          Serial.println(">> Vehículo confirmado saliendo del parqueo (sensores 2 y 1)");
        } else if (!sensor4Active && !sensor3Active) {
          // El vehículo no continuó hacia la salida
          currentState = IDLE;
          Serial.println(">> Vehículo no salió del parqueo");
        }
      } else if (!sensor4Active || !sensor3Active) {
        // La detección se interrumpió antes del tiempo mínimo
        currentState = IDLE;
      }
      break;

    case EXIT_CONFIRMED:
      // Esperar a que el vehículo complete su salida
      if (!sensor2Active && !sensor1Active) {
        // El vehículo ha salido completamente del parqueo
        if (totalSpaces < 11) {
          totalSpaces++;
        }
        // Mostrar espacios disponibles
        if (lastDisplayedSpaces != totalSpaces) {
          Serial.print("Auto salió. Espacios disponibles: ");
          Serial.println(totalSpaces);
          lastDisplayedSpaces = totalSpaces;
        }
        currentState = IDLE;
      }
      break;
  }

  // Pequeña pausa para evitar saturación
  delay(100);
}
