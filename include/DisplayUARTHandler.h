#ifndef DISPLAY_UART_HANDLER_H
#define DISPLAY_UART_HANDLER_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include "EventBus.h"

class DisplayUARTHandler {
private:
    HardwareSerial* displaySerial;
    float lastTemperature;
    float lastHumidity;
    float lastAirPressure;
    bool systemError;
    String lastErrorMessage;
    
    void handleDisplayCommand(const String& message);
    void sendSensorDataToDisplay();
    void sendStatusToDisplay();
    void handleLightsCommand();
    void handleSprayCommand();
    
    // Sensor interface methods
    float getCurrentTemperature();
    float getCurrentHumidity();
    float getCurrentAirPressure();
    
    // Actuator interface methods
    void toggleLights();
    void executeSingleSprayCycle();
    
    // Error handling methods
    bool hasSystemError();
    String getLastErrorMessage();
    
    // Event handlers
    void onSensorEvent(const Event& event);
    void onErrorEvent(const Event& event);
    
public:
    DisplayUARTHandler();
    
    void begin();
    void processDisplayMessages();
    void updateSensorData(float temp, float hum, float press);
    void setSystemError(const String& error);
    void clearSystemError();
};

extern DisplayUARTHandler displayUARTHandler;

#endif