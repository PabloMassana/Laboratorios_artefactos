// ==========================================
// HC-SR04 + ESP32
// TRIG -> GPIO 5
// ECHO -> GPIO 18
// ==========================================

#define TRIG_PIN 5
#define ECHO_PIN 18

void setup() {

  // Iniciar comunicación serial
  Serial.begin(115200);

  // Configurar pines del sensor
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // TRIG comienza apagado
  digitalWrite(TRIG_PIN, LOW);

  Serial.println();
  Serial.println("================================");
  Serial.println("       HC-SR04 + ESP32");
  Serial.println("================================");
}

void loop() {

  // 1. Generar pulso ultrasónico
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  // 2. Medir duración del eco
  // Timeout de 30 ms
  unsigned long duracion =
      pulseIn(ECHO_PIN, HIGH, 30000);

  // Comprobar si hubo eco
  if (duracion == 0) {

    Serial.println("No se detecto eco");

  } else {

    // Calcular distancia en centímetros
    float distancia =
        (duracion * 0.0343) / 2.0;

    // Mostrar resultado
    Serial.print("Duracion: ");
    Serial.print(duracion);
    Serial.print(" us");

    Serial.print(" | Distancia: ");
    Serial.print(distancia, 2);
    Serial.println(" cm");
  }

  delay(500);
}