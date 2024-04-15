
// Libraries needed
#include <list> // For std::list
#include <numeric> // for std::accumulate
#include <Preferences.h> // For storing data into flash
#include <Wire.h>
#include "WiFi.h"
#include <esp_now.h>
#include <Adafruit_AHTX0.h>

Adafruit_AHTX0 aht1;
Adafruit_AHTX0 aht2;
Adafruit_AHTX0 aht3;
Adafruit_AHTX0 aht4;


// Define pins
#define temp_SDA A4
#define temp_SCK A5
#define I2CMux 0x70


// Define Constants
// #define padMacAddr "EC:DA:3B:63:AE:80"
// #define fanMacAddr "34:85:18:7B:C4:24"
uint8_t padMacAddr[] = {0xEC, 0xDA, 0x3B, 0x63, 0xAE, 0x80};
uint8_t fanMacAddr[] = {0x34, 0x85, 0x18, 0x7B, 0xC4, 0x24};

// Create Timers
unsigned long timer;

// Any Enums
enum SystemState {WAKE_UP, RECV, SEND, SLEEP} systemState;

typedef struct message{
  float temp;
  float hum;
} message;

float incomingTemp, incomingHum;

message incomingReadings;
message outgoingData;
esp_now_peer_info_t peerInfo;

void dataSent(const uint8_t* mac_addr, esp_now_send_status_t status){
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
void dataRecieved(const uint8_t* mac, const uint8_t *incomingData, int len){
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  Serial.print("Bytes received: ");
  Serial.println(len);
  incomingTemp = incomingReadings.temp;
  incomingHum = incomingReadings.hum;
  Serial.println("Temperature: " + String(incomingTemp));
  Serial.println("Humidity: " + String(incomingHum));
}

// Helper function for changing TCA output channel
void muxSelect(uint8_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(I2CMux);
  Wire.write(1 << channel);
  Wire.endTransmission();  
}

void sensorInit(){
  muxSelect(0);
  if (!aht1.begin()) {
    Serial.println("Could not find sensor 1. Check wiring");
    // while (1) delay(10);
  }
  muxSelect(1);
  if (!aht2.begin()) {
    Serial.println("Could not find sensor 2. Check wiring");
    // while (1) delay(10);
  }
  muxSelect(2);
  if (!aht3.begin()) {
    Serial.println("Could not find sensor 3. Check wiring");
    // while (1) delay(10);
  }
  muxSelect(3);
  if (!aht4.begin()) {
    Serial.println("Could not find sensor 4. Check wiring");
    // while (1) delay(10);
  }
}

void setup() {
  // put your setup code here, to run once:

  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  while(!Serial);
  Wire.begin();

  if (!aht1.begin()) {
    Serial.println("Could not find sensor. Check wiring");
    while (1) delay(10);
  }
  //sensorInit();

  WiFi.mode(WIFI_MODE_STA);
  Serial.println("MAC Address = " + String(WiFi.macAddress()));

  if(esp_now_init() != ESP_OK){
    Serial.print("Error initializing ESP-NOW");
  }
  esp_now_register_send_cb(dataSent);

  memcpy(peerInfo.peer_addr, fanMacAddr, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK)
    Serial.print("Failed to add peer");

  esp_now_register_recv_cb(dataRecieved);
  // pinMode(temp_SDA, );
  // pinMode(temp_SCK, OUTPUT);

  //Wire.begin(); // Device Address
  // Wire.write(); // Register address of data
  // Wire.endTransmission();
}

void loop() {
  // put your main code here, to run repeatedly:
  // Serial.println("MAC Address = " + String(WiFi.macAddress()));
  // Serial.println("Here");
  // Wire.requestFrom(1, 16);
  // while(Wire.available()){
  //muxSelect(1);
  sensors_event_t humidity, temp;
  aht1.getEvent(&humidity, &temp);// populate temp and humidity objects with fresh data
  Serial.print("Temperature: "); Serial.print(temp.temperature); Serial.println(" degrees C");
  Serial.print("Humidity: "); Serial.print(humidity.relative_humidity); Serial.println("% rH");

  outgoingData.temp = temp.temperature;
  outgoingData.hum = humidity.relative_humidity;

  esp_err_t result = esp_now_send(fanMacAddr, (uint8_t *) &outgoingData, sizeof(outgoingData));
   
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
  delay(5000);
  // }
  // Wire.requestFrom(0x38, 1);
  // data = Wire.read();
}

void parseData(String data){

  return;
}
