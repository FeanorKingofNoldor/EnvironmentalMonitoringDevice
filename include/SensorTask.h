#ifndef SENSOR_TASK_H
#define SENSOR_TASK_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class SensorTask {
private:
    static TaskHandle_t taskHandle;
    static const uint32_t STACK_SIZE = 4096;
    static const UBaseType_t PRIORITY = 10; // High priority
    static const TickType_t READ_INTERVAL = pdMS_TO_TICKS(5000); // 5 seconds
    
    static void taskFunction(void* parameter);
    static void performSensorReading();
    static void handleSensorErrors();
    
public:
    static bool start();
    static void stop();
    static bool isRunning();
    
    // Task control
    static void suspend();
    static void resume();
    
    // Configuration
    static void setReadInterval(uint32_t intervalMs);
};

#endif // SENSOR_TASK_H