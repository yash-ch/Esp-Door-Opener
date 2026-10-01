#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

// =====================
// WiFi
// =====================
const char* WIFI_SSID = "YOUR_WIFI"; //use 2.4ghz ones 
const char* WIFI_PASSWORD = "WIFI_PASSWORD";

// =====================
// MQTT
// =====================
const char* MQTT_SERVER = "broker.hivemq.com"; //you can use any other mqtt server aswell
const int MQTT_PORT = 1883;

const char* MQTT_TOPIC = "YourTopic/control";  //make sure that you change this so that anyone can't open your door

// =====================
// Servo
// =====================
#define SERVO_PIN 4  // change servo pin number as per your connection

Servo servo;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// =====================
// Servo timeout
// =====================
unsigned long lastMessageTime = 0;

const unsigned long SERVO_TIMEOUT = 2000;  // 2 seconds


// =====================
// Connect WiFi
// =====================
void connectWiFi() {

  Serial.print("Connecting to WiFi");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");

  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());
}


// =====================
// MQTT callback
// =====================
void mqttCallback(char* topic, byte* payload, unsigned int length) {

  String message = "";

  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("MQTT message: ");
  Serial.println(message);

  int angle = message.toInt();

  angle = constrain(angle, 0, 180);

  Serial.print("Moving servo to: ");
  Serial.println(angle);

  // Re-attach servo if it was detached
  if (!servo.attached()) {
    servo.attach(SERVO_PIN, 500, 2400);
    Serial.println("Servo attached");
  }
 
  servo.write(angle);
  delay(1000);
  servo.write(10);

  // Reset timeout
  lastMessageTime = millis();
}


// =====================
// Connect MQTT
// =====================
void connectMQTT() {

  while (!mqttClient.connected()) {

    Serial.print("Connecting to MQTT...");

    String clientID = "ESP32C3_Servo_";
    clientID += String((uint32_t)ESP.getEfuseMac(), HEX);

    if (mqttClient.connect(clientID.c_str())) {

      Serial.println("connected");

      mqttClient.subscribe(MQTT_TOPIC);

      Serial.print("Subscribed to: ");
      Serial.println(MQTT_TOPIC);

    } else {

      Serial.print("failed, rc=");
      Serial.println(mqttClient.state());

      delay(2000);
    }
  }
}


// =====================
// Setup
// =====================
void setup() {

  Serial.begin(115200);

  // Servo
  servo.setPeriodHertz(50);
  servo.attach(SERVO_PIN, 500, 2400);

  servo.write(10);

  // Start timeout timer
  lastMessageTime = millis();

  // WiFi
  connectWiFi();

  // MQTT
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);

  connectMQTT();
}


// =====================
// Loop
// =====================
void loop() {

  // WiFi reconnect
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  // MQTT reconnect
  if (!mqttClient.connected()) {
    connectMQTT();
  }

  mqttClient.loop();


  // =====================
  // Servo timeout
  // =====================

  if (servo.attached() &&
      millis() - lastMessageTime > SERVO_TIMEOUT) {

    Serial.println("No MQTT message - releasing servo");

    servo.detach();
  }
}