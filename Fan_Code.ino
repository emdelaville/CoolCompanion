// Libraries needed
#include <Arduino.h>
#include <list> // For std::list
#include <numeric> // for std::accumulate
#include <Preferences.h> // For storing data into flash
#include <Wire.h>
#include "WiFi.h"
#include <esp_now.h>


//Display Initalization//----------------------
#include <Adafruit_GFX.h>     // Core graphics library
#include <Adafruit_ST7735.h>  // Hardware-specific library for ST7735
#include <Adafruit_ST7789.h>  // Hardware-specific library for ST7789
#include <SPI.h>
#include <stdlib.h>
#include <stdio.h>

#if defined(ARDUINO_FEATHER_ESP32)  // Feather Huzzah32
#define TFT_CS 14
#define TFT_RST 15
#define TFT_DC 32

#elif defined(ESP8266)
#define TFT_CS 4
#define TFT_RST 16
#define TFT_DC 5

#else
// For the breakout board, you can use any 2 or 3 pins.
// These pins will also work for the 1.8" TFT shield.
//define ARDUINO pins
#define TFT_CS 10
#define TFT_RST 9  // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC 8
#endif

// include fonts
#include <Fonts/FreeMonoBoldOblique12pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>

#define AVG_AMOUNT 60 //defines how many readings to average
#define TEMP_F_MODE 0
#define TEMP_C_MODE 1

//declaere display
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);


//function prototypes
float rollingAvgTemp(float temperatureReading, int second_counter);
float rollingAvgHum(float humidityReading, int second_counter);
void printSensorReadings(float temperature, float humidity, int mode);
//Display Initalization end //----------------------

// Define pins
#define ENA A7
#define IN1 D3
#define IN2 D4

// Define Constants
// #define padMacAddr "EC:DA:3B:63:AE:80"
// #define fanMacAddr "34:85:18:7B:C4:24"
#define displayWidth 320
#define displayHeight 240

uint8_t padMacAddr[] = {0xEC, 0xDA, 0x3B, 0x63, 0xAE, 0x80};
uint8_t fanMacAddr[] = {0x34, 0x85, 0x18, 0x7B, 0xC4, 0x24};

// Create Timers
unsigned long timer;

// Any Enums
enum SystemState {WAKE_UP, ACTIVE, SLEEP} systemState;
enum FanState {OFF, ONE, TWO, THREE} fanState;

typedef struct message{
  float temp;
  float hum;
} message;

message incomingReadings;// = (message*)malloc(sizeof(message));
message outgoingData;
float incomingTemp;
float incomingHum;
esp_now_peer_info_t peerInfo;

void dataSent(const uint8_t* mac_addr, esp_now_send_status_t status){
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
void dataRecieved(const uint8_t* mac, const uint8_t* incomingData, int len){
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  Serial.print("Bytes received: ");
  Serial.println(len);
  incomingTemp = incomingReadings.temp;
  incomingHum = incomingReadings.hum;
}

void setup() {
  // put your setup code here, to run once:

  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  while(!Serial);
  WiFi.mode(WIFI_MODE_STA);
  Serial.print("MAC Address = " + String(WiFi.macAddress()));

  if(esp_now_init() != ESP_OK){
    Serial.print("Error initializing ESP-NOW");
  }
  esp_now_register_send_cb(dataSent);

  memcpy(peerInfo.peer_addr, padMacAddr, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK)
    Serial.print("Failed to add peer");
  esp_now_register_recv_cb(dataRecieved);


  systemState = WAKE_UP;
  fanState = OFF;

  //pinMode();
  pinMode(A7, OUTPUT);
  pinMode(D3, OUTPUT);
  pinMode(D4, OUTPUT);

  digitalWrite(D3, LOW);
  digitalWrite(D4, LOW);


  //register a callback function to recieve ESPNOW transmissions
  

  //Display Setup
  tft.init(240, 320);  // Init ST7789 320x240
  Serial.println(F("Initialized"));
  tft.setRotation(3);
  //set background to black
  tft.fillScreen(ST77XX_BLACK);
}

void loop() {
  // put your main code here, to run repeatedly:

  // outgoingData.temp = 70.0;
  // outgoingData.hum = 20.0;

  // esp_err_t result = esp_now_send(padMacAddr, (uint8_t *) &outgoingData, sizeof(outgoingData));
   
  // if (result == ESP_OK) {
  //   Serial.println("Sent with success");
  // }
  // else {
  //   Serial.println("Error sending the data");
  // }
  // delay(10000);
  // Wire.requestFrom(0x38, 1);
  // data = Wire.read();
  //systemStateMachine();

  // int temp_second_flag = 0;
  // int hum_second_flag = 0;
  int second_flag =0;
  long unsigned int second_counter =0;
  float hum_reading;
  float temp_reading;

  int mode = TEMP_F_MODE;

  float AvgHumidity;
  float AvgTemperatureC;

  if(millis() % 1000 == 1)
  { //every second
    second_flag = 1; //raise flag to signal average
    second_counter++;
  }
  mode = TEMP_C_MODE;

  if(second_flag == 1){
    switch(mode)
      {
        case TEMP_F_MODE:
          //printSensorReadings(rollingAvgHum(hum_reading, second_counter), rollingAvgTemp(temp_reading, second_counter), mode);
          printSensorReadings(incomingTemp, incomingHum, mode);
          updateFan(incomingTemp, incomingHum, mode);
          break;
        case TEMP_C_MODE:
          printSensorReadings(incomingTemp, incomingHum, mode);
          updateFan(incomingTemp, incomingHum, mode);
          break;
      }
      second_flag = 0;
  }

}

void printSensorReadings(float temperature, float humidity, int mode) //-------------------------------------------------------------------------------------
{ 
  //if temperature reads below one digit, place a black box after temperature (may not be needed)
  if(temperature < 10){
    //tft.fillRect(); //print black box where value will be. 
  }

  //if humidity reads below one digit, place a black box after temperature
  if(humidity < 10){
    //tft.fillRect(); //print black box where value will be. 
  }

  //tft.drawRectangle();
  //draw bounding boxes around values
  // tft.fillRoundRect(25, 10, 78, 60, 8, ST77XX_WHITE);
  // tft.fillRoundRect(25, 90, 78, 60, 8, ST77XX_WHITE);

  //display temperature reading
  tft.setTextSize(5);
  tft.setTextWrap(false);
  tft.setFont();

  tft.setCursor(10, 10);
  //tft.setTextColor(0xff4112);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.println(temperature);

  //tft.setCursor(50, 10);
  //tft.setTextColor(ST77XX_WHITE);

  switch(mode)
  {
    case TEMP_F_MODE:
      tft.println("F");
      break;
    case TEMP_C_MODE:
      tft.println("C");
      break;
  }

  //dislay humidity reading
  //tft.setCursor(10, 100);
  //tft.setTextColor(0x8fbaff);
  tft.println(humidity);

  //tft.setCursor(50, 30);
  tft.setTextColor(ST77XX_WHITE);
  tft.println("%");
}

void updateFan(incomingTemp, incomingHum, mode){
  switch(mode)
  {
    case TEMP_F_MODE:
      
      break;
    case TEMP_C_MODE:
      if(temperature < 25)
        fanState = OFF;
      else if(temperature < 27)
        fanState = TWO;
      else if(temperature < 29)
        fanState = THREE;
      break;
  }
  fanStateMachine();
}

void systemStateMachine(){
  switch(systemState){
    case WAKE_UP:

      break;
    case ACTIVE:

      
      
      updateDisplay();
      fanStateMachine();
      break;
    case SLEEP:

      break;
  }
}


void updateDisplay(){
  // Look at graphics library to start coding
}

void fanStateMachine(){
  switch(fanState){
    case OFF:
      analogWrite(ENA, 0);
      break;
    case ONE:
      analogWrite(ENA, 100);
      break;
    case TWO:
      analogWrite(ENA, 175);
      break;
    case THREE:
      analogWrite(ENA, 255);
      break;
  }
  digitalWrite(D3, HIGH);
  digitalWrite(D4, LOW);
}

