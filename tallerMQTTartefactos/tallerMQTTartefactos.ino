#include <WiFi.h>
#include <PubSubClient.h>
//libreria de dht11 que sirve para ahorrarnos las conversiones manuales
#include <DHT.h>

// ======================================================
// CONFIGURACIÓN WIFI
// ======================================================

//const char* WIFI_SSID = "********";
//const char* WIFI_PASSWORD = "********";

// ======================================================
// CONFIGURACIÓN MQTT
// ======================================================

//const char* MQTT_SERVER = "*********";
const int MQTT_PORT = 1883;

const char* TOPIC_TEMPERATURA = "clima/esp32/temperatura";
const char* TOPIC_HUMEDAD = "clima/esp32/humedad";
const char* TOPIC_LUZ = "clima/esp32/luz";

// ======================================================
// CONFIGURACIÓN DE SENSORES
// ======================================================

#define DHT_PIN 4
#define DHT_TYPE DHT11
#define LDR_PIN 34

//Declaración de la libreria, DHT es la clase de objeto 
//dht es el nombre del objeto a usar en este caso un dht11
//el parentesis es el constructor de la clase DHT.
DHT dht(DHT_PIN, DHT_TYPE);

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Enviar una lectura cada 2 segundos.
const unsigned long INTERVALO_ENVIO = 2000;
unsigned long ultimoEnvio = 0;

// ======================================================
// CONEXIÓN WIFI
// ======================================================

void conectarWiFi() {
  Serial.print("Conectando a WiFi");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi conectado");

  Serial.print("IP del ESP32: ");
  Serial.println(WiFi.localIP());
}

// ======================================================
// CONEXIÓN MQTT
// ======================================================

void conectarMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Conectando a Mosquitto...");

    String clientId = "ESP32-";
    clientId += String(
      (uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF),
      HEX
    );

    if (mqttClient.connect(clientId.c_str())) {
      Serial.println(" conectado");
    } else {
      Serial.print(" error, rc=");
      Serial.print(mqttClient.state());
      Serial.println(". Reintentando en 2 segundos...");

      delay(2000);
    }
  }
}

// ======================================================
// LEER Y PUBLICAR SENSORES
// ======================================================

void publicarSensores() {
  //dht es el objeto creado con anterioridad
  //readTemperature() = es la función de la librería DHT que lee el sensor y devuelve el valor de temperatura.
  float temperatura = dht.readTemperature();
  //mismo caso para leer humedad.
  float humedad = dht.readHumidity();

  // Leer el valor analógico de la LDR.
  int lecturaLuz = analogRead(LDR_PIN);

  // Comprobar si el DHT11 entregó lecturas válidas.
  //isnan(valor) Devuelve true si el valor que recibimos NO es un número válido.
  if (isnan(temperatura) || isnan(humedad)) {
    Serial.println("Error al leer el DHT11.");
    return;
  }

  //funcion map(valor, deMin, deMax, aMin, aMax) sirve para el reescalado de un valor a otro rango.
  //rescalado de dht a porcentaje.
  int porcentajeLuz = map(lecturaLuz, 0, 4095, 0, 100);
  //un seguro por si cualquier razon ocurre algo que cause que el porcentaje salga del 0-100 y lo recorta
  porcentajeLuz = constrain(porcentajeLuz, 0, 100);

  // Preparar los valores como texto, pues en MQTT y protocolo Websocket todo debe ser texto
  //puede ser de 16 a 20 los chars para mayor exactitud en MQTT
  char valorTemperatura[16];
  char valorHumedad[16];
  char valorLuz[16];

  //funcion parecida a sprintf pero más segura que define snpinft(destino, tamaño-array, formato, valor a convertir)
  //"%.1f" indica el formato de numero decimal, "%d" es para representar un numero entero decimal con signo.
  snprintf(valorTemperatura, sizeof(valorTemperatura), "%.1f", temperatura);

  snprintf(valorHumedad, sizeof(valorHumedad), "%.1f", humedad);

  snprintf(valorLuz, sizeof(valorLuz), "%d", porcentajeLuz);

  // Publicar cada dato en su topic MQTT.
  bool tempOK = mqttClient.publish(
    TOPIC_TEMPERATURA, valorTemperatura, true
  );

  bool humedadOK = mqttClient.publish(
    TOPIC_HUMEDAD, valorHumedad, true
  );

  bool luzOK = mqttClient.publish(
    TOPIC_LUZ, valorLuz, true
  );

  // Mostrar resultados en el Monitor Serial.
  Serial.println("\n----- LECTURAS DEL ESP32 -----");

  Serial.print("Temperatura: ");
  Serial.print(temperatura);
  Serial.println(" °C");

  Serial.print("Humedad: ");
  Serial.print(humedad);
  Serial.println(" %");

  Serial.print("Lectura LDR: ");
  Serial.print(lecturaLuz);
  Serial.print(" | Iluminación relativa: ");
  Serial.print(porcentajeLuz);
  Serial.println(" %");

  Serial.print("Publicación MQTT: ");
  if (tempOK && humedadOK && luzOK) {
    Serial.println("correcta");
  } else {
    Serial.println("uno o más datos no se pudieron publicar");
  }

  Serial.println("------------------------------");
}

// ======================================================
// SETUP
// ======================================================

void setup() {
  Serial.begin(115200);

  // Inicializar sensores.
  dht.begin();
  pinMode(LDR_PIN, INPUT);

  // Configurar resolución del ADC: 12 bits (0 a 4095).
  analogReadResolution(12);

  // Configurar la atenuación del ADC de la LDR.
  analogSetPinAttenuation(LDR_PIN, ADC_11db);

  // Conectar a WiFi y configurar MQTT.
  conectarWiFi();

  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setBufferSize(256);
}

// ======================================================
// LOOP
// ======================================================

void loop() {
  // Reconectar WiFi si se pierde la conexión.
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }

  // Reconectar MQTT si se pierde la conexión.
  if (!mqttClient.connected()) {
    conectarMQTT();
  }

  mqttClient.loop();

  // Publicar periódicamente.
  if (millis() - ultimoEnvio >= INTERVALO_ENVIO) {
    ultimoEnvio = millis();
    publicarSensores();
  }
}

