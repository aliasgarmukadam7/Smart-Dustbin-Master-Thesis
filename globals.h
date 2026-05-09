#pragma once

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// === SYSTEM STATE STRUCTURE ===
// Centralized data model representing the entire system state.
// All sensor readings and actuator states are stored here.
// This approach ensures: Consistency across modules, Easier debugging, Clean data sharing.

typedef struct {
    float distance_cm;     // Measured distance from ultrasonic sensor
    int fill_pct;          // Calculated bin fill percentage (0–100)

    int ir;                // IR sensor state (1 = hand detected)
    int lid_open;          // Lid status (1 = open, 0 = closed)

    int water_value;       // Raw analog water sensor value
    int water_detected;    // Water presence flag (1 = detected)

    int buzzer_on;         // Buzzer state
    int alert_active;      // Alert condition flag (e.g., overflow, water)
} system_state_t;

// === INTER-TASK COMMUNICATION (FreeRTOS) ===

// Queue for sending events to the Finite State Machine (FSM)
extern QueueHandle_t fsm_event_queue;

// Queue for sending commands to servo task
extern QueueHandle_t servo_cmd_queue;

// Queue for controlling buzzer task
extern QueueHandle_t buzzer_cmd_queue;

// === SYNCHRONIZATION ===

// Mutex to protect shared system state during read/write operations.
// Prevents race conditions between multiple tasks.
extern SemaphoreHandle_t g_sensor_mutex;

// === SYSTEM STATE MANAGEMENT API ===

// Initializes system state and synchronization primitives
void sensor_init();

// Updates global system state (thread-safe)
void sensor_update(const system_state_t *new_data);

// Retrieves a copy of current system state (thread-safe)
system_state_t sensor_get();