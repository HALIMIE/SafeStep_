#include <SPI.h>
#include <WiFiEsp.h>
#include <Servo.h>

#define AP_SSID "embA"
#define AP_PASS "embA1234"
#define SERVER_NAME "10.10.141.80"
#define SERVER_PORT 5000  
#define LOGID "BOX_ARD"

#define SPRAY_PIN 7
#define SERVO_PIN 3
#define BUZZER_PIN 2
#define IR_SENSOR_PIN 4

bool servoState = false;        
bool waitingForCan = false;     
unsigned long canDetectedTime = 0;

Servo myservo;

WiFiEspClient client;

void wifi_Setup();
void wifi_Init();
int server_Connect();
void printWifiStatus();
void receiveServerMessage();

void setup() {
    Serial.begin(9600);       
    Serial1.begin(38400);     

    WiFi.init(&Serial1);
    wifi_Setup();

    myservo.attach(SERVO_PIN);
    myservo.write(0);

    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(IR_SENSOR_PIN, INPUT);
    pinMode(SPRAY_PIN, OUTPUT);
    digitalWrite(SPRAY_PIN, HIGH);

    Serial.println(F("시스템 시작: 서버 명령 대기 중..."));
}

void loop() {
    if (waitingForCan && servoState) {
        int irState = digitalRead(IR_SENSOR_PIN);

        if (irState == LOW) {
            if (canDetectedTime == 0) {
                canDetectedTime = millis();
            } else if (millis() - canDetectedTime > 1000) {
                Serial.println("캔 감지됨 → 문 자동 닫힘");
                myservo.write(0);
                tone(BUZZER_PIN, 1000);
                delay(200);
                noTone(BUZZER_PIN);

                servoState = false;
                waitingForCan = false;
                canDetectedTime = 0;
            }
        } else {
            canDetectedTime = 0;
        }
    }

    receiveServerMessage();
}

void receiveServerMessage() {
    if (client.available()) {
        String receivedMsg = client.readStringUntil('\n');
        receivedMsg.trim();

        Serial.print("서버로부터 수신: ");
        Serial.println(receivedMsg);

        if (receivedMsg == "[SERVER_SQL] OPEN_50") {
            Serial.println("서버 명령: 문 열기");
            myservo.write(90);
            tone(BUZZER_PIN, 1000);
            delay(200);
            servoState = true;
            noTone(BUZZER_PIN);
            delay(5000);

            waitingForCan = true;
        } else if (receivedMsg == "[SERVER_SQL] CLOSE") {
            Serial.println("서버 명령: 문 닫기");
            myservo.write(0);
            servoState = false;
            tone(BUZZER_PIN, 1000);
            delay(200);
            noTone(BUZZER_PIN);

            waitingForCan = false;
        }
        else if (receivedMsg == "[SERVER_SQL] OPEN_70") {
            Serial.println("서버 명령: 스프레이 분사 (5초)");
            digitalWrite(SPRAY_PIN, LOW); 
            delay(500);                  
            digitalWrite(SPRAY_PIN, HIGH);
            delay(5000);                  
            digitalWrite(SPRAY_PIN, LOW);
            delay(500);                  
            digitalWrite(SPRAY_PIN, HIGH);
            myservo.write(90);
            servoState = true;
            tone(BUZZER_PIN, 1000);
            delay(200);
            noTone(BUZZER_PIN);
            delay(5000);

            waitingForCan = true;
        }
    }
}

void wifi_Setup() {
    wifi_Init();
    server_Connect();
}

void wifi_Init() {
    do {
        if (WiFi.status() == WL_NO_SHIELD) {
            Serial.println("WiFi shield not present");
        } else break;
    } while (1);

    Serial.print("Connecting to WiFi: ");
    Serial.println(AP_SSID);

    while (WiFi.begin(AP_SSID, AP_PASS) != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }

    Serial.println("\nWiFi 연결됨");
    printWifiStatus();
}

int server_Connect() {
    Serial.println("서버 연결 시도 중...");

    if (client.connect(SERVER_NAME, SERVER_PORT)) {
        Serial.println("서버 연결 성공");
        client.print("["LOGID"]");
        return 1;
    } else {
        Serial.println("서버 연결 실패");
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
