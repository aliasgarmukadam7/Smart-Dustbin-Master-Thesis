#include <Arduino.h>
#include "config.h"        // Servo timing constants (pulse widths, frequency, etc.)
#include "globals.h"       // Shared system state and queues
#include "events.h"        // FSM event definitions
#include "servo_control.h"

// === Utility: Microseconds → PWM Duty Cycle ===

// Converts a pulse width (in microseconds) into a duty cycle value
// compatible with ESP32 LEDC PWM hardware.
static uint32_t us_to_duty(uint32_t us)
{
    return (us * SERVO_MAX_DUTY) / SERVO_PERIOD_US;
}

// === Low-Level Servo Control (Pulse Width) ===
// Writes a PWM pulse (in microseconds) to the servo.
// This is the fundamental control signal for servo positioning.
static void servo_write_us(uint32_t us)
{
    // Clamp pulse width to safe servo limits
    if (us < SERVO_MIN_US) us = SERVO_MIN_US;
    if (us > SERVO_MAX_US) us = SERVO_MAX_US;

    // Convert pulse width to PWM duty cycle
    uint32_t duty = us_to_duty(us);

    // Send PWM signal to servo pin
    ledcWrite(SERVO_GPIO, duty);
}

// === Initialisation ===
// Configures ESP32 LEDC PWM channel for servo control
void servo_init()
{
    // Attach GPIO pin to PWM channel with given frequency and resolution
    ledcAttach(SERVO_GPIO, SERVO_FREQ_HZ, SERVO_RESOLUTION);
}

// === High-Level Servo Control (Degrees) ===
// Converts angle (0–180°) into corresponding pulse width and sends it to the servo.
void servo_write_deg(int deg)
{
    // Clamp input angle to valid servo range
    if (deg < 0) deg = 0;
    if (deg > 180) deg = 180;

    // Linear mapping:
    // 0°   → SERVO_MIN_US
    // 180° → SERVO_MAX_US
    uint32_t us = SERVO_MIN_US +
                  (uint32_t)((SERVO_MAX_US - SERVO_MIN_US) * deg / 180);

    servo_write_us(us);
}

// === FreeRTOS Task: Servo Control ===

// This task waits for commands from a queue and controls the lid.
void servo_task(void *arg)
{
    servo_cmd_t cmd;

    while (1) {

        // Wait indefinitely for a command from queue
        if (xQueueReceive(servo_cmd_queue, &cmd, portMAX_DELAY)) {

            dustbin_event_t ev;
            ev.timestamp_ms = millis();

            // Retrieve current system state
            system_state_t serv = sensor_get();

            // --- OPEN LID ---
            if (cmd == SERVO_CMD_OPEN) {

                // Move servo to open position
                servo_write_deg(LID_OPEN_DEG);

                // Wait for physical movement to complete
                vTaskDelay(pdMS_TO_TICKS(700));

                // Update system state
                serv.lid_open = 1;
                sensor_update(&serv);

                // Notify FSM that action is complete
                ev.type = EVENT_SERVO_OPEN_DONE;
                xQueueSend(fsm_event_queue, &ev, 0);

                Serial.println("SERVO: Open done");
            }

            // --- CLOSE LID ---
            else if (cmd == SERVO_CMD_CLOSE) {

                servo_write_deg(LID_CLOSED_DEG);

                vTaskDelay(pdMS_TO_TICKS(700));

                serv.lid_open = 0;
                sensor_update(&serv);

                ev.type = EVENT_SERVO_CLOSE_DONE;
                xQueueSend(fsm_event_queue, &ev, 0);

                Serial.println("SERVO: Close done");
            }
        }
    }
}