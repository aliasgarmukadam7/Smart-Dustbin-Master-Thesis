// === GLOBAL SYSTEM STATE & SHARED RESOURCES ===

// Purpose:
// - Store system-wide data (sensor readings + actuator states)
// - Provide thread-safe access using a mutex
// - Define shared queues for inter-task communication
// This module is the central data layer of the system.

#include "globals.h"

// === FreeRTOS Queues (Inter-Task Communication) ===

// Queue for sending events to the Finite State Machine (FSM)
// Example: hand detected, bin full, lid opened, etc.
QueueHandle_t fsm_event_queue = NULL;

// Queue for sending commands to the servo task
// Example: open lid, close lid
QueueHandle_t servo_cmd_queue = NULL;

// Queue for controlling buzzer behavior
QueueHandle_t buzzer_cmd_queue = NULL;

// === Global System State (Shared Data) ===

// This struct holds the latest system state.
// It is accessed by multiple tasks (sensors, MQTT, web, FSM), therefore must be protected by a mutex.
system_state_t g_sensor = {

    .distance_cm = -1,     // Invalid initial distance
    .fill_pct = -1,        // Unknown fill level

    .ir = 0,               // No hand detected initially
    .lid_open = 0,         // Lid starts closed

    .water_value = 0,      
    .water_detected = 0,   

    .buzzer_on = 0,        
    .alert_active = 0      
};

// === Mutex for Thread Safety ===

// Protects access to g_sensor from concurrent tasks
SemaphoreHandle_t g_sensor_mutex = NULL;

// === Initialization ===
// Creates the mutex used for protecting shared state.
// Must be called before any task accesses g_sensor.
void sensor_init()
{
    g_sensor_mutex = xSemaphoreCreateMutex();
}

// === Update System State (Thread-Safe) ===
void sensor_update(const system_state_t *new_data)
{
    // Ensure mutex exists and acquire lock
    if (g_sensor_mutex && xSemaphoreTake(g_sensor_mutex, portMAX_DELAY)) {

        // Critical section: update shared state
        g_sensor = *new_data;

        // Release lock
        xSemaphoreGive(g_sensor_mutex);
    }
}

// === Read System State (Thread-Safe) ===
system_state_t sensor_get()
{
    system_state_t copy = {0};

    // Acquire mutex before reading shared data
    if (g_sensor_mutex && xSemaphoreTake(g_sensor_mutex, portMAX_DELAY)) {

        copy = g_sensor;

        // Release mutex after reading
        xSemaphoreGive(g_sensor_mutex);
    }

    return copy;
}