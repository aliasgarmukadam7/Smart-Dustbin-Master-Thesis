#include <Arduino.h>
#include "config.h"     // Contains IR_GPIO definition
#include "globals.h"    // Shared system state
#include "ir_sensor.h"
#include "events.h"     // FSM event definitions

// === Initialisation ===

// Configures IR sensor pin as input.
// The sensor outputs a digital signal (HIGH/LOW).
void ir_init()
{
    pinMode(IR_GPIO, INPUT);
}

// === FreeRTOS Task: IR Monitoring ===

// This task continuously monitors the IR sensor to detect the presence of a hand near the bin.
void ir_task(void *arg)
{  
    int last_ir = 0;   // Stores previous state (for edge detection)

    while (1) {

        // --- Read Sensor ---
        int raw = digitalRead(IR_GPIO);

        // --- Normalize Logic ---
        // Many IR sensors output LOW when object is detected.
        // This line inverts the logic:
        //   1 → hand detected
        //   0 → no hand
        int current = (raw == 0) ? 1 : 0;

        // ---- Update System State ---
        system_state_t Ir = sensor_get();

        Ir.ir = current;

        sensor_update(&Ir);

        // ---- Event Generation ---
        // Detect transitions instead of continuous triggering

        dustbin_event_t ev;
        ev.timestamp_ms = millis();

        // --- Rising Edge: Hand Detected ---
        // Transition from 0 → 1
        if (current == 1 && last_ir == 0) {
            ev.type = EVENT_HAND_DETECTED;

            xQueueSend(fsm_event_queue, &ev, 0);
        }

        // --- Falling Edge: Hand Removed ---
        // Transition from 1 → 0
        if (current == 0 && last_ir == 1) {
            ev.type = EVENT_HAND_REMOVED;

            xQueueSend(fsm_event_queue, &ev, 0);
        }

        // Store current state for next cycle
        last_ir = current;

        // --- Timing ---
        // Runs every 100 ms (10 Hz sampling rate)
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}