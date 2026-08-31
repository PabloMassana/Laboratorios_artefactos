#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

int pinLM35 = A0;
int dot = 1000;

#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_RESET -1

Adafruit_SH1106G display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

int lecturaLM35;
float voltaje;
float temperatura;

void setup() {

  Serial.begin(9600);

  if (!display.begin(0x3C, true)) {
    Serial.println("No se encontro la OLED");
    while (1);
  }

  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);

  display.println("Sensor LM35");
  display.println("Iniciando...");
  display.display();

  delay(dot);
}

void loop() {

  lecturaLM35 = analogRead(pinLM35);

  voltaje = lecturaLM35 * (5.0 / 1023.0);

  temperatura = voltaje * 100.0;

  Serial.print("Lectura ADC: ");
  Serial.println(lecturaLM35);

  Serial.print("Voltaje: ");
  Serial.print(voltaje, 3);
  Serial.println(" V");

  Serial.print("Temperatura: ");
  Serial.print(temperatura, 1);
  Serial.println(" C");

  Serial.println("--------------------");

  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("TEMPERATURA LM35");

  display.setTextSize(2);
  display.setCursor(0, 25);
  display.print(temperatura, 1);
  display.println(" C");

  display.display();

  delay(dot);
}