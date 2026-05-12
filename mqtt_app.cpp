// === MQTT COMMUNICATION MODULE ===

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include "config.h"        // MQTT broker details, topics
#include "globals.h"       // System state access
#include "servo_control.h"
#include "events.h"        // FSM event definitions
#include "mqtt_app.h"

// === MQTT Client Setup ===

// WiFi client used as transport layer
static WiFiClient espClient;

// MQTT client built on top of WiFi
static PubSubClient mqttClient(espClient);

// === Command Handling ===

// Processes incoming lid control commands from MQTT
// Converts string commands into FSM events
static void handle_lid_command(const char *cmd)
{
  dustbin_event_t ev;
  ev.timestamp_ms = millis();

  // --- OPEN COMMAND ---
  if (strcmp(cmd, "open") == 0) {
    ev.type = EVENT_MANUAL_OPEN;

    // Send event to FSM queue
    xQueueSend(fsm_event_queue, &ev, 0);

    Serial.println("MQTT: OPEN event");
  }

  // --- CLOSE COMMAND ---
  else if (strcmp(cmd, "close") == 0) {
    ev.type = EVENT_MANUAL_CLOSE;

    xQueueSend(fsm_event_queue, &ev, 0);

    Serial.println("MQTT: CLOSE event");
  }

  // --- UNKNOWN COMMAND ---
  else {
    Serial.print("MQTT: Unknown command: ");
    Serial.println(cmd);
  }
}

// === MQTT Callback (Incoming Messages) ===

// This function is automatically called whenever a message is received from a subscribed MQTT topic.
static void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
  char msg[128];

  // Ensure safe copy and null termination
  unsigned int copyLen = (length < sizeof(msg) - 1) ? length : sizeof(msg) - 1;

  memcpy(msg, payload, copyLen);
  msg[copyLen] = '\0';

  // Debug output
  Serial.print("MQTT: topic=");
  Serial.print(topic);
  Serial.print(" data=");
  Serial.println(msg);

  // Route message based on topic
  if (strcmp(topic, MQTT_TOPIC_CMD) == 0) {
    handle_lid_command(msg);
  }
}

// === MQTT Reconnection Logic ===

// Ensures the client stays connected to the broker.
// Automatically retries connection if disconnected.
static void mqtt_reconnect()
{
  while (!mqttClient.connected()) {

    Serial.println("MQTT: Connecting... ");

    // Create unique client ID using ESP32 MAC address
    String clientId = "SmartDustbin-";
    clientId += String((uint32_t)ESP.getEfuseMac(), HEX);

    // Connect with Last Will and Testament (LWT)
    // If device disconnects unexpectedly → broker publishes "offline"
    bool ok = mqttClient.connect(
      clientId.c_str(),
      MQTT_TOPIC_STATUS,   // LWT topic
      1,                   // QoS level
      true,                // Retain flag
      "offline"            // LWT message
    );

    if (ok) {
      Serial.println("connected");

      // Subscribe to command topic
      mqttClient.subscribe(MQTT_TOPIC_CMD, 1);

      // Announce device is online
      mqttClient.publish(MQTT_TOPIC_STATUS, "online", true);

      Serial.println("MQTT: subscribed to command topic");
    } 
    else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());

      Serial.println(" retrying in 2 sec");

      delay(2000); // retry delay
    }
  }
}

// === Public API: Initialisation ===

// Configures MQTT broker and callback handler
void mqtt_app_start()
{
  mqttClient.setServer(MQTT_BROKER_HOST, MQTT_BROKER_PORT);
  mqttClient.setCallback(mqtt_callback);
}

// === Public API: Main Loop ===
void mqtt_app_loop()
{
  // Do nothing if WiFi is not connected
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  // Reconnect if needed
  if (!mqttClient.connected()) {
    mqtt_reconnect(); 
  }

  // Process incoming/outgoing MQTT traffic
  mqttClient.loop();
}

// === Publish Sensor Data ===

// Sends current system state as JSON to MQTT broker.
// Optimization: publishes only when data changes.
void mqtt_publish_sensor_data()
{
  // Skip if not connected
  if (!mqttClient.connected()) {
    return;
  }

  char json[256];

  // Store last published state for comparison
  static system_state_t last = {0};

  // Get current state
  system_state_t s = sensor_get();

  // --- Change Detection ---
  // Avoid unnecessary network traffic
  if (memcmp(&s, &last, sizeof(system_state_t)) == 0) {
      return; // No change → skip publishing
  }

  last = s;

  // --- JSON Formatting ---
  snprintf(json, sizeof(json),
          "{"
          "\"distance_cm\":%.2f,"
          "\"fill_pct\":%d,"
          "\"ir\":%d,"
          "\"lid_open\":%d,"
          "\"water_value\":%d,"
          "\"water_detected\":%d,"
          "\"buzzer_on\":%d,"
          "\"alert_active\":%d"
          "}",
          s.distance_cm,
          s.fill_pct,
          s.ir,
          s.lid_open,
          s.water_value,
          s.water_detected,
          s.buzzer_on,
          s.alert_active
        );

  // Publish to MQTT topic
  mqttClient.publish(MQTT_TOPIC_DATA, json);
}

// === FreeRTOS Task: Periodic Publishing ===

// Runs in background and periodically publishes system state to MQTT broker.
void mqtt_publish_task(void *arg)
{
  while (1) {

    // Publish sensor data (if changed)
    mqtt_publish_sensor_data();

    // Run every 2 seconds
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}