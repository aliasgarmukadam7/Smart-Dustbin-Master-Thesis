#pragma once

// Initialises ADC settings for the water sensor
void water_sensor_init();

// Reads raw analog value from the water sensor
int water_sensor_read();

// FreeRTOS task that continuously monitors water level
void water_task(void *arg);