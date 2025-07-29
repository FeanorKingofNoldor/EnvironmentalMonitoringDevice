#include "DisplayUARTHandler.h"
#include <WiFi.h>

DisplayUARTHandler displayUARTHandler;

DisplayUARTHandler::DisplayUARTHandler() 
    : displaySerial(&Serial2), lastTemperature(0), lastHumidity(0), 
      lastAirPressure(0), systemError(false) {}

void DisplayUARTHandler::begin() {
    // CRITICAL: Must match display firmware settings exactly
    displaySerial->begin(115200, SERIAL_8N1, 16, 17);  // RX=16, TX=17
    Serial.println("Display UART initialized");
    
    // Subscribe to sensor events for real-time data
    eventBus.subscribe("sensor.temperature", [this](const Event& event) {
        this->onSensorEvent(event);
    });
    eventBus.subscribe("sensor.humidity", [this](const Event& event) {
        this->onSensorEvent(event);
    });
    eventBus.subscribe("sensor.pressure", [this](const Event& event) {
        this->onSensorEvent(event);
    });
    
    // Subscribe to error events
    eventBus.subscribe("sensor.error", [this](const Event& event) {
        this->onErrorEvent(event);
    });
    eventBus.subscribe("system.error", [this](const Event& event) {
        this->onErrorEvent(event);
    });
}

void DisplayUARTHandler::processDisplayMessages() {
    while (displaySerial->available()) {
        String message = displaySerial->readStringUntil('\n');
        message.trim();
        
        if (message.length() > 0) {
            handleDisplayCommand(message);
        }
    }
}

void DisplayUARTHandler::handleDisplayCommand(const String& message) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
        Serial.printf("Display JSON parse error: %s\n", error.c_str());
        return;
    }
    
    String cmd = doc["cmd"];
    
    // REQUIRED: All main devices must respond to these
    if (cmd == "get_sensors") {
        sendSensorDataToDisplay();
    }
    else if (cmd == "get_status") {
        sendStatusToDisplay();
    }
    // Environment-specific manual commands
    else if (cmd == "manual_lights") {
        handleLightsCommand();
    }
    else if (cmd == "manual_spray") {
        handleSprayCommand();
    }
    else {
        Serial.printf("Unknown display command: %s\n", cmd.c_str());
    }
}

void DisplayUARTHandler::sendSensorDataToDisplay() {
    JsonDocument doc;
    
    // CRITICAL: Use these exact field names - display expects them
    doc["temp"] = getCurrentTemperature();           // float, degrees Celsius
    doc["humidity"] = getCurrentHumidity();          // float, percentage
    doc["air_pressure"] = getCurrentAirPressure();   // float, PSI
    
    String response;
    serializeJson(doc, response);
    displaySerial->println(response);
    
    // Debug output
    Serial.printf("Sent sensor data: %s\n", response.c_str());
}

void DisplayUARTHandler::sendStatusToDisplay() {
    JsonDocument doc;
    doc["status"] = hasSystemError() ? "error" : "ok";
    doc["wifi_connected"] = (WiFi.status() == WL_CONNECTED);
    
    // Optional: Report specific errors
    if (hasSystemError()) {
        doc["error"] = getLastErrorMessage();
    }
    
    String response;
    serializeJson(doc, response);
    displaySerial->println(response);
    
    Serial.printf("Sent status: %s\n", response.c_str());
}

void DisplayUARTHandler::handleLightsCommand() {
    // Publish command event to CommandHandler (same as webapp commands)
    eventBus.publish("command.received", "DisplayUARTHandler", 
                    "{\"type\":\"lights\",\"action\":\"toggle\",\"source\":\"display\"}");
    
    Serial.println("Display command: lights executed");
    
    // Optional: Send confirmation back to display
    JsonDocument doc;
    doc["cmd_response"] = "manual_lights";
    doc["status"] = "executed";
    String response;
    serializeJson(doc, response);
    displaySerial->println(response);
}

void DisplayUARTHandler::handleSprayCommand() {
    // Publish command event to CommandHandler (same as webapp commands)
    eventBus.publish("command.received", "DisplayUARTHandler", 
                    "{\"type\":\"spray\",\"action\":\"cycle\",\"source\":\"display\"}");
    
    Serial.println("Display command: spray executed");
    
    // Optional: Send confirmation back to display
    JsonDocument doc;
    doc["cmd_response"] = "manual_spray";
    doc["status"] = "executed";
    String response;
    serializeJson(doc, response);
    displaySerial->println(response);
}

float DisplayUARTHandler::getCurrentTemperature() {
    // Return cached sensor value (updated via events)
    return (lastTemperature > -999) ? lastTemperature : 0;
}

float DisplayUARTHandler::getCurrentHumidity() {
    // Return cached sensor value (updated via events)
    return (lastHumidity > -999) ? lastHumidity : 0;
}

float DisplayUARTHandler::getCurrentAirPressure() {
    // Return cached sensor value (updated via events)
    return (lastAirPressure > -999) ? lastAirPressure : 0;
}

bool DisplayUARTHandler::hasSystemError() {
    return systemError;
}

String DisplayUARTHandler::getLastErrorMessage() {
    return lastErrorMessage;
}

void DisplayUARTHandler::onSensorEvent(const Event& event) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, event.data);
    
    if (!error) {
        float value = doc["value"];
        
        if (event.type == "sensor.temperature") {
            lastTemperature = value;
        } else if (event.type == "sensor.humidity") {
            lastHumidity = value;
        } else if (event.type == "sensor.pressure") {
            lastAirPressure = value;
        }
    }
}

void DisplayUARTHandler::onErrorEvent(const Event& event) {
    systemError = true;
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, event.data);
    
    if (!error) {
        lastErrorMessage = doc["error"].as<String>();
    } else {
        lastErrorMessage = "System error detected";
    }
    
    Serial.printf("System error set: %s\n", lastErrorMessage.c_str());
}

void DisplayUARTHandler::updateSensorData(float temp, float hum, float press) {
    lastTemperature = temp;
    lastHumidity = hum;
    lastAirPressure = press;
}

void DisplayUARTHandler::setSystemError(const String& error) {
    systemError = true;
    lastErrorMessage = error;
}

void DisplayUARTHandler::clearSystemError() {
    systemError = false;
    lastErrorMessage = "";
}