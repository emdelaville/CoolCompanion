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
void printSensorReadings(float temperature, float humidity, int mode, int targetTemp);
void displayEditMode(float targetTemp, int mode);
//Display Initalization end //----------------------

void updateFan(float incomingTemp, float incomingHum, int mode);
// Define pins
#define ENA A7
#define IN1 D3
#define IN2 D4
#define ON_OFF D5
#define increase D6
#define decrease D7
#define modeSel D12

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
//enum SystemState {WAKE_UP, ACTIVE, SLEEP} systemState;
enum FanState {OFF, ONE, TWO, THREE} fanState;
enum Mode {ACTIVE, EDIT_TEMP, EDIT_UNIT} editMode;


typedef struct inMessage{
  float avgTemp;
} inMessage;

typedef struct outMessage{
  boolean onOrOff;
} outMessage;

boolean isOn;
int buttonVal1, prevButtonVal1;
int buttonVal2, prevButtonVal2;
int buttonVal3, prevButtonVal3;
int buttonVal4, prevButtonVal4;

volatile float targetAppTemp = 22.50;
int temp_unit = TEMP_C_MODE;
int dispRefresh = 0;

unsigned long currTime = 0;

inMessage incomingReadings;// = (message*)malloc(sizeof(message));
outMessage outgoingData;
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
  incomingTemp = incomingReadings.avgTemp;
}

void setup() {
  // put your setup code here, to run once:

  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  // while(!Serial);
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


  //systemState = WAKE_UP;
  fanState = OFF;

  isOn = true;
  prevButtonVal1 = HIGH;
  buttonVal1 = HIGH;
  prevButtonVal2 = HIGH;
  buttonVal2 = HIGH;
  prevButtonVal3 = HIGH;
  buttonVal3 = HIGH;
  prevButtonVal4 = HIGH;
  buttonVal4 = HIGH;

  //pinMode();
  pinMode(A7, OUTPUT);
  pinMode(D3, OUTPUT);
  pinMode(D4, OUTPUT);
  pinMode(ON_OFF, INPUT);
  pinMode(increase, INPUT);
  pinMode(decrease, INPUT);
  pinMode(modeSel, INPUT);



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

  //Buttons 
  int buttonVal1 = digitalRead(ON_OFF);
  if(buttonVal1 != prevButtonVal1){
    if(buttonVal1 == LOW){
       Serial.println("Button 1 Pressed");
      isOn = !isOn;
    }
    delay(50);
  }
  prevButtonVal1 = buttonVal1;

  if(buttonVal2 != prevButtonVal2){
    if(buttonVal2 == LOW){
      if(editMode == EDIT_TEMP){
        targetAppTemp += 0.5;
        Serial.println("Button 2 Pressed");
      }else if(editMode == EDIT_UNIT){
        temp_unit = TEMP_C_MODE;
      }
    }
    delay(50);
  }
  if(buttonVal3 != prevButtonVal3){
    if(buttonVal3 == LOW){
      if(editMode == EDIT_TEMP){
        targetAppTemp -= 0.5;
        Serial.println("Button 3 Pressed");
      }else if(editMode == EDIT_UNIT){
        temp_unit = TEMP_F_MODE;
      }
    }
    delay(50);
  }
  int buttonVal4 = digitalRead(modeSel);
  if(buttonVal4 != prevButtonVal4){
    if(buttonVal4 == LOW){
       Serial.println("Button 4 Pressed");
      dispRefresh = 0;
      switch(editMode){
        case ACTIVE:{
          editMode = EDIT_TEMP;
          break;
        }
        case EDIT_TEMP:{
          editMode = EDIT_UNIT;
          break;
        }
        case EDIT_UNIT:{
          editMode = ACTIVE;
          break;
        }
      }
    }
    delay(50);
  }
  prevButtonVal4 = buttonVal4;

  if(isOn){

    int second_flag =0;
    long unsigned int second_counter =0;
    float hum_reading;
    float temp_reading;

    float AvgHumidity;
    float AvgTemperatureC;
    switch(editMode)
    {
      case ACTIVE:

        if(dispRefresh==0){
          tft.fillScreen(ST77XX_BLACK);
        }
        dispRefresh = 1;

        if(millis() % 1000 == 1)
        { //every second
          second_flag = 1; //raise flag to signal average
          second_counter++;
        }
    
        if(second_flag == 1){
          switch(temp_unit)
            {
              case TEMP_F_MODE:
                //printSensorReadings(rollingAvgHum(hum_reading, second_counter), rollingAvgTemp(temp_reading, second_counter), mode);
                printSensorReadings(incomingTemp, incomingHum, temp_unit, targetAppTemp);
                updateFan(incomingTemp, incomingHum, temp_unit);
                break;
              case TEMP_C_MODE:
                //printSensorReadings(incomingTemp, incomingHum, temp_unit);
                printSensorReadings(92.85, incomingHum, temp_unit, targetAppTemp);
                updateFan(incomingTemp, incomingHum, temp_unit);
                break;
            }
            second_flag = 0;
        }
        break;
      case EDIT_TEMP:
        if(dispRefresh==0){
          tft.fillScreen(ST77XX_BLACK);
        }
        dispRefresh = 1;
        //Button 1 - ON/OFF
        //Button 2 - UP
        //Button 3 - DOWN
        //Button 4 - Mode Sel

        displayEditMode(targetAppTemp, temp_unit);

        break;
      case EDIT_UNIT:
        if(dispRefresh==0){
          tft.fillScreen(ST77XX_BLACK);
        }
        dispRefresh = 1;
        //Button 1 - ON/OFF
        //Button 2 - UP
        //Button 3 - DOWN
        //Button 4 - Mode Sel
    
        tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
        tft.setTextSize(3);
        tft.setCursor(30, 100);
        tft.print("Select:");
        tft.setCursor(120, 100);
        tft.write(0x58);
        tft.print("C or ");
        tft.write(0x58);
        tft.print("F");


        break;
    }
  }
}


#define TEMP_DISP_VERT_COORD 50
void displayEditMode(float targetTemp, int mode)
{
  tft.setTextWrap(false);
  tft.setTextSize(4);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(55, TEMP_DISP_VERT_COORD);
  tft.setTextSize(6);

  tft.cp437(true); // Use correct CP437 character codes
  tft.print(targetTemp);

  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK); //set text to white and background to black
  tft.write(0xF8); // Print the degrees symbol

  switch(mode)
  {
    case TEMP_F_MODE:    
      tft.println("F");   
    break;
    case TEMP_C_MODE:
      tft.println("C");   
    break;
  }
}



int blink =0;
void printSensorReadings(float temperature, float humidity, int mode, int targetTemp) //-------------------------------------------------------------------------------------
{ 
  tft.setTextWrap(false);
  tft.setTextSize(4);
  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);

    //if there is no temperature reading, print a no signal message
    if(temperature == 0)
    {
      blink++; 
      if(blink % 2 == 0)
      {
        tft.setCursor(54, 50);
        tft.println("NO SIGNAL");
      }
      if(blink % 2 == 1)
      {
        tft.setTextColor(ST77XX_BLACK, ST77XX_BLACK);
        tft.setCursor(54, 50);
        tft.println("NO SIGNAL");
      }

      tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
      tft.setTextSize(2);
      tft.setCursor(83, 80);
      tft.println("Please Ensure");
      tft.setCursor(59, 126);
      tft.print("Connection to Pad");
    }
    else
    {
    //tft.drawRectangle();
    //draw bounding boxes around values
    // tft.fillRoundRect(25, 10, 78, 60, 8, ST77XX_WHITE);
    // tft.fillRoundRect(25, 90, 78, 60, 8, ST77XX_WHITE);

    //display temperature reading
    // tft.setCursor(30, 100);
    // tft.setTextSize(6);

    if(temperature < 10 && temperature > 0) // if text is three digits, make smaller.
    {
      tft.setCursor(55, TEMP_DISP_VERT_COORD);
      tft.setTextSize(6);
    }
    if(temperature < 100 && temperature >= 10) // if text is three digits, make smaller.
    {
      tft.setCursor(37, TEMP_DISP_VERT_COORD);
      tft.setTextSize(6);
    }
    if(temperature < 1000 && temperature >= 100) // if text is three digits, make smaller.
    {
      tft.setCursor(19, TEMP_DISP_VERT_COORD);
      tft.setTextSize(6);
    }
    //tft.setFont(&FreeSerif9pt7b);
    //tft.setTextColor(0xff4112);

    tft.setTextColor(ST77XX_RED, ST77XX_BLACK); //set text to white and background to black
    tft.cp437(true); // Use correct CP437 character codes
    tft.print(temperature);

    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK); //set text to white and background to black
    tft.write(0xF8); // Print the degrees symbol

    switch(mode)
    {
      case TEMP_F_MODE:    
        tft.println("F");   
      break;
      case TEMP_C_MODE:
        tft.println("C");   
      break;
    }

    //tft.fillRoundRect(25, 10, 78, 60, 8, ST77XX_WHITE);
    //display target temperature
    tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK); //set text to white and background to black

    if(targetTemp < 10 && targetTemp > 0) 
    {
      tft.setCursor(90, TEMP_DISP_VERT_COORD+50);
      tft.setTextSize(4);
    }
    if(targetTemp < 100 && targetTemp >= 10) 
    {
      tft.setCursor(78, TEMP_DISP_VERT_COORD+50);
      tft.setTextSize(4);
    }
    if(targetTemp < 1000 && targetTemp >= 100) 
    {
      tft.setCursor(66, TEMP_DISP_VERT_COORD+50);
      tft.setTextSize(4);
    }

    tft.setTextColor(ST77XX_RED, ST77XX_BLACK); //set text to white and background to black
    tft.cp437(true); // Use correct CP437 character codes
    tft.print(targetTemp);

    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK); //set text to white and background to black
    tft.write(0xF8); // Print the degrees symbol

    switch(mode)
    {
      case TEMP_F_MODE:    
        tft.println("F");   
      break;
      case TEMP_C_MODE:
        tft.println("C");   
      break;
    }

    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.setTextSize(3);
    tft.setCursor(10, 150);
    tft.print("Fan Speed:");
    tft.setCursor(187, 150);
    tft.print("50");
    tft.write(0x25); //% symbol
    //dislay humidity reading
    //tft.setCursor(10, 100);
    //tft.setTextColor(0x8fbaff);
    // tft.setTextSize(1);
    // //tft.println("----------------");
    // void drawFastHLine(0, 60, 320, ST77XX_WHITE);
    //tft.drawLine(0, 60, x, tft.height() - 1, color);
    //tft.setTextSize(3);


    // tft.print(humidity); 
    // tft.write(0x25); //% symbol
    // tft.println();

    // tft.setTextSize(3);
    // tft.println("User Setting:");
    // tft.print("HOT");

    //drawBitmap(300, 10, 24, 24 temp_icon, ST77XX_WHITE);
  }
}

void updateFan(float incomingTemp, float incomingHum, int mode)
{
  switch(mode)
  {
    case TEMP_F_MODE:
      
      break;
    case TEMP_C_MODE:
      if(incomingTemp < 26)
        fanState = OFF;
      else if(incomingTemp < 29)
        fanState = ONE;
      else if(incomingTemp < 32)
        fanState = TWO;
      else if(incomingTemp >= 32)
        fanState = THREE;
      break;
  }
  fanStateMachine();
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
