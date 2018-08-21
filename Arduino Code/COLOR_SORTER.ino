//***********LIBRERIAS SERVO************************************************
#include <Servo.h>

//**************LIBRERIAS SENSOR DE COLOR*******************************
#include "Wire.h"
#include "Adafruit_TCS34725.h"

//*******************LIBRERÍAS DEL BLUETOOTH**********************************
#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#include "BluefruitConfig.h"
#if SOFTWARE_SERIAL_AVAILABLE
  #include <SoftwareSerial.h>
#endif
#define FACTORYRESET_ENABLE         1
#define MINIMUM_FIRMWARE_VERSION    "0.6.6"
#define MODE_LED_BEHAVIOUR          "MODE"

// Instancia del bluetooth-----------------------------------------------------------
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);
// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

//Instancia Servos-------------------------------------------------------------------
Servo servoArriba;
Servo servoAbajo;

// Instancia sensor de Color---------------------------------------------------------------
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_1X);


//Variables globales -----------------------------------------------------------------
int colores[3][3]={};
int instruCode = 0;
String SendToApp="";
String InstReceive="";
int cantColores = 0;
int contC1=0;
int contC2=0;
int contC3=0;
int contCNone=0;

/*-------------------------------------------------------------------------------------
  SETUP
  -------------------------------------------------------------------------------------*/
void setup() {
  Serial.begin(115200);
  tcs.setInterrupt(true);  // turn off LED
  /*-------------------------------------------------------------------------------------
  BLUETOOTH
  -------------------------------------------------------------------------------------*/
  while (!Serial);  // required for Flora & Micro
  delay(500);
  
  Serial.println(F("Adafruit Bluefruit Command Mode Example"));
  Serial.println(F("---------------------------------------"));

  /* Initialise the module */
  Serial.print(F("Initialising the Bluefruit LE module: "));

  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println( F("OK!") );

  if ( FACTORYRESET_ENABLE )
  {
  /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset() ){
      error(F("Couldn't factory reset"));
    }
  }
  /* Disable command echo from Bluefruit */
  ble.echo(false);
  
  Serial.println("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  ble.info();

  Serial.println(F("Please use Adafruit Bluefruit LE app to connect in UART mode"));
  Serial.println(F("Then Enter characters to send to Bluefruit"));
  Serial.println();

  ble.verbose(false);  // debug info is a little annoying after this point!

  /* Wait for connection */
  while (! ble.isConnected()) {
      delay(500);
  }

  // LED Activity command is only supported from 0.6.6
  if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) )
  {
    // Change Mode LED Activity
    Serial.println(F("******************************"));
    Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
    ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
    Serial.println(F("******************************"));
  }  
/*-------------------------------------------------------------------------------------
  SERVOS
  -------------------------------------------------------------------------------------*/
 //Declarar pines para los servos
 servoArriba.attach(3); //Colocar en pin 3
 servoAbajo.attach(5); //Colocar en pin 5
 /*-------------------------------------------------------------------------------------
  SENSOR DE COLOR
  -------------------------------------------------------------------------------------*/
 // Verificar el sensor de color
 if (tcs.begin()) {
  Serial.println("LISTO");
 } else {
  Serial.println("TCS34725 No inicia");
  while (1); // Halt!
 }
}

/*-------------------------------------------------------------------------------------
  LOOP
  -------------------------------------------------------------------------------------*/
void loop() { 

  if(ble.isConnected()){    
        
    //Verificando si hay información en el buffer del bluetooth
    ble.println("AT+BLEUARTRX");
    ble.readline();   
     
    if ((strcmp(ble.buffer, "OK") == 0)) {
      if(instruCode==1){
        iniciar();
      return;
      }
      // No hay información
      return;
    }
    
    // Encontró información en el buffer    
    InstReceive = ble.buffer;    
    Serial.print("[Recv] "); 
    Serial.println(ble.buffer);
    ble.waitForOK();    

    //variable que almacena el tipo de instrucción recibida
    instruCode = InstReceive.substring(0,1).toInt();  
    
    if(instruCode==1){ //iniciar clasificación
      iniciar();
    }
    if(instruCode==2){
       cantColores = InstReceive.substring(2).toInt(); //cantidad de  colores a escanear
       escanear(cantColores);//Escanear la cantidad de colores recibida
       instruCode=0;
    }
    if(instruCode==3){
      reiniciar();//Reiniciar clasificacion
    }
    if(instruCode==0){
      detener();//Detener clasificacion
    }
  }  
}

/*-------------------------------------------------------------------------------------
  FUNCION Reiniciar (3)
  -------------------------------------------------------------------------------------*/

void reiniciar(){
  contC1=0;
  contC2=0;
  contC3=0;
  contCNone=0;
  instruCode=1;
}

/*-------------------------------------------------------------------------------------
  FUNCION INICIAR (1)
  -------------------------------------------------------------------------------------*/
void iniciar(){
    int rojo = 0; 
    int verde = 0;
    int azul = 0;
    int codColor=0; 
    String countColor;
    
    tcs.setInterrupt(false); // turn on LED

   //Sacar chicle de la cola
   servoArriba.write(171);
   delay(500);

   //Posicionar chicle debajo del sensor
   servoArriba.write(120);
   delay(500);
   
    uint16_t clear, red, green, blue;  

     //Sensar color
    delay(60);  // takes 50ms to read 
    tcs.getRawData(&red, &green, &blue, &clear);
    
    // Pasarlos entre 0 a 255
    uint32_t sum = clear/1.4;
    float r, g, b;
    r = red; r /= sum;
    g = green; g /= sum;
    b = blue; b /= sum;
    r *= 256; 
    g *= 256; 
    b *= 256;

    //Clasificar color, si los valores del arreglo colores (valores rgb) coinciden con algún color, almacena el codigo de ese color en una variable
    for(int i=0; i<3; i++){
      rojo = colores[i][0];
      verde = colores[i][1];
      azul = colores[i][2];      
      if ((r >= (rojo-15) && r <= (rojo+15)) && (g >= (verde-15) && g <= (verde+15)) && (b >= (azul-15) && b <= (azul+15))) {  
        codColor=i+1; //Código del color identificado
        break;
       }
    }

    //Según el color identificado, mueve la rampa hacia el tazon correspondiente y aumenta el contador de ese color
    //También se asigna el valor de un string que será enviado a la aplicación, por ejemplo: count c1 20
    switch(codColor){
      case 1:
        servoAbajo.write(35);
        contC1 +=1;
        countColor = "count c1 " + String(contC1);
        break;
      case 2:
        servoAbajo.write(71);
        contC2 +=1;
        countColor = "count c2 " + String(contC2);
        break;
      case 3:
        servoAbajo.write(107);
        contC3 +=1;
        countColor = "count c3 " + String(contC3);
        break;
      case 0:
        servoAbajo.write(143);
        contCNone +=1;
        countColor = "count cNone " + String(contCNone);
        break;      
    }

    //Imprime en el serial la lectura que hizo el sensor y el color que identificó
    Serial.print("\tR:\t"); Serial.print((int)r);
    Serial.print("\tG:\t"); Serial.print((int)g);
    Serial.print("\tB:\t"); Serial.print((int)b);  
    Serial.print("\tB:\t"); Serial.print(countColor);  
    Serial.println();  
      
    delay(600); 
    servoArriba.write(65);
    delay(500); 
    servoArriba.write(120);   

    //Envía a la aplicacion el color que identifica junto al contador del respectivo color
    Serial.print("[Send] ");
    Serial.println(countColor);
    ble.print("AT+BLEUARTTX=");
    ble.println(countColor);
    // check response stastus
    if (! ble.waitForOK() ) {
      Serial.println(F("Failed to send!!!"));
    }
 }

/*-------------------------------------------------------------------------------------
  FUNCIÓN DETENER(0)
  -------------------------------------------------------------------------------------*/
void detener(){
  tcs.setInterrupt(true);  // turn off LED
  servoArriba.write(65);
  delay(500); 
  servoArriba.write(120);
  servoAbajo.write(143);  
}

/*-------------------------------------------------------------------------------------
 FUNCIÓN ESCANEAR(2)
  -------------------------------------------------------------------------------------*/

void escanear(int n){
  
  tcs.setInterrupt(false);      // turn on LED
  
  String countColor;
  //Limpiando variables de conteo de colores
    contC1=0;
    contC2=0;
    contC3=0;
    contCNone=0;
    for(int m=0;m<3;m++){
      colores[m][0]=0;
      colores[m][1]=0;
      colores[m][2]=0;
    }
  //grados para el servo de la rampa
  int gradosServo = 35;
  
 for(int m=0;m<n;m++){
   //mover la rampa para que el chicle caiga en uno de los 3 tazones disponibles
   servoAbajo.write(gradosServo);
   
   //aumentar angulo de la rampa al siguiente tazon
   gradosServo += 36;
   delay(500); 

   //Mueve la plancha para sacar un chicle de la pila
   servoArriba.write(171);
   delay(1000);

   //Mueve la plancha hasta posicionar el chicle debajo del sensor de color
   servoArriba.write(120);
   delay(1000);

    //Variables para el sensor de color
    uint16_t clear, red, green, blue;
     
    //Ciclo para hacer 10 mediciones
    for(int i=0; i<10; i++){
      
      //Sensar color
      delay(60);  // takes 50ms to read 
      tcs.getRawData(&red, &green, &blue, &clear);
      
      //Pasar valores obtenidos a rgb(0-255)
      uint32_t sum = clear/1.4;
      float r, g, b;
      r = red; r /= sum;
      g = green; g /= sum;
      b = blue; b /= sum;
      r *= 256; g *= 256; b *= 256;

      //Asignar valores rgb a un arreglo
      colores[m][0]=(int)r;
      colores[m][1]=(int)g;
      colores[m][2]=(int)b;
      //imprimir en el serial los valores obtenidos en la lectura
      Serial.print("\tR:\t"); Serial.print((int)r);
      Serial.print("\tG:\t"); Serial.print((int)g);
      Serial.print("\tB:\t"); Serial.print((int)b);  
      Serial.println();        
     }  
      Serial.println(); 

      switch(m){
        case 0:
          contC1++;
          break;
        case 1:
          contC2++;
          break;
        case 2:
          contC3++;
          break;
      }

      //Ennviar el contador del progrma escaneado a la app
      countColor = "count c" + String(m+1) + " 1";
      Serial.print("[Send] ");
      Serial.println(countColor);
      ble.print("AT+BLEUARTTX=");
      ble.println(countColor);
      // check response stastus
      if (! ble.waitForOK() ) {
        Serial.println(F("Failed to send!!!"));
      }

    
      
      //Mover la plancha de arriba para liberar el chicle en la rampa
      servoArriba.write(70);
      delay(1000); 

      //Mover la plancha alineando con la pila de chicles
      servoArriba.write(171);  

      //Asignando el string que se envía a la aplicacion, por ejemplo: rgb c1 123 23 12     
      SendToApp = "rgb c" + String(m+1) + " " + colores[m][0] + " " + colores[m][1] + " " + colores[m][2];

      //Imprime en el serial lo que envia
      Serial.print("[Send] ");
      Serial.println(SendToApp);

      //Enviando a la aplicación el string 
      ble.print("AT+BLEUARTTX=");
      ble.println(SendToApp);
      // check response stastus
      if (! ble.waitForOK() ) {
        Serial.println(F("Failed to send!!!"));
      }
      
 }

  tcs.setInterrupt(true);  // turn off LED
  
 //Imprime los colores escaneados con sus respectivos valores rgb
 Serial.println("Arreglos-----------------------------------");
 for (int m=0;m<n;m++){  
  Serial.println(SendToApp);   
  servoArriba.write(120);    
 }  
}

