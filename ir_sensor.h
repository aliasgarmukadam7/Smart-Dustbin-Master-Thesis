#pragma once

// === INFRARED (IR) SENSOR INTERFACE ===

// Initialises the IR sensor GPIO pin.
void ir_init();

// FreeRTOS task that:
// - Continuously reads IR sensor
// - Detects hand presence/removal
// - Updates global system state
// - Sends events to FSM on state changes
void ir_task(void *arg);