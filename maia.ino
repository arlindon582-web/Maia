#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// --- UPDATE THESE VALUES! ---
const char* ssid          = "RAUL";
const char* password      = "raulraul";
const char* mqtt_server   = "b8f97f54560a45f8ae04c7624c7d7cd4.s1.eu.hivemq.cloud";
const char* mqtt_user     = "ufrb.maia.esp";
const char* mqtt_password = "Ma1a@hivemq";
const int   mqtt_port     = 8883; // Use 8883 for secure connection

// --- MQTT and Sensor Setup ---
const char* temperature_topic = "esp8266/temperature"; // The "address" for our data
const char* led_topic = "esp8266/led";

#define LED_PIN D2
#define DHT11_PIN    D3 // Pin where the DHT11 data line is connected
#define ONE_WIRE_BUS D4 // Pin where the DS18B20 data line is connected
#define TURB_PIN A0 // Pin where the turbidity sensor is connected


// Setup instances
WiFiClientSecure espClient; // Use WiFiClientSecure for port 8883
PubSubClient client(espClient);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';  // Make payload a string
  String msg = String((char*)payload);

  if (String(topic) == led_topic) {
    if (msg == "ON") {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("LED turned ON");
    } else if (msg == "OFF") {
      digitalWrite(LED_PIN, LOW);
      Serial.println("LED turned OFF");
    }
  }
}


void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(led_topic);  // <--- SUBSCRIBE HERE
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(30000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  sensors.begin(); // Start up the sensor library
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);


  // This line is important for secure connections without certificate validation
  espClient.setInsecure(); 
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Read and send data every 30 seconds
  static unsigned long lastMsg = 0;
  if (millis() - lastMsg > 30000) {
    lastMsg = millis();

    sensors.requestTemperatures(); // Command to get temperatures
    float tempC = sensors.getTempCByIndex(0); // Get temperature in Celsius
    int turbidezRaw = analogRead(TURB_PIN);


    if (tempC == DEVICE_DISCONNECTED_C) {
      Serial.println("Error: Could not read from DS18B20 sensor.");
      return;
    }
    
    Serial.print("Temperature: ");
    Serial.print(tempC);
    Serial.println(" °C");
    Serial.print("Turbidez (Raw): ");
    Serial.print(turbidezRaw);
    Serial.println(" %");


    // Convert float to a char array to publish
    char tempString[8];
    dtostrf(tempC, 1, 2, tempString); // 1 decimal place, 2 characters wide for the value
    char turbString[8];
    dtostrf(turbidezRaw, 1, 0, turbString);
    
    client.publish(temperature_topic, tempString);
    client.publish("esp8266/turbidez", turbString);
  }
}