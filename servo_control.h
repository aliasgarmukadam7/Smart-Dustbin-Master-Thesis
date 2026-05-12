#pragma once

// === SERVO CONTROL INTERFACE ===
// Initialises PWM hardware for servo control.
void servo_init();

// Sets servo position in degrees (0–180).
// Internally converted to PWM pulse width.
void servo_write_deg(int deg);

// FreeRTOS task that:
// - Waits for commands (open/close)
// - Moves servo accordingly
// - Updates system state
// - Notifies FSM when action completes
void servo_task(void *arg);