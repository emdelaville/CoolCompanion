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

// Define pins
#define ENA A7
#define IN1 D2
#define IN2 D3
#define ON_OFF D4
#define increase D5
#define decrease D6
#define modeSel D7

// Define Constants
// #define padMacAddr "EC:DA:3B:63:AE:80"
// #define fanMacAddr "34:85:18:7B:C4:24"
#define displayWidth 320
#define displayHeight 240
#define ABS_MAX_TEMP 90
#define TEMP_DISP_VERT_COORD 30
#define TARGET_DISP_VERT_COORD 80

uint8_t padMacAddr[] = {0xEC, 0xDA, 0x3B, 0x63, 0xAE, 0x80};
uint8_t fanMacAddr[] = {0x34, 0x85, 0x18, 0x7B, 0xC4, 0x24};

// Create Timers
unsigned long timer;

// Any Enums
//enum SystemState {WAKE_UP, ACTIVE, SLEEP} systemState;
enum FanState {OFF, ONE, TWO, THREE} fanState;
enum Mode {ACTIVE, EDIT_TEMP} editMode;
//enum Mode {ACTIVE, EDIT_TEMP, EDIT_UNIT} editMode;

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

volatile float targetAppTemp = 60;
int temp_unit = TEMP_F_MODE;
int dispRefresh = 0;
int noSignal =0;

unsigned long currTime = 0;

inMessage incomingReadings;// = (message*)malloc(sizeof(message));
outMessage outgoingData;
float incomingTemp;
esp_now_peer_info_t peerInfo;

int blink = 0;
int second_flag =0;
long unsigned int second_counter = 0;
float temp_reading;
int displayFlag =0;
int fanSpeedPercent=0; 
int fanSpeed;
int size1Flag =0;
int size2Flag =0;
int size3Flag =0;

unsigned long previousTime = 0;
const unsigned long dataTimeout = 6000; // 6 seconds in milliseconds

//function prototypes
void printSensorReadings(float temperature, int mode, float targetTemp);
void displayEditMode(float targetTemp, int mode);
void fanSpeedControl(void);
void readButtons(void);


void setup() {
  Serial.begin(9600);
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
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ON_OFF, INPUT);
  pinMode(increase, INPUT);
  pinMode(decrease, INPUT);
  pinMode(modeSel, INPUT);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);

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
  readButtons();

  if(isOn){
    switch(editMode)
    {
      case ACTIVE:

        if (millis() - previousTime > dataTimeout)
        {
            if(dispRefresh==0)
            {
              tft.fillScreen(ST77XX_BLACK);
            }
            second_counter = millis()/1000;
            dispRefresh = 1;
            noSignal=1;
            //tft.fillScreen(ST77XX_BLACK);
            tft.setTextWrap(false);
            tft.setTextSize(4);
            if(second_counter%2==0)
            {
              tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK); 
            }
            else
            {
               tft.setTextColor(ST77XX_BLACK, ST77XX_BLACK); 
            }
            
            tft.setCursor(54, 50);
            tft.println("NO SIGNAL");
              
            tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
            tft.setTextSize(2);
            tft.setCursor(83, 106);
            tft.println("Please Ensure");
            tft.setCursor(59, 126);
            tft.print("Connection to Pad");

            analogWrite(ENA, 0);
            digitalWrite(IN1, LOW);
            digitalWrite(IN2, HIGH);
        }
        if(displayFlag==1 && isOn == true)
        {
          if(dispRefresh==1)
          {
          tft.fillScreen(ST77XX_BLACK);
          }
          dispRefresh = 0;
          displayFlag = 0;
          printSensorReadings(incomingTemp, temp_unit, targetAppTemp);
          fanSpeedControl();
  
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
    }
  }
}

