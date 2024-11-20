#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

const char* ssid = "Recovery";
const char* password = "a1b2c3d4e5";
const char* mqttServer = "192.168.188.74";
const int mqttPort = 1883;
const char* mqttUser = "Shanmukh";
const char* mqttPassword = "mosquittosucks";

const char* ledTopic = "home/led";
const char* distanceTopic = "home/distance";
const char* intrusionTopic = "home/intrusion";
const char* temperatureTopic = "home/temperature";
const char* humidityTopic = "home/humidity";

const int trigPin = D2;
const int echoPin = D3;
const int ledPin = D1;
const int buzzerPin = D4;
const int dhtPin = D5;

#define DHTTYPE DHT11
DHT dht(dhtPin, DHTTYPE);

WiFiClient espClient;
PubSubClient client(espClient);

int distance;
bool intrusionDetected = false;

void setupWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }
}

void setupMQTT() {
  client.setServer(mqttServer, mqttPort);
  client.setCallback(mqttCallback);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, ledTopic) == 0) {
    if (payload[0] == '1') {
      digitalWrite(ledPin, HIGH);
    } else if (payload[0] == '0') {
      digitalWrite(ledPin, LOW);
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP8266_Client", mqttUser, mqttPassword)) {
      client.subscribe(ledTopic);
    } else {
      delay(5000);
    }
  }
}

int measureDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  return duration * 0.034 / 2;
}

void publishData() {
  distance = measureDistance();
  if (distance > 0) {
    client.publish(distanceTopic, String(distance).c_str());
    if (distance < 50) {
      if (!intrusionDetected) {
        intrusionDetected = true;
        client.publish(intrusionTopic, "Intrusion detected!");
        tone(buzzerPin, 1000);
      }
    } else {
      if (intrusionDetected) {
        intrusionDetected = false;
        client.publish(intrusionTopic, "Intrusion cleared");
        noTone(buzzerPin);
      }
    }
  }
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  if (!isnan(temp) && !isnan(hum)) {
    client.publish(temperatureTopic, String(temp).c_str());
    client.publish(humidityTopic, String(hum).c_str());
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  dht.begin();
  setupWiFi();
  setupMQTT();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  publishData();
  delay(2000);
}
