#include <Arduino.h>
#include <WiFi.h>

// Core systems
#include "EventBus.h"
#include "Config.h"

// Managers
#include "SensorManager.h"
#include "ActuatorManager.h"
#include "APIClient.h"
#include "DisplayUART.h"

// Tasks
#include "SensorTask.h"
#include "NetworkTask.h"
#include "DisplayTask.h"

// Utils
#include "Logger.h"
#include "NetworkUtils.h"

// Global managers
SensorManager sensorManager;
ActuatorManager actuatorManager;
APIClient apiClient;
DisplayUART displayUART;

void setup() {
    Serial.begin(115200);
    delay(2000); // Allow serial to initialize
    
    Logger::init();
    Logger::info("AeroEnv", "Starting AeroEnv Environmental Controller");
    Logger::info("AeroEnv", "Firmware: " + String(__DATE__) + " " + String(__TIME__));
    
    // Initialize core systems
    if (!initializeCoreSystem()) {
        Logger::error("AeroEnv", "Core system initialization failed");
        while(true) delay(1000);
    }
    
    // Initialize hardware
    if (!initializeHardware()) {
        Logger::error("AeroEnv", "Hardware initialization failed");
        while(true) delay(1000);
    }
    
    // Initialize communication
    if (!initializeCommunication()) {
        Logger::error("AeroEnv", "Communication initialization failed");
        while(true) delay(1000);
    }
    
    // Start system tasks
    startSystemTasks();
    
    Logger::info("AeroEnv", "System initialization complete");
    eventBus.publish(EventTypes::SYSTEM_STARTUP, "main", "{}");
}

void loop() {
    // Everything runs in FreeRTOS tasks
    vTaskDelay(pdMS_TO_TICKS(1000));
}

bool initializeCoreSystem() {
    Logger::info("Core", "Initializing core systems...");
    
    if (!config.begin()) {
        Logger::error("Core", "Configuration system failed");
        return false;
    }
    
    Logger::info("Core", "Core systems ready");
    return true;
}

bool initializeHardware() {
    Logger::info("Hardware", "Initializing hardware components...");
    
    if (!sensorManager.begin()) {
        Logger::error("Hardware", "Sensor manager initialization failed");
        return false;
    }
    
    if (!actuatorManager.begin()) {
        Logger::error("Hardware", "Actuator manager initialization failed");
        return false;
    }
    
    Logger::info("Hardware", "Hardware components ready");
    return true;
}

bool initializeCommunication() {
    Logger::info("Comm", "Initializing communication systems...");
    
    if (!NetworkUtils::connectWiFi()) {
        Logger::warn("Comm", "WiFi connection failed - will retry in background");
    }
    
    if (!apiClient.begin()) {
        Logger::error("Comm", "API client initialization failed");
        return false;
    }
    
    if (!displayUART.begin()) {
        Logger::error("Comm", "Display UART initialization failed");
        return false;
    }
    
    Logger::info("Comm", "Communication systems ready");
    return true;
}

void startSystemTasks() {
    Logger::info("Tasks", "Starting system tasks...");
    
    SensorTask::start();
    NetworkTask::start();
    DisplayTask::start();
    
    Logger::info("Tasks", "All tasks started");
}