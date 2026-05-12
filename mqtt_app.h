#pragma once

// === MQTT APPLICATION INTERFACE ===
// Initialises MQTT client and connects to broker.
// Should be called after WiFi is connected.
void mqtt_app_start();

// Handles MQTT client loop processing.
// Required for maintaining connection and handling messages.
void mqtt_app_loop();

// Publishes current sensor/system state to MQTT topic.
// Typically called periodically or on state change.
void mqtt_publish_sensor_data();

// FreeRTOS task for periodic MQTT publishing.
// Runs in background and sends data at fixed intervals.
void mqtt_publish_task(void *arg);