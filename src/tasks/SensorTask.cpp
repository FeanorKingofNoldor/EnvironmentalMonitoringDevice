#include "SensorTask.h"
#include "Logger.h"

TaskHandle_t SensorTask::taskHandle = nullptr;

bool SensorTask::start() {
    Logger::info("SensorTask", "Starting sensor task...");
    
    BaseType_t result = xTaskCreatePinnedToCore(
        taskFunction,
        "SensorTask",
        STACK_SIZE,
        nullptr,
        PRIORITY,
        &taskHandle,
        1  // Core 1
    );
    
    if (result != pdPASS) {
        Logger::error("SensorTask", "Failed to create sensor task");
        return false;
    }
    
    Logger::info("SensorTask", "Sensor task started successfully");
    return true;
}

void SensorTask::stop() {
    if (taskHandle != nullptr) {
        vTaskDelete(taskHandle);
        taskHandle = nullptr;
        Logger::info("SensorTask", "Sensor task stopped");
    }
}

bool SensorTask::isRunning() {
    return taskHandle != nullptr;
}

void SensorTask::taskFunction(void* parameter) {
    Logger::info("SensorTask", "Sensor task running");
    
    while (true) {
        performSensorReading();
        vTaskDelay(READ_INTERVAL);
    }
}

void SensorTask::performSensorReading() {
    Logger::debug("SensorTask", "Reading sensors...");
    // TODO: Integrate with SensorManager
}

void SensorTask::handleSensorErrors() {
    Logger::warn("SensorTask", "Handling sensor errors...");
}

void SensorTask::suspend() {
    if (taskHandle != nullptr) {
        vTaskSuspend(taskHandle);
    }
}

void SensorTask::resume() {
    if (taskHandle != nullptr) {
        vTaskResume(taskHandle);
    }
}

void SensorTask::setReadInterval(uint32_t intervalMs) {
    Logger::info("SensorTask", "Read interval set to " + String(intervalMs) + "ms");
}