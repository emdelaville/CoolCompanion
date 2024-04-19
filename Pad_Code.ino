
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
uint8_t padMacAddr[] = {0xEC, 0xDA, 0x3B, 0x63, 0xAE, 0x80};
uint8_t fanMacAddr[] = {0x34, 0x85, 0x18, 0x7B, 0xC4, 0x24};

// Create Timers
unsigned long timer;

// Any Enums
enum SystemState {WAKE_UP, RECV, SEND, SLEEP} systemState;

typedef struct message{
  float avgTemp;
} message;

float incomingTemp;

message incomingReadings;
message outgoingData;
float sensorData[4];
esp_now_peer_info_t peerInfo;

void dataSent(const uint8_t* mac_addr, esp_now_send_status_t status){
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
void dataRecieved(const uint8_t* mac, const uint8_t *incomingData, int len){
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  Serial.print("Bytes received: ");
  Serial.println(len);
  incomingTemp = incomingReadings.avgTemp;
  Serial.println("Apparent Temperature: " + String(incomingTemp));
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
  // muxSelect(2);
  // if (!aht3.begin()) {
  //   Serial.println("Could not find sensor 3. Check wiring");
  //   // while (1) delay(10);
  // }
  // muxSelect(3);
  // if (!aht4.begin()) {
  //   Serial.println("Could not find sensor 4. Check wiring");
  //   // while (1) delay(10);
  // }
}

void setup() {
  // put your setup code here, to run once:

  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  while(!Serial);
  Wire.begin();

   sensorInit();

  // if (!aht1.begin()) {
  //   Serial.println("Could not find sensor. Check wiring");
  //   while (1) delay(10);
  // }
  
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

  sensors_event_t humidity, temp;
  Serial.println("\nSensor 1 ");
  muxSelect(0);
  aht1.getEvent(&humidity, &temp);// populate temp and humidity objects with fresh data
  Serial.print("\tTemperature: "); Serial.print(temp.temperature); Serial.println(" degrees C");
  Serial.print("\tHumidity: "); Serial.print(humidity.relative_humidity); Serial.println("% rH");

  sensorData[0] = apparentTemp(temp.temperature, humidity.relative_humidity);
  Serial.print("\tApparent Temperature: "); Serial.print(sensorData[0]); Serial.println(" degrees C");


  Serial.println("Sensor 2 ");
  muxSelect(1);
  aht2.getEvent(&humidity, &temp);// populate temp and humidity objects with fresh data
  Serial.print("\tTemperature: "); Serial.print(temp.temperature); Serial.println(" degrees C");
  Serial.print("\tHumidity: "); Serial.print(humidity.relative_humidity); Serial.println("% rH");

  sensorData[1] = apparentTemp(temp.temperature, humidity.relative_humidity);
  Serial.print("\tApparent Temperature: "); Serial.print(sensorData[1]); Serial.println(" degrees C");


  outgoingData.avgTemp = (sensorData[0] + sensorData[1])/2.0;
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

float apparentTemp(float temp, float hum){
    float dewpoint = temp - ((100 - hum) / 5); // the numbers are constants, RH =relative humidity, temperature must be in C
    float exponent = 5417.7530*((1/273.15)-(1/(273.15+dewpoint)));

    float humidex = temp + (0.5555*(6.11*exp(exponent)-10)); // apparent temp in C

    //float heatIndex = C1 * (temp + C2 + ((temp - C3) * C4) + (C5 * hum));
    return humidex;
}

void parseData(String data){

  return;
}
