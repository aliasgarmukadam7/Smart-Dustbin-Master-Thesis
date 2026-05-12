#pragma once

// === WiFi + Web Server Interface ===
// Initialises the WiFi connection and starts the web server
void wifi_web_start();
// Intended for handling web server tasks in loop-based systems
void wifi_web_loop();                             