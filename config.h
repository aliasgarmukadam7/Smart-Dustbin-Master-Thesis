#pragma once

// === HARDWARE PIN CONFIGURATION ===
// Defines GPIO assignments for all sensors and actuators.
// Centralising this allows easy hardware changes without modifying application logic.

#define TRIG_GPIO   5    // Ultrasonic trigger pin (output)
#define ECHO_GPIO   17   // Ultrasonic echo pin (input)
#define IR_GPIO     32   // IR proximity sensor input
#define SERVO_GPIO  14   // Servo motor control pin (PWM)
#define WATER_GPIO  34   // Analog water sensor input
#define BUZZER_GPIO 13   // Buzzer output (PWM)

// === SERVO CONFIGURATION ===
// Defines PWM timing parameters for standard hobby servo.

#define SERVO_MIN_US 500        // Minimum pulse width (0°)
#define SERVO_MAX_US 2500       // Maximum pulse width (180°)

#define SERVO_FREQ_HZ 50        // Servo PWM frequency (50 Hz)
#define SERVO_CHANNEL 0         // LEDC channel
#define SERVO_RESOLUTION 13     // PWM resolution (bits)

#define SERVO_PERIOD_US 20000   // Period = 20 ms (standard servo)
#define SERVO_MAX_DUTY ((1 << SERVO_RESOLUTION) - 1)

// === BUZZER CONFIGURATION ===
// Uses PWM to generate audible tones.

#define BUZZER_CHANNEL 1
#define BUZZER_RESOLUTION 8
#define BUZZER_FREQ_HZ 2000     // 2 kHz tone

// === LID POSITIONS (SERVO ANGLES) ===

#define LID_CLOSED_DEG 0
#define LID_OPEN_DEG   90

// === SENSOR THRESHOLDS & CALIBRATION ===

#define WATER_THRESHOLD 1500     // Analog threshold for water detection

#define BIN_FULL_PERCENT 95      // Bin considered full above this %

#define ULTRASONIC_MIN_CM 2      // Valid measurement range
#define ULTRASONIC_MAX_CM 30

#define BIN_EMPTY_CM 24.5f       // Distance when bin is empty
#define BIN_FULL_CM 2.0f         // Distance when bin is full

// === WIFI CONFIGURATION ===

#define WIFI_SSID "YOUR_WIFI_NAME"
#define WIFI_PASS "YOUR_WIFI_PASSWORD"

// === SYSTEM TIMING ===

#define MANUAL_OVERRIDE_MS 5000UL   // Manual override duration

// === MQTT CONFIGURATION ===
// Defines broker connection and topics.

#define MQTT_BROKER_HOST "broker.hivemq.com"
#define MQTT_BROKER_PORT 1883

#define MQTT_TOPIC_DATA   "smartdustbin/data"
#define MQTT_TOPIC_STATUS "smartdustbin/status"
#define MQTT_TOPIC_CMD    "smartdustbin/cmd/lid"