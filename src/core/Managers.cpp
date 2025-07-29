// File: src/core/Managers.cpp
#include "Managers.h"
#include "EventBus.h"
#include <WiFi.h>

// SensorManager Implementation
SensorManager::~SensorManager() {
    shutdown();
}

bool SensorManager::begin() {
    setState(ManagerState::INITIALIZING);
    initTime = millis();
    
    Serial.println("Initializing sensor manager...");
    
    if (!deviceCapabilities) {
        setError("Device capabilities not set");
        return false;
    }
    
    subscribeToEvents();
    
    // Load sensor configurations from config
    std::vector<SensorConfig> sensorConfigs = config.getSensors();
    
    for (const auto& sensorConfig : sensorConfigs) {
        if (!sensorConfig.enabled) {
            Serial.printf("Sensor %s disabled, skipping\n", sensorConfig.name.c_str());
            continue;
        }
        
        if (!addSensor(sensorConfig)) {
            Serial.printf("WARNING: Failed to add sensor: %s\n", sensorConfig.name.c_str());
        }
    }
    
    if (sensors.empty()) {
        setError("No sensors initialized");
        return false;
    }
    
    // Create sensor reading task
    xTaskCreatePinnedToCore(
        sensorTask,
        "SensorTask",
        4096,
        this,
        10, // Medium priority
        &sensorTaskHandle,
        1   // Pin to core 1
    );
    
    if (sensorTaskHandle == nullptr) {
        setError("Failed to create sensor task");
        return false;
    }
    
    setState(ManagerState::READY);
    Serial.printf("Sensor manager ready with %d sensors\n", sensors.size());
    return true;
}

void SensorManager::shutdown() {
    setState(ManagerState::SHUTDOWN);
    
    // Stop sensor task
    if (sensorTaskHandle != nullptr) {
        vTaskDelete(sensorTaskHandle);
        sensorTaskHandle = nullptr;
    }
    
    // Shutdown all sensors
    for (auto& sensor : sensors) {
        sensor->shutdown();
    }
    
    sensors.clear();
    lastReadings.clear();
    Serial.println("Sensor manager shutdown");
}

void SensorManager::update() {
    // This is called from main thread for periodic maintenance
    // Actual sensor reading happens in sensor task
}

bool SensorManager::addSensor(const SensorConfig& config) {
    auto sensor = createSensor(config);
    if (!sensor) {
        Serial.printf("Failed to create sensor: %s\n", config.name.c_str());
        return false;
    }
    
    if (!sensor->begin()) {
        Serial.printf("Failed to initialize sensor: %s\n", config.name.c_str());
        return false;
    }
    
    sensors.push_back(std::move(sensor));
    Serial.printf("Added sensor: %s\n", config.name.c_str());
    return true;
}

std::unique_ptr<BaseSensor> SensorManager::createSensor(const SensorConfig& config) {
    if (!deviceCapabilities) {
        return nullptr;
    }
    
    return deviceCapabilities->createSensor(config);
}

void SensorManager::subscribeToEvents() {
    eventBus.subscribe(CoreEventTypes::CONFIG_CHANGED, [this](const Event& event) {
        Serial.println("Config changed, reconfiguring sensors...");
        // TODO: Implement sensor reconfiguration
    });
}

void SensorManager::sensorTask(void* parameter) {
    SensorManager* manager = static_cast<SensorManager*>(parameter);
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(manager->readIntervalMs);
    
    while (true) {
        if (manager->getState() == ManagerState::READY) {
            manager->readAllSensors();
        }
        
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

bool SensorManager::readAllSensors() {
    if (sensors.empty()) return false;
    
    lastReadings.clear();
    bool allSuccess = true;
    
    for (auto& sensor : sensors) {
        if (!sensor->isReady()) {
            Serial.printf("Sensor %s not ready\n", sensor->getName().c_str());
            continue;
        }
        
        SensorReading reading = sensor->read();
        lastReadings.push_back(reading);
        
        if (!reading.valid) {
            Serial.printf("Invalid reading from %s: %s\n", 
                         sensor->getName().c_str(), reading.errorMessage.c_str());
            allSuccess = false;
        }
    }
    
    lastReadTime = millis();
    return allSuccess;
}

SensorReading SensorManager::getReading(const String& sensorName) const {
    for (const auto& reading : lastReadings) {
        if (reading.sensorName == sensorName) {
            return reading;
        }
    }
    
    // Return invalid reading if not found
    SensorReading invalidReading;
    invalidReading.sensorName = sensorName;
    invalidReading.errorMessage = "Sensor not found";
    return invalidReading;
}

std::vector<SensorReading> SensorManager::getAllReadings() const {
    return lastReadings;
}

bool SensorManager::areAllSensorsReady() const {
    for (const auto& sensor : sensors) {
        if (!sensor->isReady()) {
            return false;
        }
    }
    return true;
}

void SensorManager::printSensorStatus() const {
    Serial.println("=== Sensor Status ===");
    for (const auto& sensor : sensors) {
        String status = sensor->isReady() ? "READY" : "NOT READY";
        Serial.printf("  %s (%s): %s\n", 
                     sensor->getName().c_str(), 
                     sensor->getType().c_str(), 
                     status.c_str());
    }
    Serial.println("====================");
}

// ActuatorManager Implementation
ActuatorManager::~ActuatorManager() {
    shutdown();
}

bool ActuatorManager::begin() {
    setState(ManagerState::INITIALIZING);
    initTime = millis();
    
    Serial.println("Initializing actuator manager...");
    
    if (!deviceCapabilities) {
        setError("Device capabilities not set");
        return false;
    }
    
    subscribeToEvents();
    
    // Load actuator configurations from config
    std::vector<ActuatorConfig> actuatorConfigs = config.getActuators();
    
    for (const auto& actuatorConfig : actuatorConfigs) {
        if (!actuatorConfig.enabled) {
            Serial.printf("Actuator %s disabled, skipping\n", actuatorConfig.name.c_str());
            continue;
        }
        
        if (!addActuator(actuatorConfig)) {
            Serial.printf("WARNING: Failed to add actuator: %s\n", actuatorConfig.name.c_str());
        }
    }
    
    if (actuators.empty()) {
        Serial.println("WARNING: No actuators initialized");
    }
    
    setState(ManagerState::READY);
    Serial.printf("Actuator manager ready with %d actuators\n", actuators.size());
    return true;
}

void ActuatorManager::shutdown() {
    setState(ManagerState::SHUTDOWN);
    
    Serial.println("Shutting down all actuators...");
    emergencyStopAll();
    
    for (auto& actuator : actuators) {
        actuator->shutdown();
    }
    
    actuators.clear();
    Serial.println("Actuator manager shutdown");
}

void ActuatorManager::update() {
    // Update actuators that need periodic processing (like timed sprays)
    for (auto& actuator : actuators) {
        if (actuator->getType() == "VenturiNozzle") {
            // Call update method if actuator supports it
            // This is a bit of a hack - proper solution would be interface
            static_cast<VenturiNozzleActuator*>(actuator.get())->update();
        }
    }
}

bool ActuatorManager::addActuator(const ActuatorConfig& config) {
    auto actuator = createActuator(config);
    if (!actuator) {
        Serial.printf("Failed to create actuator: %s\n", config.name.c_str());
        return false;
    }
    
    if (!actuator->begin()) {
        Serial.printf("Failed to initialize actuator: %s\n", config.name.c_str());
        return false;
    }
    
    actuators.push_back(std::move(actuator));
    Serial.printf("Added actuator: %s\n", config.name.c_str());
    return true;
}

std::unique_ptr<BaseActuator> ActuatorManager::createActuator(const ActuatorConfig& config) {
    if (!deviceCapabilities) {
        return nullptr;
    }
    
    return deviceCapabilities->createActuator(config);
}

void ActuatorManager::subscribeToEvents() {
    // Subscribe to command events for actuator control
    eventBus.subscribe(CoreEventTypes::COMMAND_RECEIVED, [this](const Event& event) {
        // Parse command and execute actuator actions
        // TODO: Implement command parsing
    });
}

bool ActuatorManager::activateActuator(const String& name) {
    for (auto& actuator : actuators) {
        if (actuator->getName() == name) {
            return actuator->activate();
        }
    }
    Serial.printf("Actuator not found: %s\n", name.c_str());
    return false;
}

bool ActuatorManager::deactivateActuator(const String& name) {
    for (auto& actuator : actuators) {
        if (actuator->getName() == name) {
            return actuator->deactivate();
        }
    }
    Serial.printf("Actuator not found: %s\n", name.c_str());
    return false;
}

void ActuatorManager::emergencyStopAll() {
    Serial.println("EMERGENCY STOP: Deactivating all actuators");
    
    for (auto& actuator : actuators) {
        if (actuator->isActive()) {
            actuator->deactivate();
            Serial.printf("Emergency stopped: %s\n", actuator->getName().c_str());
        }
    }
    
    eventBus.publish(CoreEventTypes::SYSTEM_ERROR, "ActuatorManager", 
                    "{\"message\":\"Emergency stop activated\"}");
}

void ActuatorManager::printActuatorStatus() const {
    Serial.println("=== Actuator Status ===");
    for (const auto& actuator : actuators) {
        String status = actuator->isReady() ? "READY" : "NOT READY";
        String state = actuator->isActive() ? "ACTIVE" : "INACTIVE";
        Serial.printf("  %s (%s): %s, %s\n", 
                     actuator->getName().c_str(), 
                     actuator->getType().c_str(), 
                     status.c_str(), 
                     state.c_str());
    }
    Serial.println("======================");
}

// SystemMonitor Implementation
bool SystemMonitor::begin() {
    setState(ManagerState::INITIALIZING);
    initTime = millis();
    
    Serial.println("Initializing system monitor...");
    
    // Initialize metrics
    collectMetrics();
    
    setState(ManagerState::READY);
    Serial.println("System monitor ready");
    return true;
}

void SystemMonitor::shutdown() {
    setState(ManagerState::SHUTDOWN);
    Serial.println("System monitor shutdown");
}

void SystemMonitor::update() {
    unsigned long currentTime = millis();
    
    if (currentTime - lastHealthCheck >= healthCheckInterval) {
        collectMetrics();
        checkSystemHealth();
        publishMetrics();
        lastHealthCheck = currentTime;
    }
}

void SystemMonitor::collectMetrics() {
    currentMetrics.freeHeap = ESP.getFreeHeap();
    currentMetrics.totalHeap = ESP.getHeapSize();
    currentMetrics.minFreeHeap = ESP.getMinFreeHeap();
    currentMetrics.uptime = millis();
    
    if (WiFi.status() == WL_CONNECTED) {
        currentMetrics.wifiRSSI = WiFi.RSSI();
        currentMetrics.wifiIP = WiFi.localIP().toString();
    } else {
        currentMetrics.wifiRSSI = 0;
        currentMetrics.wifiIP = "";
    }
    
    currentMetrics.lastUpdateTime = millis();
}

void SystemMonitor::checkSystemHealth() {
    // Check memory health
    if (currentMetrics.freeHeap < 10000) { // Less than 10KB
        PUBLISH_SYSTEM_ERROR("SystemMonitor", "Low memory warning");
    }
    
    // Check WiFi health
    if (currentMetrics.wifiRSSI < -80) {
        PUBLISH_SYSTEM_ERROR("SystemMonitor", "Weak WiFi signal");
    }
}

void SystemMonitor::publishMetrics() {
    String metricsJson = "{";
    metricsJson += "\"free_heap\":" + String(currentMetrics.freeHeap) + ",";
    metricsJson += "\"total_heap\":" + String(currentMetrics.totalHeap) + ",";
    metricsJson += "\"uptime\":" + String(currentMetrics.uptime) + ",";
    metricsJson += "\"wifi_rssi\":" + String(currentMetrics.wifiRSSI);
    metricsJson += "}";
    
    eventBus.publish("system.metrics", "SystemMonitor", metricsJson);
}

bool SystemMonitor::isSystemHealthy() const {
    return currentMetrics.freeHeap > 10000 && // At least 10KB free
           currentMetrics.wifiRSSI > -80;      // Decent WiFi signal
}

void SystemMonitor::printSystemStatus() const {
    Serial.println("=== System Status ===");
    Serial.printf("Uptime: %lu ms\n", currentMetrics.uptime);
    Serial.printf("Free Heap: %u / %u bytes\n", currentMetrics.freeHeap, currentMetrics.totalHeap);
    Serial.printf("Min Free Heap: %u bytes\n", currentMetrics.minFreeHeap);
    Serial.printf("WiFi RSSI: %d dBm\n", currentMetrics.wifiRSSI);
    Serial.printf("WiFi IP: %s\n", currentMetrics.wifiIP.c_str());
    Serial.printf("System Healthy: %s\n", isSystemHealthy() ? "Yes" : "No");
    Serial.println("====================");
}

// DeviceCoordinator Implementation
DeviceCoordinator::~DeviceCoordinator() {
    shutdown();
}

bool DeviceCoordinator::begin() {
    setState(ManagerState::INITIALIZING);
    initTime = millis();
    
    Serial.println("Initializing device coordinator...");
    
    if (!deviceCapabilities) {
        setError("Device capabilities not set");
        return false;
    }
    
    // Initialize all managers
    sensorManager = new SensorManager();
    actuatorManager = new ActuatorManager();
    systemMonitor = new SystemMonitor();
    
    // Set device capabilities for all managers
    sensorManager->setDeviceCapabilities(deviceCapabilities);
    actuatorManager->setDeviceCapabilities(deviceCapabilities);
    
    // Initialize managers in order
    if (!systemMonitor->begin()) {
        setError("Failed to initialize system monitor");
        return false;
    }
    
    if (!sensorManager->begin()) {
        setError("Failed to initialize sensor manager");
        return false;
    }
    
    if (!actuatorManager->begin()) {
        setError("Failed to initialize actuator manager");
        return false;
    }
    
    setState(ManagerState::READY);
    Serial.println("Device coordinator ready");
    return true;
}

void DeviceCoordinator::shutdown() {
    setState(ManagerState::SHUTDOWN);
    
    if (actuatorManager) {
        actuatorManager->shutdown();
        delete actuatorManager;
        actuatorManager = nullptr;
    }
    
    if (sensorManager) {
        sensorManager->shutdown();
        delete sensorManager;
        sensorManager = nullptr;
    }
    
    if (systemMonitor) {
        systemMonitor->shutdown();
        delete systemMonitor;
        systemMonitor = nullptr;
    }
    
    Serial.println("Device coordinator shutdown");
}

void DeviceCoordinator::update() {
    if (systemMonitor && systemMonitor->isReady()) {
        systemMonitor->update();
    }
    
    if (actuatorManager && actuatorManager->isReady()) {
        actuatorManager->update();
    }
    
    if (sensorManager && sensorManager->isReady()) {
        sensorManager->update();
    }
}

bool DeviceCoordinator::isSystemReady() const {
    return getState() == ManagerState::READY &&
           (sensorManager ? sensorManager->isReady() : false) &&
           (actuatorManager ? actuatorManager->isReady() : false) &&
           (systemMonitor ? systemMonitor->isReady() : false);
}

void DeviceCoordinator::printSystemOverview() const {
    Serial.println("\n" + String('=', 50));
    Serial.println("SYSTEM OVERVIEW");
    Serial.println(String('=', 50));
    
    if (systemMonitor) systemMonitor->printSystemStatus();
    if (sensorManager) sensorManager->printSensorStatus();
    if (actuatorManager) actuatorManager->printActuatorStatus();
    
    Serial.println(String('=', 50) + "\n");
}

// Global instances
SensorManager sensorManager;
ActuatorManager actuatorManager;
SystemMonitor systemMonitor;
DeviceCoordinator deviceCoordinator;