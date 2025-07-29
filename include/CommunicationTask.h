#ifndef COMMUNICATION_TASK_H
#define COMMUNICATION_TASK_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "EventBus.h"

struct SensorData {
    float temperature;
    float humidity;
    float pressure;
    unsigned long timestamp;
};

class CommunicationTask {
private:
    TaskHandle_t commandTaskHandle;
    TaskHandle_t sensorDataTaskHandle;
    bool running;
    String serverUrl;
    String deviceToken;
    SensorData latestSensorData;
    
    static void commandPollingTaskWrapper(void* parameter);
    static void sensorDataTaskWrapper(void* parameter);
    void commandPollingTask();
    void sensorDataTask();
    
    bool pollCommands();
    bool sendSensorData();
    void processCommand(const String& commandJson);
    String createSensorPayload();
    bool makeHttpRequest(const String& endpoint, const String& method, const String& payload = "", String* response = nullptr);
    
    // Event handlers
    void onSensorEvent(const Event& event);
    void onCommandStatus(const Event& event);
    
public:
    CommunicationTask();
    ~CommunicationTask();
    
    void begin(const String& serverUrl, const String& deviceToken);
    void stop();
    bool isRunning() const;
    void updateSensorData(float temp, float hum, float press);
};

extern CommunicationTask communicationTask;

#endif