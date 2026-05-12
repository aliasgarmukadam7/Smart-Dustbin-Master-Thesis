// === ULTRASONIC SENSOR ===
// Measures distance to waste surface and converts it into fill percentage

#include <Arduino.h>
#include "config.h"               // Pin definitions and calibration constants
#include "globals.h"              // Shared system state (sensor data)
#include "events.h"               // Event types for FSM communication
#include "ultrasonic_sensor.h"

// === Initialisation ===

// Configures GPIO pins for the ultrasonic sensor.
void ultrasonic_init()
{
    pinMode(TRIG_GPIO, OUTPUT);   // Trigger pin sends ultrasonic pulse
    digitalWrite(TRIG_GPIO, LOW); // Ensure clean LOW start state
    pinMode(ECHO_GPIO, INPUT);    // Echo pin receives reflected signal
}

// === Distance Measurement ===
// Send ultrasonic pulse and measure return time

float ultrasonic_read_cm()        
{
    // --- Trigger Pulse ---
    // A 10 µs HIGH pulse initiates the ultrasonic burst
    digitalWrite(TRIG_GPIO, LOW);
    delayMicroseconds(2);

    digitalWrite(TRIG_GPIO, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_GPIO, LOW);

    // --- Echo Measurement ---
    // pulseIn measures how long the ECHO pin stays HIGH
    // Timeout is set to 30 ms to avoid blocking indefinitely
    unsigned long duration = pulseIn(ECHO_GPIO, HIGH, 30000UL);
     
    // If no echo is received within timeout → invalid reading 
    if (duration == 0) {
        return -1.0f;
    }
    
    // --- Time → Distance Conversion ---
    // Speed of sound ≈ 0.0343 cm/µs
    // Divide by 2 because signal travels to object and back
    return (duration * 0.0343f) / 2.0f;
}

// === Distance → Fill Level Conversion ===
// Convert measured distance into bin fill percentage
int distance_to_fill_percent(float cm)
{
    const float empty_cm = BIN_EMPTY_CM;
    const float full_cm  = BIN_FULL_CM;

    if (cm <= 0) return -1;              // Invalid measurement
    if (cm >= empty_cm) return 0;        // Bin is empty (object far away)
    if (cm <= full_cm) return 100;       // Bin is full (object very close)

    float pct = (empty_cm - cm) * 100.0f / (empty_cm - full_cm);   // Linear interpolation between empty and full
    return (int)(pct + 0.5f);            // Round to nearest integer
}

// === FreeRTOS Task: Continuous Monitoring ===
// Continuous sensor task
void ultrasonic_task(void *arg)
{
    bool was_full = false;               // Tracks previous bin state

    while (1) {
        float cm = ultrasonic_read_cm();                                      // Read Distance
        bool cm_valid = (cm > ULTRASONIC_MIN_CM && cm < ULTRASONIC_MAX_CM);   // Validate measurement range
        int fill = cm_valid ? distance_to_fill_percent(cm) : -1;              // Convert to Fill Level
        bool is_full = (fill >= BIN_FULL_PERCENT && fill != -1);              // Determine if bin is considered "full"

        // Update global system state
        system_state_t Ultra = sensor_get();
        Ultra.distance_cm = cm_valid ? cm : -1;
        Ultra.fill_pct = fill;
        sensor_update(&Ultra);
        
        // --- Event Generation ---
        // Detect transitions to avoid repeated event triggering
        dustbin_event_t ev;
        uint32_t now_ms = millis();

        // Transition: NOT FULL → FULL 
        if (is_full && !was_full) {
            ev.type = EVENT_BIN_FULL_DETECTED;
            ev.timestamp_ms = now_ms;
            xQueueSend(fsm_event_queue, &ev, 0);   // Send event to FSM (non-blocking)
        } 

        // Transition: FULL → NOT FULL (bin emptied)
        else if (!is_full && was_full) {
            ev.type = EVENT_BIN_EMPTIED;
            ev.timestamp_ms = now_ms;
            xQueueSend(fsm_event_queue, &ev, 0);
        }
        
        // Update previous state
        was_full = is_full;

        vTaskDelay(pdMS_TO_TICKS(500));           // Task runs every 500 ms (2 Hz sampling rate)
    }
}