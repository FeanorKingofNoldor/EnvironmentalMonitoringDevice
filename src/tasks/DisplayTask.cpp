#include "DisplayTask.h"
#include "Logger.h"
// #include "DisplayUART.h"  // Will be included when we integrate

TaskHandle_t DisplayTask::taskHandle = nullptr;

bool DisplayTask::start() {
    Logger::info("DisplayTask", "Starting display task...");
    
    BaseType_t result = xTaskCreatePinnedToCore(
        taskFunction,
        "DisplayTask",
        STACK_SIZE,
        nullptr,
        PRIORITY,
        &taskHandle,
        0  // Core 0 (communication core)
    );
    
    if (result != pdPASS) {
        Logger::error("DisplayTask", "Failed to create display task");
        return false;
    }
    
    Logger::info("DisplayTask", "Display task started successfully");
    return true;
}

void DisplayTask::stop() {
    if (taskHandle != nullptr) {
        vTaskDelete(taskHandle);
        taskHandle = nullptr;
        Logger::info("DisplayTask", "Display task stopped");
    }
}

bool DisplayTask::isRunning() {
    return taskHandle != nullptr;
}

void DisplayTask::taskFunction(void* parameter) {
    Logger::info("DisplayTask", "Display task running");
    
    while (true) {
        performDisplayOperations();
        vTaskDelay(TASK_INTERVAL);
    }
}

void DisplayTask::performDisplayOperations() {
    // TODO: Add display operations when DisplayUART is integrated
    // displayUART.update();
    
    Logger::debug("DisplayTask", "Display operations completed");
}

void DisplayTask::handleDisplayErrors() {
    Logger::warn("DisplayTask", "Handling display errors...");
    
    // TODO: Implement display error recovery
}

void DisplayTask::suspend() {
    if (taskHandle != nullptr) {
        vTaskSuspend(taskHandle);
        Logger::info("DisplayTask", "Display task suspended");
    }
}

void DisplayTask::resume() {
    if (taskHandle != nullptr) {
        vTaskResume(taskHandle);
        Logger::info("DisplayTask", "Display task resumed");
    }
}

void DisplayTask::setTaskInterval(uint32_t intervalMs) {
    Logger::info("DisplayTask", "Task interval set to " + String(intervalMs) + "ms");
    // TODO: Implement dynamic interval changes
}