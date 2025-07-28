#include "CommandHandler.h"

CommandHandler commandHandler;

CommandHandler::CommandHandler() {}

void CommandHandler::begin() {
    eventBus.subscribe("command.received", [this](const Event& event) {
        this->onCommandReceived(event);
    });
}

void CommandHandler::processCommand(const Command& cmd) {
    Serial.printf("Processing command: %s.%s from %s\n", 
                  cmd.type.c_str(), cmd.action.c_str(), cmd.source.c_str());
    
    if (cmd.type == "lights") {
        handleLightsCommand(cmd);
    } else if (cmd.type == "spray") {
        handleSprayCommand(cmd);
    } else if (cmd.type == "system") {
        handleSystemCommand(cmd);
    } else {
        Serial.printf("Unknown command type: %s\n", cmd.type.c_str());
        sendStatusConfirmation(cmd, "error", "Unknown command type");
    }
}

void CommandHandler::processCommand(const String& jsonCommand, const String& source) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonCommand);
    
    if (error) {
        Serial.printf("Failed to parse command JSON: %s\n", error.c_str());
        return;
    }
    
    Command cmd;
    cmd.id = doc["id"].as<String>();
    cmd.type = doc["type"].as<String>();
    cmd.action = doc["action"].as<String>();
    cmd.params = doc["params"];
    cmd.source = source;
    
    processCommand(cmd);
}

void CommandHandler::handleLightsCommand(const Command& cmd) {
    if (cmd.action == "on") {
        eventBus.publish("actuator.relay.set", "CommandHandler", 
                        "{\"relay\":\"lights\",\"state\":true}");
        sendStatusConfirmation(cmd, "completed", "Lights turned on");
    } else if (cmd.action == "off") {
        eventBus.publish("actuator.relay.set", "CommandHandler", 
                        "{\"relay\":\"lights\",\"state\":false}");
        sendStatusConfirmation(cmd, "completed", "Lights turned off");
    } else if (cmd.action == "toggle") {
        // Toggle lights (for display manual control)
        eventBus.publish("actuator.relay.toggle", "CommandHandler", 
                        "{\"relay\":\"lights\"}");
        sendStatusConfirmation(cmd, "completed", "Lights toggled");
    } else if (cmd.action == "schedule") {
        // TODO: Implement scheduling logic
        sendStatusConfirmation(cmd, "pending", "Lighting schedule set");
    } else {
        sendStatusConfirmation(cmd, "error", "Unknown lights action");
    }
}

void CommandHandler::handleSprayCommand(const Command& cmd) {
    if (cmd.action == "on") {
        eventBus.publish("actuator.spray.start", "CommandHandler", "{}");
        sendStatusConfirmation(cmd, "completed", "Spray started");
    } else if (cmd.action == "off") {
        eventBus.publish("actuator.spray.stop", "CommandHandler", "{}");
        sendStatusConfirmation(cmd, "completed", "Spray stopped");
    } else if (cmd.action == "cycle") {
        int nozzle = cmd.params["nozzle"] | 1;
        String data = "{\"nozzle\":" + String(nozzle) + "}";
        eventBus.publish("actuator.nozzle.activate", "CommandHandler", data);
        sendStatusConfirmation(cmd, "completed", "Spray cycle executed");
    } else {
        sendStatusConfirmation(cmd, "error", "Unknown spray action");
    }
}

void CommandHandler::handleSystemCommand(const Command& cmd) {
    if (cmd.action == "restart") {
        sendStatusConfirmation(cmd, "completed", "Restarting device");
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP.restart();
    } else {
        sendStatusConfirmation(cmd, "error", "Unknown system action");
    }
}

void CommandHandler::sendStatusConfirmation(const Command& cmd, const String& status, const String& message) {
    JsonDocument doc;
    doc["command_id"] = cmd.id;
    doc["status"] = status;
    doc["timestamp"] = millis() / 1000;
    if (!message.isEmpty()) {
        doc["message"] = message;
    }
    
    String payload;
    serializeJson(doc, payload);
    
    eventBus.publish("command.status", "CommandHandler", payload);
}

void CommandHandler::onCommandReceived(const Event& event) {
    processCommand(event.data, event.source);
}