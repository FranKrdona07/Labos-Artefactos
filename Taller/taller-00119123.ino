#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// ======================================================
// CONFIGURACIÓN WIFI
// ======================================================


const char* WIFI_SSID = "iPhone de Fran";
const char* WIFI_PASSWORD = "ELOTELOCO";

// ======================================================
// CONFIGURACIÓN MQTT
// ======================================================


const char* MQTT_SERVER = "172.20.10.5";
const int MQTT_PORT = 1883;

// Topics utilizados por el dashboard React.
const char* TOPIC_TEMPERATURA = "clima/esp32/temperatura";
const char* TOPIC_HUMEDAD = "clima/esp32/humedad";
const char* TOPIC_LUZ = "clima/esp32/luz";

// ======================================================
// CONFIGURACIÓN DE SENSORES
// ======================================================

// (Estudiante)
// Definir los pines y objetos necesarios para los sensores.


#define DHTPIN 4
#define DHTTYPE DHT11

const int LDR_PIN = 32;

DHT dht(DHTPIN, DHTTYPE);

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Enviar una lectura cada 2 segundos.
const unsigned long INTERVALO_ENVIO = 2000;

unsigned long ultimoEnvio = 0;

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

void conectarMQTT() {

  while (!mqttClient.connected()) {

    Serial.print("Conectando a Mosquitto...");

    // Crear un Client ID diferente para este ESP32.
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
  float temperatura = dht.readTemperature();
  float humedad = dht.readHumidity();

  int lecturaLDR = analogRead(LDR_PIN);
  int luz = map(lecturaLDR, 0, 4095, 0, 100);
  luz = constrain(luz, 0, 100);

  if (isnan(temperatura) || isnan(humedad)) {
    return;
  }

  char tempTexto[10];
  char humedadTexto[10];
  char luzTexto[10];

  dtostrf(temperatura, 1, 2, tempTexto);
  dtostrf(humedad, 1, 2, humedadTexto);
  itoa(luz, luzTexto, 10);

  mqttClient.publish(TOPIC_TEMPERATURA, tempTexto, true);
  mqttClient.publish(TOPIC_HUMEDAD, humedadTexto, true);
  mqttClient.publish(TOPIC_LUZ, luzTexto, true);

  Serial.print("Temperatura: ");
  Serial.print(temperatura);
  Serial.println(" C");

  Serial.print("Humedad: ");
  Serial.print(humedad);
  Serial.println(" %");

  Serial.print("LDR ADC: ");
  Serial.print(lecturaLDR);
  Serial.print(" | Luz: ");
  Serial.print(luz);
  Serial.println(" %");

  Serial.println("------------------------------");
}

void setup() {
  Serial.begin(115200);

  dht.begin();
  pinMode(LDR_PIN, INPUT);

  delay(2000);

  conectarWiFi();

  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }

  if (!mqttClient.connected()) {
    conectarMQTT();
  }

  mqttClient.loop();

  if (millis() - ultimoEnvio >= INTERVALO_ENVIO) {
    ultimoEnvio = millis();
    publicarSensores();
  }
}