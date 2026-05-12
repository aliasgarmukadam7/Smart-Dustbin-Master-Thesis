// =====================================
// SMART DUSTBIN - MAIN ENTRY POINT
// =====================================
//
// This file initialises all system components and starts
// FreeRTOS tasks.
//
// Architecture Overview:
// Sensors → Events → FSM → Commands → Actuators
//                ↓
//         MQTT / Web Interface
//
// =====================================
// === CORE INCLUDES ===
// Hardware abstraction and system configuration
#include <Arduino.h>
#include <WiFi.h>

// --- CONFIG & GLOBAL STATE ---
#include "config.h"
#include "globals.h"
#include "events.h"

// --- HARDWARE MODULES ---
#include "servo_control.h"
#include "ultrasonic_sensor.h"
#include "water_sensor.h"
#include "buzzer_control.h"
#include "ir_sensor.h"

// --- CORE LOGIC ---
#include "fsm.h"

// --- COMMUNICATION MODULES ---
#include "wifi_web.h"
#include "mqtt_app.h"

// === SYSTEM SETUP ===
void setup()
{   
    Serial.begin(115200);
    delay(2000);                 // Allows time for serial monitor connection.
    Serial.println("Smart Dustbin Started");

    // === SENSOR INITIALISATION === 
    sensor_init();               // Initialise global shared state + mutex
    ultrasonic_init();           // Ultrasonic sensor (Measures bin fill level)
    ir_init();                   // IR sensor (Detects user presence - hand detection)
    servo_init();                // Servo motor (Lid actuator)
    buzzer_init();               // Buzzer (Alerts)
    water_sensor_init();         // Water/Moisture detection
  

     // === Queue Creation (INTER-TASK COMMUNICATION) === 
    fsm_event_queue = xQueueCreate(10, sizeof(dustbin_event_t));
    servo_cmd_queue = xQueueCreate(5, sizeof(servo_cmd_t));
    buzzer_cmd_queue = xQueueCreate(5, sizeof(buzzer_cmd_t));
    // Used for asynchronous communication between tasks
    // Validate queue creation
    if (fsm_event_queue == NULL || servo_cmd_queue == NULL || buzzer_cmd_queue == NULL) {
        Serial.println("Failed to create queues");
        return;
    }

    // Ensure lid starts in a known safe state
    servo_write_deg(LID_CLOSED_DEG);
    
    // === NETWORK INITIALISATION ===

    wifi_web_start();   // Local web interface for monitoring/control
    mqtt_app_start();   // Cloud connectivity via MQTT broker 

    // === TASK CREATION (REAL-TIME SYSTEM) ===
    // Priority explanation: Higher number = higher priority

    xTaskCreate(fsm_task, "fsm_task", 4096, NULL, 5, NULL);                   // Main decision-making engine (state machine), (highest priority)
    xTaskCreate(servo_task, "servo_task", 2048, NULL, 5, NULL);               // Executes physical lid movement commands
    xTaskCreate(ir_task, "ir_task", 2048, NULL, 4, NULL);                     // Detects hand presence near bin
    xTaskCreate(ultrasonic_task, "ultrasonic_task", 3072, NULL, 4, NULL);     // Measures fill level of dustbin
    xTaskCreate(water_task, "water_task", 2048, NULL, 3, NULL);               // Detects moisture 
    xTaskCreate(buzzer_task, "buzzer_task", 2048, NULL, 3, NULL);             // Handles alarm notifications 
    xTaskCreate(mqtt_publish_task, "mqtt_publish_task", 3072, NULL, 3, NULL); // Sends sensor data to cloud server (Cloud communication)
    
}
// === MAIN LOOP ===
void loop()
{
    // Maintain MQTT connection and handle incoming messages
    mqtt_app_loop();

    // Small delay to allow FreeRTOS scheduler to run smoothly
    vTaskDelay(pdMS_TO_TICKS(10));
}