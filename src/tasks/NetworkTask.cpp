#include "NetworkTask.h"
#include "Logger.h"
#include "NetworkUtils.h"
// #include "APIClient.h"  // Will be included when we integrate

TaskHandle_t NetworkTask::taskHandle = nullptr;

bool NetworkTask::start() {
    Logger::info("NetworkTask", "Starting network task...");
    
    BaseType_t result = xTaskCreatePinnedToCore(
        taskFunction,
        "NetworkTask",
        STACK_SIZE,
        nullptr,
        PRIORITY,
        &taskHandle,
        0  // Core 0 (communication core)
    );
    
    if (result != pdPASS) {
        Logger::error("NetworkTask", "Failed to create network task");
        return false;
    }
    
    Logger::info("NetworkTask", "Network task started successfully");
    return true;
}

void NetworkTask::stop() {
    if (taskHandle != nullptr) {
        vTaskDelete(taskHandle);
        taskHandle = nullptr;
        Logger::info("NetworkTask", "Network task stopped");
    }
}

bool NetworkTask::isRunning() {
    return taskHandle != nullptr;
}

void NetworkTask::taskFunction(void* parameter) {
    Logger::info("NetworkTask", "Network task running");
    
    while (true) {
        performNetworkOperations();
        vTaskDelay(TASK_INTERVAL);
    }
}

void NetworkTask::performNetworkOperations() {
    // Check WiFi connection and reconnect if needed
    if (!NetworkUtils::isConnected()) {
        Logger::warn("NetworkTask", "WiFi disconnected, attempting reconnection");
        NetworkUtils::handleReconnection();
    }
    
    // TODO: Add API operations when APIClient is integrated
    // if (NetworkUtils::isConnected()) {
    //     apiClient.pollCommands();
    //     apiClient.uploadSensorData();
    // }
    
    Logger::debug("NetworkTask", "Network operations completed");
}

void NetworkTask::handleNetworkErrors() {
    Logger::warn("NetworkTask", "Handling network errors...");
    
    // Attempt to reconnect WiFi
    if (!NetworkUtils::isConnected()) {
        NetworkUtils::connectWiFi();
    }
}

void NetworkTask::suspend() {
    if (taskHandle != nullptr) {
        vTaskSuspend(taskHandle);
        Logger::info("NetworkTask", "Network task suspended");
    }
}

void NetworkTask::resume() {
    if (taskHandle != nullptr) {
        vTaskResume(taskHandle);
        Logger::info("NetworkTask", "Network task resumed");
    }
}

void NetworkTask::setTaskInterval(uint32_t intervalMs) {
    Logger::info("NetworkTask", "Task interval set to " + String(intervalMs) + "ms");
    // TODO: Implement dynamic interval changes
}