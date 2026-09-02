#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define I2C_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define LM35_PIN A0

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

float leerTemperaturaC() {
  long suma = 0;

  for (byte i = 0; i < 10; i++) {
    suma += analogRead(LM35_PIN);
    delay(10);
  }

  float lecturaPromedio = suma / 10.0;
  float voltaje = lecturaPromedio * (5.0 / 1024.0);
  return voltaje * 100.0;
}

void mostrarTemperatura(float temperaturaC) {
  display.clearDisplay();

  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Sensor LM35");

  display.drawLine(0, 12, 127, 12, SH110X_WHITE);

  display.setTextSize(3);
  display.setCursor(4, 24);
  display.print(temperaturaC, 1);

  display.setTextSize(2);
  display.setCursor(95, 29);
  display.print((char)247);
  display.print("C");

  display.fillRect(0, 54, 128, 10, SH110X_WHITE);
  display.setTextColor(SH110X_BLACK);
  display.setTextSize(1);
  display.setCursor(15, 56);
  display.print("Temperatura actual");

  display.display();
}

void setup() {
  Serial.begin(9600);
  Wire.begin();
  delay(250);

  if (!display.begin(I2C_ADDRESS, true)) {
    Serial.println("ERROR: OLED no encontrada");
    while (1);
  }

  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.println("OLED funcionando");
  display.setCursor(0, 34);
  display.println("Leyendo LM35...");
  display.display();
  delay(1200);
}

void loop() {
  float temperaturaC = leerTemperaturaC();

  Serial.print("Temperatura: ");
  Serial.print(temperaturaC, 1);
  Serial.println(" °C");

  mostrarTemperatura(temperaturaC);
  delay(1000);
}