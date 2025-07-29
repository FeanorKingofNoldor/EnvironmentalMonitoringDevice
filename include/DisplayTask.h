#ifndef DISPLAY_TASK_H
#define DISPLAY_TASK_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class DisplayTask {
private:
    static TaskHandle_t taskHandle;
    static const uint32_t STACK_SIZE = 4096;
    static const UBaseType_t PRIORITY = 8; // High priority for UI responsiveness
    static const TickType_t TASK_INTERVAL = pdMS_TO_TICKS(100); // 100ms for responsive UI
    
    static void taskFunction(void* parameter);
    static void performDisplayOperations();
    static void handleDisplayErrors();
    
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

#endif // DISPLAY_TASK_H