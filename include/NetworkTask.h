#ifndef NETWORK_TASK_H
#define NETWORK_TASK_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class NetworkTask {
private:
    static TaskHandle_t taskHandle;
    static const uint32_t STACK_SIZE = 8192;
    static const UBaseType_t PRIORITY = 5; // Medium priority
    static const TickType_t TASK_INTERVAL = pdMS_TO_TICKS(5000); // 5 seconds
    
    static void taskFunction(void* parameter);
    static void performNetworkOperations();
    static void handleNetworkErrors();
    
public:
    static bool start();
    static void stop();
    static bool isRunning();
    
    // Task control
    static void suspend();
    static void resume();
    
    // Configuration
    static void setTaskInterval(uint32_t intervalMs);
};

#endif // NETWORK_TASK_H