#pragma once

// === FINITE STATE MACHINE (FSM) INTERFACE ===

// This task represents the "brain" of the system.
// It processes events and decides how the system reacts.

// Responsibilities:
// - Handle sensor-triggered events
// - Control lid (via servo)
// - Control buzzer (alerts & manual)
// - Manage system states (idle, open, full, etc.)
void fsm_task(void *arg);