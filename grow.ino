// Grow program para monitorear temperatura humedad, luz en un indoor
// DHT22 humidity - temperature sensor
// Sensor de luz
#include "cactus_io_DHT22.h"
#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <Time.h>
#include <TimeLib.h>
#include <SD.h>

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7);

// Pin donde se conecta el DHT22 
// Humedad en el aire y temperatura
#define DHT22_PIN 2 
// Entrada para el sensor de luz
#define LIGHT_PIN_IN A1
// Entrada para el sensor de humedad del suelo
#define MOISTURE_PIN A0
// BACKLIGHT_PIN
#define BACKLIGHT_PIN     3
// Pin al que conectamos los bombillos
#define LIGHT_PIN_OUT  9

// Variable para guardar valor que viene del sensor de luz
int lightSensorValue = 0; 
// Variable para guardar valor que viene del sensor de 
// humedad del suelo
int moistureSensorValue = 0;

// Defino horas de luz Inicio a las 00:01 y termina a las 00:02
byte horaInicio = 13, minutoInicio = 11;
byte horaFin = 13, minutoFin = 12;
// Buffer para comandos leídos puerto serie
// 19 caracteres más retorno carro (CR) como fin de mensaje
// uno más por si acaso para el NULL fin de string
char cmdLeido[30]; 

// Texto en la pantalla
String texto_fila = "Tony Garcia GrowApp";
// Archivo de configuracion en el microsd
File configuration;

// Initialize DHT sensor for normal 16mhz Arduino. 
DHT22 dht(DHT22_PIN);
// Note: If you are using a board with a faster processor than 16MHz then you need
// to declare an instance of the DHT22 using 
// DHT22 dht(DHT22_DATA_PIN, 30);
// The additional parameter, in this case here is 30 is used to increase the number of
// cycles transitioning between bits on the data and clock lines. For the
// Arduino boards that run at 84MHz the value of 30 should be about right.
// Mac addres.
// Ip Address:
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte ip[] = { 192,168,1, 10 };

// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):
EthernetServer server = EthernetServer(80);
// Icono grados
byte grado[8] =
 {
    0b00001100,     // Los definimos como binarios 0bxxxxxxx
    0b00010010,
    0b00010010,
    0b00001100,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000
 };
// Icono de temperatura. 
byte temperatura[8] = {
        B00100,
        B01010,
        B01010,
        B01010,
        B01110,
        B11111,
        B11111,
        B01110
};
// Icono de humedad. 
byte humedad[8] = {
        B00100,
        B00100,
        B01010,
        B01010,
        B10001,
        B10001,
        B10001,
        B01110
};

// Icono humedad suelo
byte humedadSuelo[8] = {
    B00100,
    B01110,
    B01110,
    B11111,
    B11111,
    B11111,
    B01110,
    B11111
};


// Icono Luz
byte luz[8] = {
    B10101,
    B00000,
    B01110,
    B10001,
    B10001,
    B10001,
    B01010,
    B01110
};

// Icono Luz apagada
byte luzApagada[8] = {
    B00000,
    B00000,
    B01110,
    B11111,
    B11111,
    B11111,
    B01010,
    B01110
};

void setup() {
  
  lcd.begin (20,4,LCD_5x8DOTS);
  lcd.setBacklightPin(BACKLIGHT_PIN,POSITIVE); // init the backlight
  // Configuracion de Iconos
 
  lcd.createChar(1, grado);
  lcd.createChar(2, temperatura);
  lcd.createChar(3, humedad);
  lcd.createChar(4,humedadSuelo);
  lcd.createChar(5,luz);
  lcd.createChar(6,luzApagada);
  
  Serial.begin(9600); 
  Serial.print("Servidor escuchando Ip: ");
  Serial.println("DHT22 Humidity - Temperature Sensor");
  Serial.println("RH\t\tTemp (C)\tTemp (F)\tHeat Index (C)\t Heat Index (F)");
 
  // Configurando servidor
  Ethernet.begin(mac, ip);
  // Activando conexion y servidor web
  server.begin();
  // Inicializando sensor de humedad y temperatura
  dht.begin();
  
  pinMode (LIGHT_PIN_OUT, OUTPUT);
  //timeout para lectura de comandos
  Serial.setTimeout(1000); 

  //Pongo el reloj en la hora actual (en mi caso las 13:10:00 del 29/01/2017)
  //con setTime(hours, minutes, seconds, days, months, years);
  setTime(00, 00, 00, 29, 01, 2017);

  if (!SD.begin(4)){  
             Serial.println("ERROR - Fallo al inicar SD card!");     
             return;    // init failed
  }
  if (!SD.exists("configuration.txt")) { 
      configuration = SD.open("configuration.txt", FILE_WRITE);
      if (configuration){
      //Se escribe información en el documento de texto datos.txt.
      configuration.println("Esto es lo que se está escribiendo en el archivo");
      //Se cierra el archivo para almacenar los datos.
      configuration.close();
      }
  }
  
}

void loop() {
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  dht.readHumidity();
  dht.readTemperature();
  getMoistureData();
  getLightData();
  // Check if any reads failed and exit early (to try again).
  if (isnan(dht.humidity) || isnan(dht.temperature_C)) {
    Serial.println("DHT sensor fallo en la lectura!");
    return;
  }
  Serial.print(dht.humidity); Serial.print(" %\t\t");
  Serial.print(dht.temperature_C); Serial.print(" *C\t");
  Serial.print(dht.temperature_F); Serial.print(" *F\t");
  Serial.print(dht.computeHeatIndex_C()); Serial.print(" *C\t");
  Serial.print(dht.computeHeatIndex_F()); Serial.println(" *F");

  //Escuchando para conexiones con clientes
  EthernetClient client = server.available();      // assign any newly connected Web Browsers to the "client" variable.
  
  if(client.connected()){
    Serial.println("Client Connected");
    
    while(client.available()){
      //Serial.write(client.read());               // Uncomment if you want to write the request from the Browser (CLIENT) to the SERIAL MONITOR (and comment out the next line)
      client.read();                               // This line will clear the communication buffer between the client and the server.
    }
    
    //Send the Server response header back to the browser.
    client.println("HTTP/1.1 200 OK");           // This tells the browser that the request to provide data was accepted
    client.println("Access-Control-Allow-Origin: *");  //Tells the browser it has accepted its request for data from a different domain (origin).
    client.println("Content-Type: application/json;charset=utf-8");  //Lets the browser know that the data will be in a JSON format
    client.println("Server: Arduino");           // The data is coming from an Arduino Web Server (this line can be omitted)
    client.println("Connection: close");         // Will close the connection at the end of data transmission.
    client.println();                            // You need to include this blank line - it tells the browser that it has reached the end of the Server reponse header.
    
    //Transmit the Analog Readings to the Web Browser in JSON format
    //Example Transmission: [{"key":0, "value":300},{"key":1, "value":320},{"key":2, "value":143},{"key":3, "value":24},{"key":4, "value":760},{"key":5, "value":470}]
      client.print("[");                           
      client.print("{\"key\": ");
      client.print(0);  
      client.print(", \"value\": ");
      client.print(dht.temperature_C);
      client.print("},");
      client.print("{\"key\": ");
      client.print(1);  
      client.print(", \"value\": ");
      client.print(dht.humidity);
      client.print("},"); 
      client.print("{\"key\": ");
      client.print(2);  
      client.print(", \"value\": ");
      client.print(lightSensorValue);
      client.print("},"); 
      client.print("{\"key\": ");
      client.print(3);  
      client.print(", \"value\": ");
      client.print(moistureSensorValue);
      client.print("}");
      
      client.println("]");                         // LLave final del dato JSON 
      client.stop();                                        // This method terminates the connection to the client
      Serial.println("Cliente se ha desconectado");         // Print the message to the Serial monitor to indicate that the client connection has closed.
  }
  // hay caracteres recibidos en puerto serie
  if (Serial.available() > 0) 
      processMsg();
  //Muestro por el puerto serie la hora
  Serial.print("Son las: ");
  Serial.print(String(hour()) + ":" + String(minute()) + ":" +  String(second()) + " Inicio: " + String(horaInicio) + ':' + String(minutoInicio) + " Fin: " + String(horaFin) + ':' + String(minutoFin));
  // Comprobamos si es la hora del riego 
  if(esHora()){ 
    // Es hora de encender la luz: informo por serie
    Serial.println(" --> Es hora enceder luz!!"); 
    // Activo Luz
    digitalWrite (LIGHT_PIN_OUT, HIGH);
  }
  else {
    // No es hora de enceder luz: informo por serie
    Serial.println(" --> No es hora encender luz!!");
    // Apago Luz
    digitalWrite (LIGHT_PIN_OUT, LOW); // cierro la EV
  }

  printScreen();
  delay(1000);
}

void printScreen(){
  
  // clear display, set cursor position to zero
  lcd.clear(); 
  // Backlight on
  lcd.setBacklight(HIGH);     
  // go home  
  lcd.home ();
  // Print Temperature                      
  //lcd.print("Temp: "); 
  lcd.write(2);
  lcd.setCursor ( 2, 0 ); 
  lcd.print(dht.temperature_C); 
  lcd.setCursor ( 7, 0 ); 
  lcd.write(1);
  lcd.setCursor ( 8, 0 ); 
  lcd.write("C");  

  // Print humedad del aire
  lcd.setCursor ( 11, 0 ); 
  lcd.write(3); 
  lcd.setCursor ( 13, 0 ); 
  lcd.print(dht.humidity); 

  lcd.setCursor ( 19, 0 );
  lcd.write("%");  

  // Print humedad del suelo
  lcd.setCursor ( 0, 1 );
  lcd.write(4); 
  lcd.setCursor (2, 1 );
  lcd.print(moistureSensorValue);
  lcd.setCursor ( 5, 1 );
  lcd.write("%"); 

  if(moistureSensorValue < 60){
    lcd.setCursor ( 0, 2 );
    lcd.print("Revise el riego");
  }
  else
  {
    lcd.setCursor ( 0, 2 );
    lcd.print("Revise el riego"); 
  }
  // Print iconos de acciones Acciones
  // Imprime icono de luz a la hora de encendido
  if(esHora()){
    lcd.setCursor ( 11, 1 );
    lcd.write(5);
  }
  else {
    lcd.setCursor ( 11, 1 );
    lcd.write(6); 
  }

  // Obtenemos el tamaño del texto
  int tam_texto = texto_fila.length();
 
  // Mostramos entrada texto por la izquierda
  for(int i = tam_texto ; i>0 ; i--)
  {
    String texto = texto_fila.substring(i-1);
    lcd.setCursor(0, 3);
    // Limpiamos pantalla
    lcd.print("                    ");
    //Situamos el cursor
    lcd.setCursor(0, 3);
    // Escribimos el texto
    lcd.print(texto);
    // Esperamos
    delay(100);
  }
}

void getMoistureData(){
  // read the input on analog pin 1:
   float valor = 0;
   valor = analogRead(MOISTURE_PIN);
   moistureSensorValue  = map(valor, 0, 1023, 100, 0);
}

void getLightData(){
  // Leyendo valor desde el sensor de luz
  lightSensorValue = analogRead(LIGHT_PIN_IN); 
  // Imprimiendo valor en la salida serial
  Serial.println(lightSensorValue); 
}

// Programacion de horario de encendido encendido de luz
void processMsg()
{
// Leer comando
Serial.readBytesUntil('\r', cmdLeido, 30);
// Primer carácter del comando
switch (cmdLeido[0]) { 
// Setear  hora
    case 'S':
    Serial.println("Hora actualizada!");
    // Para poner en hora resto el ascii del carácter '0' para obtener el número decimal
    // (por ejemplo, si recibo '2' (ascii 50) le resto '0' (ascii 48) y obtengo 50-48=2)
    setTime((cmdLeido[1] - '0')* 10 + (cmdLeido[2]-'0'), //horas
    (cmdLeido[4]-'0')*10+(cmdLeido[5]-'0'), //minutos
    (cmdLeido[7]-'0')*10+(cmdLeido[8]-'0'), //segundos
    (cmdLeido[10]-'0')*10+(cmdLeido[11]-'0'), //día
    (cmdLeido[13]-'0')*10+(cmdLeido[14]-'0'), //mes
    (cmdLeido[16]-'0')*1000+(cmdLeido[17]-'0')*100+
    (cmdLeido[18]-'0')*10+(cmdLeido[19]-'0')); //año
    break;


    // Programar hora de inicio
    case 'I':
    horaInicio = (cmdLeido[1]-'0')*10+(cmdLeido[2]-'0');
    minutoInicio = (cmdLeido[4]-'0')*10+(cmdLeido[5]-'0');
    Serial.println("Programada hora inicio!");
    break;

    // Programar hora de (F)in
    case 'F':
    horaFin = (cmdLeido[1]-'0')*10+(cmdLeido[2]-'0');
    minutoFin = (cmdLeido[4]-'0')*10+(cmdLeido[5]-'0');
    Serial.println("Programada hora fin!");
    break;

}

}

// Verificar si es hora de encendido
boolean esHora(){

// Para hacer las comparaciones de cuando empezar y cuando terminar,lo paso todo a minutos.
// Por eso momentoInicio = 13*60 + 11 = 791 y de fin = 13*60 + 12 = 792
// También se podría hacer una doble comparación de horaActual con horaInicio y horaFin
// y de minutoActual con minutoInicio y minutoFin

int momentoInicio = (horaInicio * 60) + minutoInicio;
int momentoFin = (horaFin * 60) + minutoFin;
int momentoAhora = (hour() * 60) + minute();
// Esto es que si hemos pasado o estamos en el momento de inicio , pero antes del momento del fin...


// Estamos en período de encendido: devolver “true”
if((momentoInicio<=momentoAhora) && (momentoAhora<momentoFin)){
   return true; 
}
else {
  // No estamos en período de encender: devolver “false”
   return false; 
}
}







