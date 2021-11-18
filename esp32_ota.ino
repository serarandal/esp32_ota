#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include <ThingsBoard.h>
#include "cert.h"
#include "Sensores.hpp"

// Helper macro to calculate array size
#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))
// WiFi access point
#define WIFI_AP_NAME        "SBC"
// WiFi password
#define WIFI_PASSWORD       "sbc$2020"
#define TOKEN               "ljHgMFSpZ8ChPT9Q5cvn"
// ThingsBoard server instance.
#define THINGSBOARD_SERVER  "demo.thingsboard.io"
// Baud rate for debug serial
#define SERIAL_DEBUG_BAUD    9600
// Initialize ThingsBoard client
WiFiClient espClient;
// Initialize ThingsBoard instance
ThingsBoard tb(espClient);
// the Wifi radio's status
int status = WL_IDLE_STATUS;

float temp = 0;
float hum = 0;
// Main application loop delay
int quant = 20;

// Set to true if application is subscribed for the RPC messages.
bool subscribed = false;

// Processes function for RPC call "setValue"
// RPC_Data is a JSON variant, that can be queried using operator[]
// See https://arduinojson.org/v5/api/jsonvariant/subscript/ for more details
RPC_Response processDelayChange(const RPC_Data &data)
{
  Serial.println("Received the set delay RPC method");
  // Process data
  Serial.print("Set new delay: ");
}
// Processes function for RPC call "getValue"
// RPC_Data is a JSON variant, that can be queried using operator[]
// See https://arduinojson.org/v5/api/jsonvariant/subscript/ for more details
RPC_Response processGetDelay(const RPC_Data &data)
{
  Serial.println("Received the get value method");
}
// Processes function for RPC call "setGpioStatus"
// RPC_Data is a JSON variant, that can be queried using operator[]
// See https://arduinojson.org/v5/api/jsonvariant/subscript/ for more details
RPC_Response processSetGpioState(const RPC_Data &data)
{
  Serial.println("Received the set GPIO RPC method");
}

// RPC handlers
RPC_Callback callbacks[] = {
  { "setValue",         processDelayChange },
  { "getValue",         processGetDelay },
  { "setGpioStatus",    processSetGpioState },
};


String FirmwareVer = {
  "2"
};
#define URL_fw_Version "https://raw.githubusercontent.com/serarandal/esp32_ota/master/bin_version.txt"
#define URL_fw_Bin "https://raw.githubusercontent.com/serarandal/esp32_ota/master/fw.bin"

//#define URL_fw_Version "http://cade-make.000webhostapp.com/version.txt"
//#define URL_fw_Bin "http://cade-make.000webhostapp.com/firmware.bin"

void connect_wifi();
void firmwareUpdate();
int FirmwareVersionCheck();

unsigned long previousMillis = 0; // will store last time LED was updated
unsigned long previousMillis_2 = 0;
const long interval = 60000;
const long mini_interval = 1000;
void repeatedCall() {
  static int num=0;
  unsigned long currentMillis = millis();
  if ((currentMillis - previousMillis) >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    if (FirmwareVersionCheck()) {
      firmwareUpdate();
    }
  }
  if ((currentMillis - previousMillis_2) >= mini_interval) {
    previousMillis_2 = currentMillis;
    Serial.print("idle loop...");
    Serial.print(num++);
    Serial.print(" Active fw version:");
    Serial.println(FirmwareVer);
   if(WiFi.status() == WL_CONNECTED) 
   {
       Serial.println("wifi connected");
   }
   else
   {
    connect_wifi();
   }
  }
}

struct Button {
  const uint8_t PIN;
  uint32_t numberKeyPresses;
  bool pressed;
};

Button button_boot = {
  0,
  0,
  false
};
/*void IRAM_ATTR isr(void* arg) {
    Button* s = static_cast<Button*>(arg);
    s->numberKeyPresses += 1;
    s->pressed = true;
}*/

void IRAM_ATTR isr() {
  button_boot.numberKeyPresses += 1;
  button_boot.pressed = true;
}


void setup() {
  pinMode(button_boot.PIN, INPUT);
  attachInterrupt(button_boot.PIN, isr, RISING);
  Serial.begin(SERIAL_DEBUG_BAUD);
  Serial.print("Active firmware version:");
  Serial.println(FirmwareVer);
 // pinMode(LED_BUILTIN, OUTPUT);
  connect_wifi();
}


// Main application loop
void loop() {
	delay(quant);
  
  if (button_boot.pressed) { //to connect wifi via Android esp touch app 
    Serial.println("Firmware update Starting..");
    firmwareUpdate();
    button_boot.pressed = false;
  }
   
   
   if (!tb.connected()) {
    subscribed = false;
    // Connect to the ThingsBoard
    Serial.print("Connecting to: ");
    Serial.print(THINGSBOARD_SERVER);
    Serial.print(" with token ");
    Serial.println(TOKEN);
    if (!tb.connect(THINGSBOARD_SERVER, TOKEN)) {
      Serial.println("Failed to connect");
      return;
    }
  }
  
  // Subscribe for RPC, if needed
  if (!subscribed) {
    Serial.println("Subscribing for RPC...");
    // Perform a subscription. All consequent data processing will happen in
    // callbacks as denoted by callbacks[] array.
    if (!tb.RPC_Subscribe(callbacks, COUNT_OF(callbacks))) {
      Serial.println("Failed to subscribe for RPC");
      return;
    }
    Serial.println("Subscribe done");
    subscribed = true;
  }
  
  Serial.println("Sending data...");
  // Uploads new telemetry to ThingsBoard using MQTT.
  //llamar a sensores para coger el string para procesarlo y mandarlo
  String DATA = get_Data();
  Serial.println(DATA);
  tb.sendTelemetryFloat("temperatureDht", getDhtTemperature());
  tb.sendTelemetryFloat("humidityDht", getDhtHumidity());
  tb.sendTelemetryFloat("temperatureBme",getBmeTemperature());
  tb.sendTelemetryFloat("pressureBme",getBmePressure());
  tb.sendTelemetryFloat("altitudeBme",getBmeAltitude());
  tb.sendTelemetryFloat("humidityBme",getBmeHumidity());
  tb.sendTelemetryFloat("CO2",getCO2());
  
  
  repeatedCall();
  tb.loop();
}







void connect_wifi() {
  Serial.println("Waiting for WiFi");
  WiFi.begin(WIFI_AP_NAME, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}








void firmwareUpdate(void) {
  WiFiClientSecure client;
  client.setCACert(rootCACertificate);
 // httpUpdate.setLedPin(LED_BUILTIN, LOW);
  t_httpUpdate_return ret = httpUpdate.update(client, URL_fw_Bin);

  switch (ret) {
  case HTTP_UPDATE_FAILED:
    Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
    break;

  case HTTP_UPDATE_NO_UPDATES:
    Serial.println("HTTP_UPDATE_NO_UPDATES");
    break;

  case HTTP_UPDATE_OK:
    Serial.println("HTTP_UPDATE_OK");
    break;
  }
}
int FirmwareVersionCheck(void) {
  String payload;
  int httpCode;
  String fwurl = "";
  fwurl += URL_fw_Version;
  fwurl += "?";
  fwurl += String(rand());
  Serial.println(fwurl);
  WiFiClientSecure * client = new WiFiClientSecure;

  if (client) 
  {
    client -> setCACert(rootCACertificate);

    // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 
    HTTPClient https;

    if (https.begin( * client, fwurl)) 
    { // HTTPS      
      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      delay(100);
      httpCode = https.GET();
      delay(100);
      if (httpCode == HTTP_CODE_OK) // if version received
      {
        payload = https.getString(); // save received version
      } else {
        Serial.print("error in downloading version file:");
        Serial.println(httpCode);
      }
      https.end();
    }
    delete client;
  }
      
  if (httpCode == HTTP_CODE_OK) // if version received
  {
    payload.trim();
    if (payload.equals(FirmwareVer)) {
      Serial.printf("\nDevice already on latest firmware version:%s\n", FirmwareVer);
      return 0;
    } 
    else 
    {
      Serial.println(payload);
      Serial.println("New firmware detected");
      return 1;
    }
  } 
  return 0;  
}
