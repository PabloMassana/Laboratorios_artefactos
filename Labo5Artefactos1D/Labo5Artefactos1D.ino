#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

// ---------------------- CONFIGURACIÓN WI-FI ----------------------


// ---------------------- CONFIGURACIÓN ADAFRUIT IO ----------------------
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883

// ---------------------- PINES ----------------------
#define TRIG_PIN 5
#define ECHO_PIN 18
#define PIN_R 25
#define PIN_G 26
#define PIN_B 27

// ---------------------- AJUSTES ----------------------
#define RGB_ANODO_COMUN  false
#define PWM_FREQ         5000
#define PWM_RES          8
#define INTERVALO_PUBLICAR_MS 5000
#define DIST_CERCA_CM    20
#define DIST_MEDIA_CM    50

// ---------------------- CLIENTE MQTT Y FEEDS ----------------------
WiFiClient client;

Adafruit_MQTT_Client mqtt(
  &client,
  AIO_SERVER,
  AIO_SERVERPORT,
  AIO_USERNAME,
  AIO_KEY
);

// Feed para PUBLICAR la distancia
Adafruit_MQTT_Publish feedDistancia =
  Adafruit_MQTT_Publish(
    &mqtt,
    AIO_USERNAME "/feeds/distancia"
  );

// Feed para RECIBIR el estado del LED
Adafruit_MQTT_Subscribe feedLedControl =
  Adafruit_MQTT_Subscribe(
    &mqtt,
    AIO_USERNAME "/feeds/led-control"
  );

float ultimaDistancia = -1;

unsigned long ultimoPublicar = 0;
unsigned long ultimoPing = 0;

bool ledHabilitado = true;

void conectarWiFi();
void conectarMQTT();

float leerDistanciaCm();
float distanciaPromedio(int muestras);

void escribirRGB(uint8_t r, uint8_t g, uint8_t b);
void actualizarLED();

//  SETUP
void setup() {

  Serial.begin(115200);
  delay(10);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  ledcAttach(PIN_R, PWM_FREQ, PWM_RES);
  ledcAttach(PIN_G, PWM_FREQ, PWM_RES);
  ledcAttach(PIN_B, PWM_FREQ, PWM_RES);

  escribirRGB(255, 255, 255);

  conectarWiFi();

  // El ESP32 recibirá los cambios realizados
  mqtt.subscribe(&feedLedControl);
}

//  LOOP
void loop() {

  // Mantener conexión MQTT
  conectarMQTT();

  // RECIBIR CAMBIOS DEL TOGGLE DE ADAFRUIT IO
  Adafruit_MQTT_Subscribe *subscription;

  while ((subscription = mqtt.readSubscription(200)) != NULL) {

    if (subscription == &feedLedControl) {

      String estado = (char *)feedLedControl.lastread;

      Serial.print("Estado recibido desde Adafruit IO: ");
      Serial.println(estado);

      // Si el Toggle está en ON
      if (estado == "ON" || estado == "on" || estado == "1") {

        ledHabilitado = true;

        Serial.println("LED RGB: ENCENDIDO");

        // Volver a aplicar el color correspondiente
        actualizarLED();
      }

      // Si el Toggle está en OFF
      else if (estado == "OFF" || estado == "off" || estado == "0") {

        ledHabilitado = false;

        // Apagar completamente el RGB
        escribirRGB(0, 0, 0);

        Serial.println("LED RGB: APAGADO");
      }
    }
  }

  // LEER EL ULTRASÓNICO Y PUBLICAR CADA CIERTO TIEMPO
  if (millis() - ultimoPublicar >= INTERVALO_PUBLICAR_MS) {

    ultimoPublicar = millis();

    float d = distanciaPromedio(5);

    if (d > 0) {

      ultimaDistancia = d;

      Serial.print("Distancia: ");
      Serial.print(d, 1);
      Serial.println(" cm");

      // Publicar distancia
      if (!feedDistancia.publish(d)) {

        Serial.println("Error al publicar la distancia");

      } else {

        Serial.println("Distancia publicada correctamente");
      }

    } else {

      Serial.println("Lectura fuera de rango o sin eco");
    }

    // Actualizar LED según distancia
    // Solo tendrá efecto si ledHabilitado == true.
    actualizarLED();
  }

  // MANTENER VIVA LA CONEXIÓN MQTT
  if (millis() - ultimoPing >= 30000) {

    ultimoPing = millis();

    mqtt.ping();
  }
}

//  ULTRASÓNICO HC-SR04
float leerDistanciaCm() {

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long duracion = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duracion == 0) {
    return -1;
  }

  float distancia = (duracion * 0.0343) / 2;

  // Rango útil del sensor: 2 a 100 cm
  if (distancia < 2 || distancia > 100) {
    return -1;
  }

  return distancia;
}

//  PROMEDIO DE DISTANCIA
float distanciaPromedio(int muestras) {

  float suma = 0;
  int validas = 0;

  for (int i = 0; i < muestras; i++) {

    float d = leerDistanciaCm();

    if (d > 0) {

      suma += d;
      validas++;
    }

    delay(40);
  }

  return (validas > 0) ? suma / validas : -1;
}

//  LED RGB
void escribirRGB(uint8_t r, uint8_t g, uint8_t b) {

  if (RGB_ANODO_COMUN) {

    r = 255 - r;
    g = 255 - g;
    b = 255 - b;
  }

  ledcWrite(PIN_R, r);
  ledcWrite(PIN_G, g);
  ledcWrite(PIN_B, b);
}

//  ACTUALIZAR LED SEGÚN DISTANCIA
void actualizarLED() {

  // SI EL TOGGLE ESTÁ EN OFF
  if (!ledHabilitado) {

    escribirRGB(0, 0, 0);

    return;
  }

  // SIN DISTANCIA VÁLIDA
  if (ultimaDistancia <= 0) {

    escribirRGB(255, 255, 255);

    return;
  }

  // CERCA = ROJO
  if (ultimaDistancia <= DIST_CERCA_CM) {

    escribirRGB(255, 0, 0);

    Serial.println("Color: ROJO");
  }

  // DISTANCIA MEDIA = AMARILLO
  else if (ultimaDistancia <= DIST_MEDIA_CM) {

    escribirRGB(255, 255, 0);

    Serial.println("Color: AMARILLO");
  }

  // LEJOS = VERDE
  else {

    escribirRGB(0, 255, 0);

    Serial.println("Color: VERDE");
  }
}

//  CONEXIÓN WI-FI
void conectarWiFi() {

  Serial.print("Conectando a ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);

    Serial.print(".");
  }

  Serial.println();

  Serial.print("WiFi conectado. IP: ");
  Serial.println(WiFi.localIP());
}

//  CONEXIÓN MQTT
void conectarMQTT() {

  if (mqtt.connected()) {
    return;
  }

  Serial.print("Conectando a Adafruit IO... ");

  int8_t ret;
  uint8_t intentos = 3;

  while ((ret = mqtt.connect()) != 0) {

    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Reintentando en 5 segundos...");

    mqtt.disconnect();

    delay(5000);

    if (--intentos == 0) {

      Serial.println("No se pudo conectar. Reiniciando la ESP32...");

      ESP.restart();
    }
  }

  Serial.println("¡Conectado a Adafruit IO!");

  mqtt.subscribe(&feedLedControl);

  Serial.println("Suscrito al feed led-control");
}