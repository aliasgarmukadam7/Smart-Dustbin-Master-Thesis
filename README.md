# Smart Dustbin System (Master Thesis)

This project implements an IoT-based Smart Dustbin System using ESP32, FreeRTOS, MQTT, and a web-based dashboard.

Developed as part of a Master's Thesis project.

---

## Features

- Automatic lid opening using IR sensor
- Fill level detection using ultrasonic sensor
- Water detection
- MQTT communication
- Web dashboard interface
- Finite State Machine (FSM) architecture
- Modular FreeRTOS implementation

---

## Hardware Components

- ESP32
- Ultrasonic Sensor
- IR Sensor
- Servo Motor
- Water Sensor
- Buzzer

---

## Software Technologies

- C++
- Arduino Framework
- ESP32
- FreeRTOS
- MQTT
- HTML/CSS/JavaScript

---

## Project Structure

```text
data/
 ├── index.html
 ├── style.css
 └── app.js

SmartDustbin-rev2.ino

fsm.cpp / fsm.h
mqtt_app.cpp / mqtt_app.h
wifi_web.cpp / wifi_web.h
servo_control.cpp / servo_control.h
ultrasonic_sensor.cpp / ultrasonic_sensor.h
ir_sensor.cpp / ir_sensor.h
water_sensor.cpp / water_sensor.h
buzzer_control.cpp / buzzer_control.h
globals.cpp / globals.h
config.h
events.h
```

---

## Architecture

The system follows a Finite State Machine (FSM) architecture with sensor modules and MQTT communication tasks managed under FreeRTOS.

---

## Thesis Note

This repository contains the complete implementation developed for the Master's Thesis.

Only selected excerpts are included in the thesis appendix.

---

## Author

Aliasgar Imtiyaz Mukadam
