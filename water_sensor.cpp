#include <Arduino.h>
#include "config.h"                                              // Contains pin definitions and threshold values
#include "globals.h"                                             // Shared system state access functions (sensor_get, sensor_update)
#include "water_sensor.h"

// === Water Sensor Initialization ===
// Configures the ADC (Analog-to-Digital Converter) for reading the water sensor.
void water_sensor_init()
{
    analogReadResolution(12);                                    // Set ADC resolution to 12 bits (0–4095 range). This provides higher precision for analog readings.
    analogSetPinAttenuation(WATER_GPIO, ADC_11db);               // Set attenuation to allow higher voltage range (~3.3V)
}

// === Water Sensor Reading Function ===
// Performs a single analog read from the water sensor pin
// Returns: raw ADC value (0–4095)
int water_sensor_read()
{
    return analogRead(WATER_GPIO);
}

// === FreeRTOS Task: Water Monitoring ===
// This task continuously monitors the water sensor.
// It runs independently (parallel to other tasks) using FreeRTOS.

void water_task(void *arg)
{
    while (1) {
        int water_value = water_sensor_read();                   // Read raw sensor value
        bool water_detected = (water_value > WATER_THRESHOLD);   // Determine if water is detected

        system_state_t Watersen = sensor_get();                  // Get current system state
        Watersen.water_value = water_value;                      // Update relevant fields
        Watersen.water_detected = water_detected ? 1 : 0;
        sensor_update(&Watersen);                                // Store updated state

        vTaskDelay(pdMS_TO_TICKS(500));                          // Delay 500 ms (task runs at 2 Hz)
    }
}