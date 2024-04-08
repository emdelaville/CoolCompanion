
// Libraries needed
#include <list> // For std::list
#include <numeric> // for std::accumulate
#include <Preferences.h> // For storing data into flash
#include <Wire.h>
#include "WiFi.h"
#include <esp_now.h>


// Define pins
#define temp_SDA A4
#define temp_SCK A5

// Define Constants
#define padMacAddr "30:AE:A4:07:0D:64"
#define fanMacAddr "30:AE:A4:07:0D:64"


// Create Timers
unsigned long timer;

// Any Enums
enum SystemState {WAKE_UP, RECV, SEND, SLEEP} systemState;

typedef struct message{
  float temp;
  float hum;
} message;

message incomingReadings;
esp_now_peer_info_t peerInfo;

void dataSent(const uint8_t* mac_addr, esp_now_send_status_t status){
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
void dataRecieved(const uint8_t* mac, const uint8_t *incomingData, int len){
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  Serial.print("Bytes received: ");
  Serial.println(len);
  // incomingTemp = incomingReadings.temp;
  // incomingHum = incomingReadings.hum;
}

void setup() {
  // put your setup code here, to run once:

  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  WiFi.mode(WIFI_MODE_STA);
  Serial.print("MAC Address = " + String(WiFi.macAddress()));

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

  // Wire.beginTransmission(0x38); // Device Address
  // Wire.write(); // Register address of data
  // Wire.endTransmission();
  
}

void loop() {
  // put your main code here, to run repeatedly:

  // Wire.requestFrom(0x38, 1);
  // data = Wire.read();


}
