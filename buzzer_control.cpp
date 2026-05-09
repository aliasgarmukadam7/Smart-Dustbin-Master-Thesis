#include <Arduino.h>
#include "config.h"
#include "globals.h"
#include "buzzer_control.h"
#include "events.h"

// === Initialization ===
// Configures PWM channel for buzzer output
void buzzer_init()
{
  ledcAttach(BUZZER_GPIO, BUZZER_FREQ_HZ, BUZZER_RESOLUTION);

  // Ensure buzzer is OFF at startup
  ledcWrite(BUZZER_GPIO, 0);
}

// === Low-Level Hardware Control ===

// Turns buzzer ON using PWM tone generation
void buzzer_on_hw()
{
  ledcWriteTone(BUZZER_GPIO, BUZZER_FREQ_HZ);

  // Set duty cycle (~50%)
  ledcWrite(BUZZER_GPIO, 128);
}

// Turns buzzer OFF
void buzzer_off_hw()
{
  ledcWrite(BUZZER_GPIO, 0);
}

// === FreeRTOS Task: Buzzer Control ===
// Responsibilities:
// - Receive commands from FSM
// - Control buzzer (on/off/alert)
// - Implement non-blocking alert beeping
// - Update global system state
void buzzer_task(void *arg)
{
    buzzer_cmd_t cmd;

    bool alert_mode = false;   // Enables periodic beeping
    bool buzzer_on = false;    // Current hardware state

    Serial.println("BUZZER TASK STARTED");

    // --- Initial State ---
    system_state_t Buz = sensor_get();
    Buz.buzzer_on = 0;
    Buz.alert_active = 0;
    sensor_update(&Buz);

    // Timing for non-blocking blinking
    TickType_t last_toggle = 0;
    const TickType_t interval = pdMS_TO_TICKS(300); // 300 ms beep interval

    while (1) {

        // === HANDLE COMMANDS (NON-BLOCKING) ===

        if (xQueueReceive(buzzer_cmd_queue, &cmd, pdMS_TO_TICKS(100))) {

            Serial.printf("BUZZER CMD RECEIVED: %d\n", cmd);

            system_state_t Buz = sensor_get();

            switch (cmd) {

                // --- MANUAL ON ---
                case BUZZER_CMD_ON:
                    alert_mode = false;

                    buzzer_on_hw();
                    buzzer_on = true;

                    Buz.buzzer_on = 1;
                    Buz.alert_active = 0;
                    sensor_update(&Buz);

                    Serial.println("BUZZER: ON (manual)");
                    break;

                // --- OFF ---
                case BUZZER_CMD_OFF:
                    alert_mode = false;

                    buzzer_off_hw();
                    buzzer_on = false;

                    Buz.buzzer_on = 0;
                    Buz.alert_active = 0;
                    sensor_update(&Buz);

                    Serial.println("BUZZER: OFF");
                    break;

                // --- ALERT MODE ON ---
                case BUZZER_CMD_ALERT_ON:
                    alert_mode = true;

                    // Reset timer
                    last_toggle = xTaskGetTickCount();

                    Buz.alert_active = 1;
                    Buz.buzzer_on = 0;
                    sensor_update(&Buz);

                    Serial.println("BUZZER: ALERT START");
                    break;

                // --- ALERT MODE OFF ---
                case BUZZER_CMD_ALERT_OFF:
                    alert_mode = false;

                    buzzer_off_hw();
                    buzzer_on = false;

                    Buz.alert_active = 0;
                    Buz.buzzer_on = 0;
                    sensor_update(&Buz);

                    Serial.println("BUZZER: ALERT STOP");
                    break;
            }
        }

        // === ALERT MODE (NON-BLOCKING BLINK) ===

        if (alert_mode) {

            TickType_t now = xTaskGetTickCount();

            // Toggle buzzer at fixed interval
            if ((now - last_toggle) > interval) {

                last_toggle = now;

                if (buzzer_on) {
                    buzzer_off_hw();
                    buzzer_on = false;
                } else {
                    buzzer_on_hw();
                    buzzer_on = true;
                }
            }
        }

        // Small delay to yield CPU
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}