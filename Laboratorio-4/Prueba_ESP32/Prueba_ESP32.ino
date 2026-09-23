void setup() {

  Serial.begin(115200);

  delay(1000);

  Serial.println("=====================================================");
  Serial.println("PRUEBA DE ESP32 Francisco Jose Cardona Mejia 00119123");
  Serial.println("=====================================================");
}

void loop(){
Serial.print("ESP32 funcionando correctamente - Tiempo activo: ");
Serial.print(millis()/1000.0);
Serial.println(" segundos");

delay(2000);


}