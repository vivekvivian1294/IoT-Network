#include <string.h>
#include <Arduino_HTS221.h>
#include <ArduinoBLE.h>

#define BLE_UUID_TEST_SERVICE "9A48ECBA-2E92-082F-C079-9E75AAE428B1"
#define BLE_UUID_FILE_NAME    "2D2F88C4-F244-5A80-21F1-EE0224E80658"
#define PRINT_MODE 0

//Declare temperature services for displaying data
BLEService temperatureService(BLE_UUID_TEST_SERVICE);
BLEStringCharacteristic temperatureLevelChar(BLE_UUID_FILE_NAME, BLERead | BLENotify, 7);
//BLEFloatCharacteristic temperatureLevelChar(BLE_UUID_FILE_NAME, BLERead | BLENotify);

//Convert uint8_t array to string function
String converter(uint8_t *str){
    return String((char *)str);
}

//temperature string uint_8 array
uint8_t temp_string[7];


void setup() {
  // put your setup code here, to run once:
  //Configure WDT.
  //NRF_WDT->CONFIG         = 0x01;     // Configure WDT to run when CPU is asleep
  //NRF_WDT->CRV            = 1966079;  // Timeout set to 60 seconds, timeout[s] = (CRV-1)/32768, 120 seconds: 3932159
  //NRF_WDT->RREN           = 0x01;     // Enable the RR[0] reload register
  //NRF_WDT->TASKS_START    = 1;        // Start WDT 

  
  if(PRINT_MODE ==1)
  {
    Serial.begin(9600);
    while (!Serial);
  }
  else
  {
    delay(5000);
  }
  
  
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  //Initialize temperature sensor
  HTS.begin();
  delay(5000);
  // initialize the BLE hardware
  BLE.begin();
  delay(5000);
  
  //Set ble device name
  BLE.setDeviceName("TemperatureMonitor001");
  BLE.setLocalName("TemperatureMonitor001");
  delay(5000);
  //Set temperature service for displaying the data
  BLE.setAdvertisedService(temperatureService);
  temperatureService.addCharacteristic(temperatureLevelChar);
  BLE.addService(temperatureService);
  delay(5000);  
  //Start advertising
  BLE.advertise();
  delay(5000);
}

void loop() {
  // put your main code here, to run repeatedly:
  BLEDevice central = BLE.central();
  // read temperature sensor
  
  if(central)
  {
    
    while (central.connected()) {
      float temperature = HTS.readTemperature();
      if(PRINT_MODE == 1)
      {
        Serial.println(temperature);
      }
      //delay(500);
      

      //convert float temperature to uint8_t array
      snprintf ((char *)temp_string, sizeof(temp_string), "%.2f", temperature);
      temperatureLevelChar.writeValue(converter(temp_string));
      //NRF_WDT->RR[0] = WDT_RR_RR_Reload;
      digitalWrite(LED_BUILTIN, HIGH);
      delay(1000);
    }
    
      digitalWrite(LED_BUILTIN, LOW);   // turn the LED on (HIGH is the voltage level)
      delay(1000);                       // wait for a second
  }
  else
  {
    // turn the LED off by making the voltage LOW
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
    central.disconnect();
    
  }
      

}
