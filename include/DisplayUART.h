#ifndef DISPLAY_UART_H
#define DISPLAY_UART_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include "EventBus.h"

class DisplayUART {
private:
    HardwareSerial* serial;
    bool initialized;
    unsigned long lastSensorUpdate;
    unsigned long lastStatusUpdate;
    String receiveBuffer;
    
    static const int BAUD_RATE = 115200;
    static const int UPDATE_INTERVAL_MS = 2000; // 2 seconds
    static const int BUFFER_SIZE = 512;
    static const uint8_t RX_PIN = 16;
    static const uint8_t TX_PIN = 17;
    
    // Message processing
    void processIncomingData();
    void processMessage(const String& message);
    void handleDisplayCommand(const JsonObject& command);
    
    // Data sending
    void sendSensorData();
    void sendStatusData();
    void sendMessage(const JsonObject& message);
    
    // Event handlers
    void onSensorEvent(const Event& event);
    void onSystemEvent(const Event& event);
    
public:
    DisplayUART();
    ~DisplayUART();
    
    bool begin();
    void shutdown();
    void update();
    
    // Manual data sending
    void sendSensorReading(const String& sensor, float value, const String& unit);
    void sendSystemStatus(const String& status, const String& message = "");
    void sendError(const String& error);
    
    // Status
    bool isInitialized() const { return initialized; }
    unsigned long getLastUpdate() const { return lastSensorUpdate; }
};

#endif // DISPLAY_UART_H