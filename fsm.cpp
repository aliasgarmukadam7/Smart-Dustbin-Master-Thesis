// === FINITE STATE MACHINE (FSM) ===
// Purpose:
// Controls overall system behavior using an event-driven design.

// Architecture:
// Events → FSM → Commands → Actuators

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "events.h"
#include "globals.h"
#include "fsm.h"

// === Global Control Flags ===

// Enables/disables buzzer functionality
bool buzzer_enabled = true;

// Indicates if alert condition (e.g., bin full) is active
bool alert_active = false;

// Tracks if buzzer is manually controlled (overrides automatic behavior)
bool buzzer_manual_mode = false;

// === FSM States ===

typedef enum {
    STATE_IDLE_CLOSED = 0,   // Lid closed, waiting for interaction
    STATE_OPENING_LID,       // Servo is opening lid
    STATE_LID_OPEN,          // Lid is open
    STATE_CLOSING_LID,       // Servo is closing lid
    STATE_BIN_FULL           // Bin is full (alert mode)
} dustbin_state_t;

// === Helper Functions (Command Dispatch) ===
// Sends command to servo task via queue
static void send_servo_cmd(servo_cmd_t cmd)
{
    if (xQueueSend(servo_cmd_queue, &cmd, 0) != pdPASS) {
        Serial.println("Servo queue full");
    }
}

// Sends command to buzzer task via queue
static void send_buzzer_cmd(buzzer_cmd_t cmd)
{
    if (xQueueSend(buzzer_cmd_queue, &cmd, 0) != pdPASS) {
        Serial.println("Buzzer queue full");
    }
}

// === FSM Task ===
void fsm_task(void *arg)
{
    dustbin_state_t state = STATE_IDLE_CLOSED; // Initial state
    dustbin_event_t event;

    uint64_t open_time_us = 0;   // Timestamp when lid was opened
    bool hand_present = false;   // Tracks IR sensor state
    bool auto_close_enabled = true; // Enables automatic closing

    Serial.println("FSM started");

    while (1) {

        // === EVENT PROCESSING ===

        if (xQueueReceive(fsm_event_queue, &event, pdMS_TO_TICKS(100))) {

            // --- GLOBAL EVENTS ---
            // These events are handled regardless of current state
            switch (event.type) {

                case EVENT_BUZZER_ON:
                    buzzer_manual_mode = true;
                    alert_active = false;
                    send_buzzer_cmd(BUZZER_CMD_ON);
                    Serial.println("FSM: BUZZER ON (manual)");
                    break;

                case EVENT_BUZZER_OFF:
                    buzzer_manual_mode = true;
                    alert_active = false;
                    send_buzzer_cmd(BUZZER_CMD_OFF);
                    Serial.println("FSM: BUZZER OFF (manual)");
                    break;

                case EVENT_HAND_DETECTED:
                    hand_present = true;
                    break;

                case EVENT_HAND_REMOVED:
                    hand_present = false;
                    break;

                default:
                    break;
            }

            Serial.printf("FSM: Event %d in state %d\n", event.type, state);

            // === STATE MACHINE LOGIC ===

            switch (state) {

                // --- IDLE (LID CLOSED) ---
                case STATE_IDLE_CLOSED:

                    // Automatic open (IR sensor)
                    if (event.type == EVENT_HAND_DETECTED) {
                        send_servo_cmd(SERVO_CMD_OPEN);
                        state = STATE_OPENING_LID;
                        auto_close_enabled = true;

                        Serial.println("FSM: Opening lid");
                    }

                    // Manual open (MQTT/Web)
                    else if (event.type == EVENT_MANUAL_OPEN) {
                        send_servo_cmd(SERVO_CMD_OPEN);
                        state = STATE_OPENING_LID;
                        auto_close_enabled = false;

                        Serial.println("FSM: Forcing lid to open");
                    }

                    // Bin full detected
                    else if (event.type == EVENT_BIN_FULL_DETECTED) {

                        state = STATE_BIN_FULL;

                        buzzer_manual_mode = false;
                        alert_active = true;

                        send_buzzer_cmd(BUZZER_CMD_ALERT_ON);

                        Serial.println("FSM: ALERT MODE ON");
                    }

                    break;

                // --- OPENING ---
                case STATE_OPENING_LID:

                    // Wait for servo confirmation
                    if (event.type == EVENT_SERVO_OPEN_DONE) {
                        state = STATE_LID_OPEN;

                        open_time_us = micros(); // Start auto-close timer

                        Serial.println("FSM: Lid open");
                    }

                    break;

                // --- LID OPEN ---
                case STATE_LID_OPEN:

                    // Bin becomes full while open
                    if (event.type == EVENT_BIN_FULL_DETECTED) {

                        send_buzzer_cmd(BUZZER_CMD_ALERT_ON);
                        alert_active = true;

                        Serial.println("FSM: ALERT MODE ON");

                        // Close lid immediately
                        send_servo_cmd(SERVO_CMD_CLOSE);
                        state = STATE_CLOSING_LID;
                    }

                    // Manual close
                    else if (event.type == EVENT_MANUAL_CLOSE) {
                        send_servo_cmd(SERVO_CMD_CLOSE);
                        state = STATE_CLOSING_LID;

                        Serial.println("FSM: Manual close");
                    }

                    break;

                // --- CLOSING ---
                case STATE_CLOSING_LID:

                    if (event.type == EVENT_SERVO_CLOSE_DONE) {
                        state = STATE_IDLE_CLOSED;

                        Serial.println("FSM: Lid closed");
                    }

                    break;

                // --- BIN FULL ---
                case STATE_BIN_FULL:

                    // Reset when bin is emptied
                    if (event.type == EVENT_BIN_EMPTIED) {

                        state = STATE_IDLE_CLOSED;

                        buzzer_manual_mode = false;

                        send_buzzer_cmd(BUZZER_CMD_ALERT_OFF);
                        send_buzzer_cmd(BUZZER_CMD_OFF);

                        Serial.println("FSM: Bin emptied → normal mode");
                    }

                    break;

                default:
                    break;
            }
        }

        // === TIME-BASED AUTO CLOSE ===
        // Automatically close lid after timeout if:
        // - Lid is open
        // - Auto-close is enabled
        // - No hand is detected
        if (state == STATE_LID_OPEN && auto_close_enabled && open_time_us != 0) {

            // 3 seconds timeout
            if ((micros() - open_time_us) > 3000000ULL && !hand_present) {

                send_servo_cmd(SERVO_CMD_CLOSE);
                state = STATE_CLOSING_LID;

                Serial.println("FSM: Auto-closing lid");
            }
        }
    }
}