/*-------------------------------------------------------------
 * 
 *   .SYNOPSIS
 *   STALL SENSE (TM) - THERMAL CAMERA SENSOR
 *   Detect whether of not a bathroom stall is occupied
 *   based on thermal imagery of the stall.
 *   
 *   .NOTES
 *   Author: Charles Christensen
 *   
-------------------------------------------------------------*/

// Import libraries.
#include <FS.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_AMG88xx.h>

// Global varibles: serial.
const int serial_FREQ = 115200;
const bool serial_DEBUG = false;

// Global varibles: mqtt.
const int mqtt_RETRIES = 120;
char mqtt_host[64];
char mqtt_name[64];
WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);

// Global varibles: wifi.
const char* wifiparam_FILE = "/config.json";
const int wifiparam_SIZE = 1024;
const int wifi_TIMEOUT = 1000;
bool wifiparam_savepend = false;
WiFiManager wifi_manager;

// Global variables: stallsense.
const char* stallsense_NAME = "com.StallSense.ThermalSensor";
const char* stallsense_IDSUB = "/id";
const char* stallsense_IPSUB = "/ip";
const char* stallsense_RSTSUB = "/rst";
char stallsense_id[64];
char stallsense_ip[64];
char stallsense_rst[64];
const int stallsense_DELAY = 10000;
unsigned long stallsense_tmr = 0;

// Global variables: thermal.
const int thermal_ROWS = 8;
const int thermal_THRSHLD = 28;
const int thermal_CNSNTRT = 2;
float thermal_pixels[thermal_ROWS * thermal_ROWS];
Adafruit_AMG88xx thermal_client;
int thermal_state = 0;

// Global variables: light.
const int light_GNDPIN = 16;
const int light_REDPIN = 14;
const int light_GRNPIN = 13;

// Initialization.
void setup() {

  // Serial setup.
  Serial.begin(serial_FREQ);
  Serial.setDebugOutput(serial_DEBUG);

  // Wifiparam setup.
  if (!SPIFFS.begin()) {
    Serial.println("WP: Unable to mount filesystem.");
    wifiparamReset();
  }
  WiFiManagerParameter wifiparam_mqtt_host("mqtt_host", "mqtt host", "", 64);
  WiFiManagerParameter wifiparam_mqtt_name("mqtt_name", "mqtt name", "", 64);
  wifi_manager.setSaveConfigCallback(wifiparamCallback);
  wifi_manager.addParameter(&wifiparam_mqtt_host);
  wifi_manager.addParameter(&wifiparam_mqtt_name);

  // Wifi setup.
  char wifi_macaddr[18];
  WiFi.macAddress().toCharArray(wifi_macaddr, 18);
  wifi_manager.autoConnect(wifi_macaddr);
  
  if (wifiparam_savepend) { 
    strcpy(mqtt_host, wifiparam_mqtt_host.getValue());
    strcpy(mqtt_name, wifiparam_mqtt_name.getValue());
    wifiparamSave();
  }
  else 
    { wifiparamGet(); }

  // Stallsense setup.
  char tmp[64];
  strcpy(tmp, mqtt_name);
  strcat(tmp, stallsense_IDSUB);
  strcpy(stallsense_id, tmp);
  strcpy(tmp, mqtt_name);
  strcat(tmp, stallsense_IPSUB);
  strcpy(stallsense_ip, tmp);
  strcpy(tmp, mqtt_name);
  strcat(tmp, stallsense_RSTSUB);
  strcpy(stallsense_rst, tmp);

  // Thermal setup.
  if (!thermal_client.begin()) {
    Serial.println("Thermal: IR camera not connected correctly.");
    while (true);
  }

  // Light setup.
  pinMode(light_GNDPIN, OUTPUT);
  pinMode(light_REDPIN, OUTPUT);
  pinMode(light_GRNPIN, OUTPUT);
  digitalWrite(light_GNDPIN, LOW);
  lightUpdate();

  // MQTT setup.
  mqtt_client.setServer(mqtt_host, 1883);
  mqtt_client.setCallback(mqttCallback);
  mqttConnect();

}

// Main.
void loop() {

  // MQTT. Check for messages and reconnect MQTT, if needed.
  while (!mqtt_client.connected()) {
    mqttConnect();
  }
  mqtt_client.loop();

  // Stallsense. Do periodic updates.
  if (millis() > stallsense_tmr) {
    mqtt_client.publish(stallsense_id, stallsense_NAME);
    mqtt_client.publish(stallsense_ip, WiFi.localIP().toString().c_str());
    mqtt_client.publish(mqtt_name, String(thermal_state).c_str());
    stallsense_tmr = timerGet(stallsense_DELAY);
  }

  // Thermal. Do periodic checks.
  thermalUpdate();
  
}

// Light. Update stoplight status.
void lightUpdate() {

  // Clear lights.
  digitalWrite(light_REDPIN, LOW);
  digitalWrite(light_GRNPIN, LOW);

  // Turn on corresponding light.
  if (thermal_state == 0) {
    digitalWrite(light_GRNPIN, HIGH);
  }
  else {
    digitalWrite(light_REDPIN, HIGH);
  }
  
}

// Thermal. Take snapshot and update state.
void thermalUpdate() {

  thermal_client.readPixels(thermal_pixels);

  // Examine each sampled pixel, and print.
  int hot_count = 0;
  //Serial.println("");
  //Serial.println("");
  //Serial.print("Thermal: New Snapshot");
  for (int i=0; i<(thermal_ROWS * thermal_ROWS); i++) {
    if (i%thermal_ROWS == 0) {
      //Serial.println("");
    }
    //Serial.print(thermal_pixels[i]);
    //Serial.print(", ");
    if (thermal_pixels[i] >= thermal_THRSHLD) {
      hot_count++;
    }
  }
  //Serial.println("");

  // Update state, if needed.
  int state = 0;
  if (hot_count >= thermal_CNSNTRT) {
    state = 1;
  }
  if (thermal_state != state) {
    thermal_state = state;
    mqtt_client.publish(mqtt_name, String(thermal_state).c_str());
    lightUpdate();
    Serial.print("Thermal: State ");
    Serial.println(state);
  }
  
}

// Mqtt. Process arriving MQTT message.
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  
  // Print the MQTT message to serial.
  Serial.print("MQTT: ");
  Serial.print(topic);
  Serial.print(" = ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Adjust internal states, as determined by message.
  String topic_name(topic);
  payload[length] = '\0';

  // stallsense_reset
    if (topic_name.indexOf(stallsense_rst) >= 0) {
    if (strcmp((char*)payload, "1") == 0) {
      wifiparamReset();
    }
  }
  
}

// Mqtt. Connect to MQTT server.
void mqttConnect() {
  
  // Connect to MQTT server.
  int attempts = 0;
  while (true) {
    Serial.println("*MQ: Attempting MQTT connection.");
    if (mqtt_client.connect(mqtt_name)) {
      Serial.println("*MQ: Successfully connected to broker.");
      break;
    }
    else {
      Serial.println("*MQ: Connection failed. Retrying...");
      delay(wifi_TIMEOUT);
      attempts++;
    }
    if (attempts > mqtt_RETRIES) 
      { wifiparamReset(); }
  }

  // Subscribe to all relevant MQTT publishers.
  mqtt_client.subscribe(stallsense_rst);
  Serial.print("MQTT: Subscribed to ");
  Serial.println(stallsense_rst);
  
}

// Wifiparam. Callback to save config.
void wifiparamCallback() {
  wifiparam_savepend = true;
}

// Wifiparam. Save one-time config file.
void wifiparamSave() {
    DynamicJsonDocument wifiparam_doc(wifiparam_SIZE);
  
    // Define all WiFiManagerParameters in JSON.
    wifiparam_doc["mqtt_host"] = mqtt_host;
    wifiparam_doc["mqtt_name"] = mqtt_name;

    // Attempt to create, write, and save config info.
    File wifiparam_file = SPIFFS.open(wifiparam_FILE, "w");
    if (!wifiparam_file) {
      Serial.println("*WP: Unable to create config file.");
      wifiparamReset();
    }
    if (serializeJson(wifiparam_doc, wifiparam_file) == 0) {
      Serial.println("*WP: Unable to write config file.");
      wifiparamReset();
    }
    wifiparam_file.close();
}

// Wifiparam. Get one-time config settings.
bool wifiparamGet() {
  
    // Attempt to open config info.
    if (!SPIFFS.exists(wifiparam_FILE)) {
      Serial.println("*WP: Unable to find config file.");
      return false;
    }
    File wifiparam_file = SPIFFS.open(wifiparam_FILE, "r");
    if (!wifiparam_file) {
      Serial.println("*WP: Unable to open config file.");
      return false;
    }

    // Read information from config file.
    size_t size = wifiparam_file.size();
    std::unique_ptr<char[]> buf(new char[size]);
    wifiparam_file.readBytes(buf.get(), size);
    DynamicJsonDocument wifiparam_doc(size);
    deserializeJson(wifiparam_doc, buf.get());

    // Extract WiFiManagerParameters from config file.
    strcpy(mqtt_host, wifiparam_doc["mqtt_host"].as<char*>());
    strcpy(mqtt_name, wifiparam_doc["mqtt_name"].as<char*>());

    return true;    
}

// Wifiparam. Trigger a device reset.
void wifiparamReset() {
  delay(wifi_TIMEOUT);
  wifi_manager.resetSettings();
  SPIFFS.format();
  ESP.restart();
}

// System. Create timer value the specified number of millis in the future.
int timerGet(int millisecs) {
  if ( (millis() + millisecs) > 4294967294 )
    return millisecs - (4294967294-millis());
  else
    return millisecs + millis();
}
