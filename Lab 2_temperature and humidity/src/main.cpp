#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// Cấu hình WiFi
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// Cấu hình ThingsBoard
const char* tbServer = "thingsboard.cloud"; // Hoặc địa chỉ server riêng
const char* deviceToken = "WL6wFPlS5caYrFAWsjbQ";
WiFiClient espClient;
PubSubClient client(espClient);

// Cấu hình DHT22
#define DHTPIN 15
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Hàm kết nối WiFi
void setupWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
}

// Hàm kết nối ThingsBoard
void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32_Device", deviceToken, nullptr)) {
      Serial.println("Connected to ThingsBoard");
    } else {
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  setupWiFi();
  client.setServer(tbServer, 1883); // Port MQTT mặc định
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    // Đọc dữ liệu từ DHT22 mỗi 2 giây
    static unsigned long lastRead = 0;
    if (millis() - lastRead >= 5000) {
        lastRead = millis();

        Serial.println("Reading temperature and humidity...");
        float temperature = dht.readTemperature();
        float humidity = dht.readHumidity();

        if (isnan(temperature) || isnan(humidity)) {
            Serial.println("Failed to read from DHT sensor!");
        } else {
            Serial.print("Temperature: ");
            Serial.print(temperature);
            Serial.println(" °C");
            Serial.print("Humidity: ");
            Serial.print(humidity);
            Serial.println(" %");

            // Tạo payload JSON
            String payload = "{";
            payload += "\"temperature\":" + String(temperature) + ",";
            payload += "\"humidity\":" + String(humidity);
            payload += "}";

            // Gửi dữ liệu lên ThingsBoard qua MQTT
            client.publish("v1/devices/me/telemetry", payload.c_str());
            Serial.println("Data sent: " + payload);
        }
    }
}