#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

// =====================================================
// CONFIGURACIÓN WI-FI
// =====================================================

#define WLAN_SSID   "ARTEFACTOS"
#define WLAN_PASS   "87654321"

// =====================================================
// CONFIGURACIÓN ADAFRUIT IO
// =====================================================

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883

#define AIO_USERNAME    ""
#define AIO_KEY "TU_AIO_KEY"
// =====================================================
// PINES
// =====================================================

#define TRIG_PIN 18
#define ECHO_PIN 19

#define PIN_R 25
#define PIN_G 26
#define PIN_B 27

// =====================================================
// AJUSTES
// =====================================================

#define RGB_ANODO_COMUN false

#define PWM_FREQ 5000
#define PWM_RES  8

#define INTERVALO_PUBLICAR_MS 5000

#define DIST_CERCA_CM 10
#define DIST_MEDIA_CM 30

// =====================================================
// CLIENTE MQTT
// =====================================================

WiFiClient client;

Adafruit_MQTT_Client mqtt(
  &client,
  AIO_SERVER,
  AIO_SERVERPORT,
  AIO_USERNAME,
  AIO_KEY
);

// =====================================================
// FEEDS
// =====================================================

// Feed para publicar la distancia
Adafruit_MQTT_Publish feedDistancia =
  Adafruit_MQTT_Publish(
    &mqtt,
    AIO_USERNAME "/feeds/distancia"
  );

// Feed para recibir ON/OFF
Adafruit_MQTT_Subscribe feedBoton =
  Adafruit_MQTT_Subscribe(
    &mqtt,
    AIO_USERNAME "/feeds/boton-led"
  );

// =====================================================
// VARIABLES
// =====================================================

float ultimaDistancia = -1;

bool ledEncendido = true;

unsigned long ultimoPublicar = 0;
unsigned long ultimoPing = 0;

// =====================================================
// PROTOTIPOS
// =====================================================

void conectarWiFi();
void conectarMQTT();

float leerDistanciaCm();
float distanciaPromedio(int muestras);

void escribirRGB(uint8_t r, uint8_t g, uint8_t b);
void actualizarLED();

// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  delay(10);

  // ---------------- SENSOR ULTRASÓNICO ----------------

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // ---------------- LED RGB ----------------

  ledcAttach(PIN_R, PWM_FREQ, PWM_RES);
  ledcAttach(PIN_G, PWM_FREQ, PWM_RES);
  ledcAttach(PIN_B, PWM_FREQ, PWM_RES);

  // Empieza blanco
  escribirRGB(255, 255, 255);

  // ---------------- ADAFRUIT ----------------

  mqtt.subscribe(&feedBoton);

  // ---------------- WI-FI ----------------

  conectarWiFi();
}

// =====================================================
// LOOP
// =====================================================

void loop() {

  conectarMQTT();

  // ===================================================
  // LEER EL TOGGLE DE ADAFRUIT
  // ===================================================

  Adafruit_MQTT_Subscribe *sub;

  while ((sub = mqtt.readSubscription(200))) {

    if (sub == &feedBoton) {

      const char *msg =
        (char *)feedBoton.lastread;

      // Si recibe ON, se enciende
      // Si recibe OFF, se apaga

      ledEncendido =
        (strcmp(msg, "ON") == 0);

      Serial.print("boton-led: ");
      Serial.println(msg);

      actualizarLED();
    }
  }

  // ===================================================
  // LEER SENSOR Y PUBLICAR DISTANCIA
  // ===================================================

  if (millis() - ultimoPublicar >= INTERVALO_PUBLICAR_MS) {

    ultimoPublicar = millis();

    float d = distanciaPromedio(5);

    if (d > 0) {

      ultimaDistancia = d;

      Serial.print("Distancia: ");
      Serial.print(d, 1);
      Serial.println(" cm");

      if (!feedDistancia.publish(d)) {

        Serial.println(
          "Error al publicar la distancia"
        );
      }

    } else {

      Serial.println(
        "Lectura fuera de rango o sin eco"
      );
    }

    actualizarLED();
  }

  // ===================================================
  // MANTENER CONEXIÓN MQTT
  // ===================================================

  if (millis() - ultimoPing >= 30000) {

    ultimoPing = millis();

    mqtt.ping();
  }
}

// =====================================================
// SENSOR ULTRASÓNICO HC-SR04
// =====================================================

float leerDistanciaCm() {

  // Limpiar TRIG
  digitalWrite(TRIG_PIN, LOW);

  delayMicroseconds(2);

  // Pulso de 10 microsegundos
  digitalWrite(TRIG_PIN, HIGH);

  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  // Leer duración del eco
  long duracion =
    pulseIn(ECHO_PIN, HIGH, 30000);

  // Si no hubo eco
  if (duracion == 0) {

    return -1;
  }

  // Calcular distancia
  float distancia =
    (0.0343 * duracion) / 2;

  // Rango útil
  if (distancia < 2 ||
      distancia > 100) {

    return -1;
  }

  return distancia;
}

// =====================================================
// PROMEDIO DE DISTANCIAS
// =====================================================

float distanciaPromedio(int muestras) {

  float suma = 0;

  int validas = 0;

  for (int i = 0;
       i < muestras;
       i++) {

    float d = leerDistanciaCm();

    if (d > 0) {

      suma += d;

      validas++;
    }

    delay(40);
  }

  if (validas > 0) {

    return suma / validas;
  }

  return -1;
}

// =====================================================
// LED RGB
// =====================================================

void escribirRGB(
  uint8_t r,
  uint8_t g,
  uint8_t b
) {

  if (RGB_ANODO_COMUN) {

    r = 255 - r;

    g = 255 - g;

    b = 255 - b;
  }

  ledcWrite(PIN_R, r);

  ledcWrite(PIN_G, g);

  ledcWrite(PIN_B, b);
}

// =====================================================
// ACTUALIZAR COLOR DEL LED
// =====================================================

void actualizarLED() {

  // Si el Toggle está apagado
  if (!ledEncendido) {

    escribirRGB(0, 0, 0);

    return;
  }

  // Si no hay distancia válida
  if (ultimaDistancia <= 0) {

    escribirRGB(255, 255, 255);

    return;
  }

  // CERCA
  if (ultimaDistancia <= DIST_CERCA_CM) {

    escribirRGB(
      255,
      0,
      0
    );

  }

  // DISTANCIA MEDIA
  else if (
    ultimaDistancia <= DIST_MEDIA_CM
  ) {

    escribirRGB(
      255,
      255,
      0
    );

  }

  // LEJOS
  else {

    escribirRGB(
      0,
      255,
      0
    );
  }
}

// =====================================================
// CONEXIÓN WI-FI
// =====================================================

void conectarWiFi() {

  Serial.print(
    "Conectando a "
  );

  Serial.println(
    WLAN_SSID
  );

  WiFi.begin(
    WLAN_SSID,
    WLAN_PASS
  );

  while (
    WiFi.status() != WL_CONNECTED
  ) {

    delay(500);

    Serial.print(".");
  }

  Serial.println();

  Serial.print(
    "WiFi conectado. IP: "
  );

  Serial.println(
    WiFi.localIP()
  );
}

// =====================================================
// CONEXIÓN ADAFRUIT IO
// =====================================================

void conectarMQTT() {

  if (mqtt.connected()) {

    return;
  }

  Serial.print(
    "Conectando a Adafruit IO... "
  );

  int8_t ret;

  uint8_t intentos = 3;

  while (
    (ret = mqtt.connect()) != 0
  ) {

    Serial.println(
      mqtt.connectErrorString(ret)
    );

    Serial.println(
      "Reintentando en 5 segundos..."
    );

    mqtt.disconnect();

    delay(5000);

    intentos--;

    if (intentos == 0) {

      Serial.println(
        "No se pudo conectar. Reiniciando la ESP32..."
      );

      ESP.restart();
    }
  }

  Serial.println(
    "¡Conectado a Adafruit IO!"
  );
}