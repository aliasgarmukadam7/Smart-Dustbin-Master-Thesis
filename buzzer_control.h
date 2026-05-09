#pragma once

// === BUZZER CONTROL INTERFACE ===

// Initializes PWM for buzzer
void buzzer_init();

// Low-level control: turn buzzer ON (hardware)
void buzzer_on_hw();

// Low-level control: turn buzzer OFF
void buzzer_off_hw();

// FreeRTOS task:
// - Handles commands from FSM
// - Supports alert mode (beeping)
// - Updates system state
void buzzer_task(void *arg);