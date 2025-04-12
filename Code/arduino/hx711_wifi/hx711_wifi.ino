#include <Wire.h>
#include "HX711.h"
#include <WiFiEsp.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN 6
#define LED_COUNT 60
#define BRIGHTNESS 50

#define DOUT 2  
#define SCK  3  

#define ESP_SERIAL Serial1

#define AP_SSID "embA"
#define AP_PASS "embA1234"
#define SERVER_NAME "10.10.141.80"
#define SERVER_PORT 5000  

#define LOGID "HX_ARD"
#define CMD_SIZE 50

HX711 scale;
WiFiEspClient client;

float calibration_factor = -329.0; 
float zero = -92.0;                

typedef struct {
  char sendId[10];   
  char getSensorId[10];
  int sensorTime;      
  char sendBuf[CMD_SIZE];
} CommunicationData;

CommunicationData commData = {"HX_ARD", "", 10, ""};

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void wifi_Setup();
void wifi_Init();
int server_Connect();
void printWifiStatus();
void sendDataToServer();

void setup() {
  Serial.begin(9600);          
  ESP_SERIAL.begin(38400);     

  scale.begin(DOUT, SCK);
  scale.set_scale(calibration_factor); 

  wifi_Setup();

  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
}

void loop() {
  float weight = scale.get_units() - zero;
  if(weight > 10)
  {
    for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, 255, 255, 255); 
    }  
    strip.show();
    sendDataToServer();
    delay(10000);
    for (int i = 0; i < LED_COUNT; i++) {
      strip.setPixelColor(i, 0, 0, 0);
    }
    strip.show();
    Serial.print("Weight: ");
    Serial.println(weight);
    delay(5000);
  }
  delay(20000);
}

void wifi_Setup() {
  wifi_Init();
  server_Connect();
}

void wifi_Init() {
  do {
    WiFi.init(&ESP_SERIAL);
    if (WiFi.status() == WL_NO_SHIELD) {
      Serial.println("WiFi shield not present");
    } else {
      break;
    }
  } while (1);

  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(AP_SSID);

  while (WiFi.begin(AP_SSID, AP_PASS) != WL_CONNECTED) {
    Serial.print("Retrying connection to SSID: ");
    Serial.println(AP_SSID);
    delay(1000);
  }

  Serial.println("You're connected to the network");
  printWifiStatus();
}

int server_Connect() {
  Serial.println("Starting connection to server...");

  if (client.connect(SERVER_NAME, SERVER_PORT)) {
    Serial.println("Connected to server");
    client.print("[" LOGID "]");
    return 1;
  } else {
    Serial.println("Server connection failure");
    return 0;
  }
}

void printWifiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI): ");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void sendDataToServer() {
  snprintf(commData.sendBuf, CMD_SIZE, "[JETSON]:1\r\n");

  if (client.connected()) {
    client.write(commData.sendBuf, strlen(commData.sendBuf));
    client.flush();

    Serial.print("전송 데이터: ");
    Serial.print(commData.sendBuf);
  } else {
    Serial.println("Server disconnected, attempting reconnect...");
    server_Connect();
  }
}
