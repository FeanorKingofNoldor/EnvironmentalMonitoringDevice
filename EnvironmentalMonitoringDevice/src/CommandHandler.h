#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "EventBus.h"

struct Command {
    String id;
    String type;
    String action;
    JsonObject params;
    String source; // "webapp" or "display"
    
    Command() = default;
    Command(const String& cmdType, const String& cmdAction, const String& cmdSource = "unknown") 
        : type(cmdType), action(cmdAction), source(cmdSource) {}
};

class CommandHandler {
private:
    void handleLightsCommand(const Command& cmd);
    void handleSprayCommand(const Command& cmd);
    void handleSystemCommand(const Command& cmd);
    void sendStatusConfirmation(const Command& cmd, const String& status, const String& message = "");
    
public:
    CommandHandler();
    
    void begin();
    void processCommand(const Command& cmd);
    void processCommand(const String& jsonCommand, const String& source = "unknown");
    
    // Event handlers
    void onCommandReceived(const Event& event);
};

extern CommandHandler commandHandler;

#endif