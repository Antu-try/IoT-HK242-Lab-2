#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Cấu hình WiFi
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// Cấu hình ThingsBoard
const char* thingsboardServer = "thingsboard.cloud"; 
const char* accessToken = "ubvcrKrhXVLyYtTBnVXU";

WiFiClient espClient;
PubSubClient client(espClient);

const int buttonPin = 18; // Nút nhấn
const int pumpPin = 2;    
bool pumpState = false;   

// Kết nối WiFi
void setupWiFi() {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");
}

// Callback khi nhận dữ liệu từ ThingsBoard
void callback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    Serial.println("Received from ThingsBoard: " + message);

    // Phân tích chuỗi JSON
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, message);

    if (error) {
        Serial.println("Failed to parse JSON!");
        return;
    }

    // Kiểm tra trường "method"
    String method = doc["method"];

    if (method == "setState") {
        // Xử lý setState
        bool params = doc["params"];
        if (params) {
            digitalWrite(pumpPin, HIGH);
            pumpState = true;
            Serial.println("Pump turned ON");
        } else {
            digitalWrite(pumpPin, LOW);
            pumpState = false;
            Serial.println("Pump turned OFF");
        }
    } else if (method == "led") {
        // Xử lý led
        JsonObject params = doc["params"];
        String ledMethod = params["method"];
        if (ledMethod == "on" || ledMethod == "true") {
            digitalWrite(pumpPin, HIGH);
            pumpState = true;
            Serial.println("LED turned ON");
        } else if (ledMethod == "off" || ledMethod == "false") {
            digitalWrite(pumpPin, LOW);
            pumpState = false;
            Serial.println("LED turned OFF");
        } else {
            Serial.println("Unknown LED method");
        }
    } else {
        Serial.println("Unknown method received");
    }
}

// Kết nối MQTT với ThingsBoard
void setupMQTT() {
  Serial.println("Starting MQTT connection...");
  client.setServer(thingsboardServer, 1883);
  client.setCallback(callback);

  unsigned long startAttemptTime = millis();
  while (!client.connected()) {
      if (millis() - startAttemptTime > 5000) { // Timeout sau 5 giây
          Serial.println("MQTT connection timeout! Check access token and server.");
          return;
      }

      Serial.println("Attempting MQTT connection...");
      if (client.connect("ESP32_Client", accessToken, NULL)) {
          Serial.println("Connected to ThingsBoard MQTT!");
          client.subscribe("v1/devices/me/rpc/request/+");
      } else {
          Serial.print("Failed, MQTT state = ");
          Serial.println(client.state());
          delay(1000);
      }
  }
}

void sendPumpState() {
  StaticJsonDocument<256> doc;
  doc["pump_state"] = pumpState;  // Giá trị true hoặc false

  char buffer[256];
  serializeJson(doc, buffer);

  Serial.print("Sending: ");
  Serial.println(buffer);

  // Gửi dữ liệu lên ThingsBoard
  if (client.publish("v1/devices/me/telemetry", buffer)) {
      Serial.println("Data sent successfully!");
  } else {
      Serial.println("Failed to send data!");
  }
}
void setup() {
    Serial.begin(115200);
    Serial.println("Starting ESP32...");
    pinMode(buttonPin, INPUT_PULLUP);
    pinMode(pumpPin, OUTPUT);
    
    setupWiFi();
    Serial.println("WiFi setup done.");
    setupMQTT();
    Serial.println("MQTT setup done.");
}

void loop() {
    if (!client.connected()) {
        setupMQTT();
    }
    client.loop();
    
    // Kiểm tra nút nhấn
    if (digitalRead(buttonPin) == LOW) {
        pumpState = !pumpState;
        digitalWrite(pumpPin, pumpState ? HIGH : LOW);
        sendPumpState();
        delay(500);
    }
    
    delay(1000);
}
