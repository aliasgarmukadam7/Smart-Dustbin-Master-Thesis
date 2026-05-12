#pragma once

// Initialises ultrasonic sensor pins and configuration
void ultrasonic_init();

// Measures distance in centimeters using ultrasonic sensor
float ultrasonic_read_cm();

// Converts measured distance to fill percentage
int distance_to_fill_percent(float cm);

// FreeRTOS task for continuous distance measurement
void ultrasonic_task(void *arg);