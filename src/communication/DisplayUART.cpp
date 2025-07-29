#include "DisplayUART.h"
#include "Logger.h"

DisplayUART::DisplayUART() : serial(nullptr), initialized(false), 
                             lastSensorUpdate(0), lastStatusUpdate(0) {}

DisplayUART::~DisplayUART() {
    shutdown();
}

bool DisplayUART::begin() {
    Logger::info("DisplayUART", "Initializing display UART communication...");
    
    // Use Serial2 for display communication
    serial = &Serial2;
    serial->begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);
    
    // Subscribe to relevant events
    eventBus.subscribe(EventTypes::SENSOR_TEMPERATURE, [this](const Event& event) {
        onSensorEvent(event);
    });
    
    eventBus.subscribe(EventTypes::SENSOR_HUMIDITY, [this](const Event& event) {
        onSensorEvent(event);
    });
    
    eventBus.subscribe(EventTypes::SENSOR_PRESSURE, [this](const Event& event) {
        onSensorEvent(event);
    });
    
    eventBus.subscribe(EventTypes::SYSTEM_STARTUP, [this](const Event& event) {
        onSystemEvent(event);
    });
    
    eventBus.subscribe(EventTypes::SYSTEM_ERROR, [this](const Event& event) {
        onSystemEvent(event);
    });
    
    initialized = true;
    
    // Send initial status
    sendSystemStatus("startup", "AeroEnv system initializing");
    
    Logger::info("DisplayUART", "Display UART initialized on pins RX:" + String(RX_PIN) + " TX:" + String(TX_PIN));
    return true;
}

void DisplayUART::shutdown() {
    if (serial) {
        serial->end();
    }
    initialized = false;
    Logger::info("DisplayUART", "Display UART shutdown");
}

void DisplayUART::update() {
    if (!initialized) return;
    
    processIncomingData();
    
    unsigned long currentTime = millis();
    
    // Send periodic sensor updates
    if (currentTime - lastSensorUpdate > UPDATE_INTERVAL_MS) {
        sendSensorData();
        lastSensorUpdate = currentTime;
    }
    
    // Send periodic status updates (less frequent)
    if (currentTime - lastStatusUpdate > (UPDATE_INTERVAL_MS * 5)) {
        sendStatusData();
        lastStatusUpdate = currentTime;
    }
}

void DisplayUART::processIncomingData() {
    while (serial && serial->available()) {
        char c = serial->read();
        
        if (c == '\n' || c == '\r') {
            if (receiveBuffer.length() > 0) {
                processMessage(receiveBuffer);
                receiveBuffer = "";
            }
        } else if (receiveBuffer.length() < BUFFER_SIZE - 1) {
            receiveBuffer += c;
        }
    }
}

void DisplayUART::processMessage(const String& message) {
    Logger::debug("DisplayUART", "Received: " + message);
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
        Logger::warn("DisplayUART", "Invalid JSON from display: " + String(error.c_str()));
        return;
    }
    
    handleDisplayCommand(doc.as<JsonObject>());
}

void DisplayUART::handleDisplayCommand(const JsonObject& command) {
    String cmd = command["cmd"];
    
    if (cmd == "get_sensors") {
        sendSensorData();
    } else if (cmd == "get_status") {
        sendStatusData();
    } else if (cmd == "manual_lights") {
        eventBus.publish(EventTypes::ACTUATOR_LIGHTS_ON, "DisplayUART", "{}");
    } else if (cmd == "manual_spray") {
        eventBus.publish(EventTypes::ACTUATOR_SPRAY_START, "DisplayUART", "{}");
    } else {
        Logger::warn("DisplayUART", "Unknown command from display: " + cmd);
    }
}

void DisplayUART::sendSensorData() {
    JsonDocument doc;
    
    // TODO: Get actual sensor readings from SensorManager
    doc["temp"] = 0.0;
    doc["humidity"] = 0.0;
    doc["air_pressure"] = 0.0;
    
    sendMessage(doc.as<JsonObject>());
}

void DisplayUART::sendStatusData() {
    JsonDocument doc;
    doc["status"] = "ok";
    doc["wifi_connected"] = WiFi.status() == WL_CONNECTED;
    doc["uptime"] = millis();
    doc["free_heap"] = ESP.getFreeHeap();
    
    sendMessage(doc.as<JsonObject>());
}

void DisplayUART::sendMessage(const JsonObject& message) {
    if (!serial || !initialized) return;
    
    String jsonString;
    serializeJson(message, jsonString);
    
    serial->println(jsonString);
    Logger::debug("DisplayUART", "Sent: " + jsonString);
}

void DisplayUART::sendSensorReading(const String& sensor, float value, const String& unit) {
    JsonDocument doc;
    doc["sensor"] = sensor;
    doc["value"] = value;
    doc["unit"] = unit;
    doc["timestamp"] = millis();
    
    sendMessage(doc.as<JsonObject>());
}

void DisplayUART::sendSystemStatus(const String& status, const String& message) {
    JsonDocument doc;
    doc["status"] = status;
    if (!message.isEmpty()) {
        doc["message"] = message;
    }
    doc["timestamp"] = millis();
    
    sendMessage(doc.as<JsonObject>());
}

void DisplayUART::sendError(const String& error) {
    JsonDocument doc;
    doc["error"] = error;
    doc["timestamp"] = millis();
    
    sendMessage(doc.as<JsonObject>());
}

void DisplayUART::onSensorEvent(const Event& event) {
    // Parse sensor data from event and send to display
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, event.data);
    
    if (!error) {
        String sensor = doc["sensor"];
        float value = doc["value"];
        String unit = doc["unit"];
        
        sendSensorReading(sensor, value, unit);
    }
}

void DisplayUART::onSystemEvent(const Event& event) {
    if (event.type == EventTypes::SYSTEM_STARTUP) {
        sendSystemStatus("ready", "System initialized successfully");
    } else if (event.type == EventTypes::SYSTEM_ERROR) {
        sendError(event.data);
    }
}