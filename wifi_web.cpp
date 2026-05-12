#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "config.h".             // Contains WiFi credentials and configuration constants
#include "wifi_web.h"            // Interface for this module
#include "globals.h"             // Shared global variables (e.g., queues)
#include "events.h"              // Event definitions for FSM (finite state machine)


// === Web Server Initialisation ===
// Create an asynchronous web server object listening on port 80 (HTTP)
AsyncWebServer server(80);

// === Utility: Determine MIME Content Type ===
// This function maps file extensions to HTTP content types.
// It ensures the browser correctly interprets served files.

static String contentTypeFromFilename(const String &path)
{
    if (path.endsWith(".html")) return "text/html";
    if (path.endsWith(".css"))  return "text/css";
    if (path.endsWith(".js"))   return "application/javascript";
    if (path.endsWith(".json")) return "application/json";
    if (path.endsWith(".png"))  return "image/png";
    if (path.endsWith(".jpg"))  return "image/jpeg";
    if (path.endsWith(".jpeg")) return "image/jpeg";
    if (path.endsWith(".svg"))  return "image/svg+xml";
    if (path.endsWith(".ico"))  return "image/x-icon";

    // Default fallback for unknown file types
    return "text/plain";
}

// === API Endpoint: System Data ===
// Handles GET requests to "/data"
// Returns current sensor and system state in JSON format
static void handleData(AsyncWebServerRequest *request)
{
    char json[256];
    
    // Get latest sensor/system state (sensor readings, flags, etc.)
    system_state_t s = sensor_get();

    // Format system data into JSON string
    // This is consumed by the frontend (e.g., dashboard UI)
    snprintf(
        json,
        sizeof(json),
        "{\"distance_cm\":%.2f,\"fill_pct\":%d,\"ir\":%d,\"lid_open\":%d,"
        "\"water_value\":%d,\"water_detected\":%d,\"buzzer_on\":%d,\"alert_active\":%d}",
        s.distance_cm,
        s.fill_pct,
        s.ir,
        s.lid_open,
        s.water_value,
        s.water_detected,
        s.buzzer_on,
        s.alert_active
    );
    // Send HTTP response (status 200 = OK)
    request->send(200, "application/json", json);
}

// === API Endpoint: Manual Lid Control ===
// Handles POST request to "/lid/open"
// Sends an "open lid" event to the system FSM through a FreeRTOS queue.
static void handleLidOpen(AsyncWebServerRequest *request)
{
    dustbin_event_t ev;
    // Define event type and timestamp
    ev.type = EVENT_MANUAL_OPEN;
    ev.timestamp_ms = millis();
    
    // Send event to FSM queue (non-blocking)
    if (xQueueSend(fsm_event_queue, &ev, 0) != pdPASS) {
        Serial.println("FSM queue full (lid open)");
    }
    // Respond to client confirming action
    request->send(200, "application/json", "{\"ok\":1,\"cmd\":\"open\"}");
}

// Handles POST request to "/lid/close"
// Sends a "close lid" event to the FSM
static void handleLidClose(AsyncWebServerRequest *request)
{
    dustbin_event_t ev;
    ev.type = EVENT_MANUAL_CLOSE;
    ev.timestamp_ms = millis();

    if (xQueueSend(fsm_event_queue, &ev, 0) != pdPASS) {
        Serial.println("FSM queue full (lid close)");
    }

    request->send(200, "application/json", "{\"ok\":1,\"cmd\":\"close\"}");
}

// === API Endpoint: Alarm (Buzzer) Control ===
// Turns alarm ON via FSM event
static void handleAlarmOn(AsyncWebServerRequest *request)
{
    dustbin_event_t ev;
    ev.type = EVENT_BUZZER_ON;
    ev.timestamp_ms = millis();

    xQueueSend(fsm_event_queue, &ev, 0);

    request->send(200, "application/json", "{\"ok\":1,\"alarm\":\"on\"}");
}

// Turns alarm OFF via FSM event
static void handleAlarmOff(AsyncWebServerRequest *request)
{
    dustbin_event_t ev;
    ev.type = EVENT_BUZZER_OFF;
    ev.timestamp_ms = millis();

    xQueueSend(fsm_event_queue, &ev, 0);

    request->send(200, "application/json", "{\"ok\":1,\"alarm\":\"off\"}");
}

// === System Initialisation Function ===
// Initialises filesystem, connects to WiFi, and starts web server
void wifi_web_start()
{
    // --- Mount Filesystem ---
    // LittleFS is used to store web files (HTML, CSS, JS)
    Serial.println("WEB: Mounting LittleFS...");
    if (!LittleFS.begin(true)) {
        Serial.println("WEB: LittleFS mount failed");
        return;
    }
    Serial.println("WEB: LittleFS mounted");
    
    // --- Connect to WiFi ---
    Serial.printf("WEB: Connecting to WiFi SSID: %s\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);                    // Set device as station (client)                   
    WiFi.begin(WIFI_SSID, WIFI_PASS);       // Start connection

    unsigned long startMs = millis();

    // Wait up to 20 seconds for connection
    while (WiFi.status() != WL_CONNECTED && (millis() - startMs) < 20000UL) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    
    // Print connection result
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("WEB: WiFi connected, IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("WEB: WiFi connect timeout");
    }

    // API routes
    // Data endpoint (GET)
    server.on("/data", HTTP_GET, handleData);

    // Lid control (POST)
    server.on("/lid/open", HTTP_POST, handleLidOpen);
    server.on("/lid/close", HTTP_POST, handleLidClose);

    // Alarm control (POST)
    server.on("/alarm/on", HTTP_POST, handleAlarmOn);
    server.on("/alarm/off", HTTP_POST, handleAlarmOff);

    // --- Serve Web Interface ---

    // Root URL → serves main dashboard page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/index.html", "text/html");
    });

    // Serve all static files (CSS, JS, images) from filesystem
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    // Start server
    server.begin();
    Serial.println("WEB: HTTP server started");
}






