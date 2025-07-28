#include "SensorTask.h"

SensorTask sensorTask;

SensorTask::SensorTask() : taskHandle(nullptr), running(false) {}

SensorTask::~SensorTask() {
    stop();
}

void SensorTask::addSensor(ISensor* sensor) {
    if (sensor != nullptr) {
        sensors.push_back(sensor);
        Serial.printf("Added sensor: %s\n", sensor->getName().c_str());
    }
}

void SensorTask::begin() {
    if (running) {
        Serial.println("SensorTask already running");
        return;
    }
    
    // Initialize all sensors
    for (ISensor* sensor : sensors) {
        if (!sensor->begin()) {
            Serial.printf("Failed to initialize sensor: %s\n", sensor->getName().c_str());
        }
    }
    
    // Create the task
    xTaskCreate(
        taskWrapper,
        "SensorTask",
        4096,
        this,
        15, // High priority
        &taskHandle
    );
    
    running = true;
    Serial.println("SensorTask started");
}

void SensorTask::stop() {
    if (taskHandle != nullptr) {
        running = false;
        vTaskDelete(taskHandle);
        taskHandle = nullptr;
        Serial.println("SensorTask stopped");
    }
}

bool SensorTask::isRunning() const {
    return running;
}

void SensorTask::taskWrapper(void* parameter) {
    SensorTask* sensorTask = static_cast<SensorTask*>(parameter);
    sensorTask->task();
}

void SensorTask::task() {
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(1000); // Read every 1000ms
    
    while (running) {
        // Read all sensors
        for (ISensor* sensor : sensors) {
            if (sensor->isConnected()) {
                sensor->read();
            } else {
                // Try to reconnect failed sensors
                sensor->begin();
            }
        }
        
        // Wait for next cycle
        vTaskDelayUntil(&lastWakeTime, frequency);
    }
    
    vTaskDelete(nullptr);
}