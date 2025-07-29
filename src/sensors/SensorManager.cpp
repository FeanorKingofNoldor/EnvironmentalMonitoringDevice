#include "SensorManager.h"
#include "SHT3xSensor.h"
#include "PressureTransducer.h"
#include "Logger.h"

SensorManager::SensorManager() : lastReadTime(0), isInitialized(false) {}

bool SensorManager::begin() {
    Logger::info("SensorMgr", "Initializing sensor manager...");
    
    // Subscribe to configuration events
    subscribeToEvents();
    
    // Load sensor configurations and create sensors
    std::vector<SensorConfig> sensorConfigs = config.getSensors();
    
    for (const auto& sensorConfig : sensorConfigs) {
        if (!sensorConfig.enabled) {
            Logger::debug("SensorMgr", "Sensor " + sensorConfig.name + " disabled, skipping");
            continue;
        }
        
        auto sensor = createSensor(sensorConfig);
        if (sensor && sensor->begin()) {
            Logger::info("SensorMgr", "Initialized sensor: " + sensorConfig.name);
            sensors.push_back(std::move(sensor));
        } else {
            Logger::error("SensorMgr", "Failed to initialize sensor: " + sensorConfig.name);
        }
    }
    
    if (sensors.empty()) {
        Logger::error("SensorMgr", "No sensors initialized");
        return false;
    }
    
    isInitialized = true;
    Logger::info("SensorMgr", "Sensor manager ready with " + String(sensors.size()) + " sensors");
    return true;
}

std::unique_ptr<BaseSensor> SensorManager::createSensor(const SensorConfig& config) {
    Logger::debug("SensorMgr", "Creating sensor: " + config.name + " (type: " + config.type + ")");
    
    if (config.type == "SHT3x" || config.type == "SHT30") {
        return std::make_unique<SHT30Sensor>(config);
    } else if (config.type == "Analog" && config.name == "pressure") {
        return std::make_unique<PressureTransducer>(config);
    } else {
        Logger::error("SensorMgr", "Unknown sensor type: " + config.type);
        return nullptr;
    }
}

void SensorManager::subscribeToEvents() {
    // Subscribe to configuration changes
    eventBus.subscribe("config.sensors.changed", [this](const Event& event) {
        Logger::info("SensorMgr", "Sensor configuration changed, reconfiguring...");
        reconfigure();
    });
}

bool SensorManager::readAllSensors() {
    if (!isInitialized) {
        Logger::error("SensorMgr", "Sensor manager not initialized");
        return false;
    }
    
    bool allSuccess = true;
    lastReadings.clear();
    
    for (auto& sensor : sensors) {
        if (!sensor->isReady()) {
            Logger::warn("SensorMgr", "Sensor " + sensor->getName() + " not ready");
            continue;
        }
        
        SensorReading reading = sensor->read();
        lastReadings.push_back(reading);
        
        if (reading.valid) {
            publishSensorEvent(reading);
            Logger::debug("SensorMgr", reading.sensorName + ": " + String(reading.value) + " " + reading.unit);
        } else {
            Logger::warn("SensorMgr", "Invalid reading from " + reading.sensorName);
            allSuccess = false;
        }
    }
    
    lastReadTime = millis();
    return allSuccess;
}

void SensorManager::publishSensorEvent(const SensorReading& reading) {
    String eventType;
    
    // Map sensor readings to event types
    if (reading.type == "temperature") {
        eventType = EventTypes::SENSOR_TEMPERATURE;
    } else if (reading.type == "humidity") {
        eventType = EventTypes::SENSOR_HUMIDITY;
    } else if (reading.type == "pressure") {
        eventType = EventTypes::SENSOR_PRESSURE;
    } else {
        eventType = "sensor." + reading.type;
    }
    
    // Create JSON data payload
    String data = "{";
    data += "\"sensor\":\"" + reading.sensorName + "\",";
    data += "\"value\":" + String(reading.value) + ",";
    data += "\"unit\":\"" + reading.unit + "\",";
    data += "\"timestamp\":" + String(reading.timestamp);
    data += "}";
    
    eventBus.publish(eventType, "SensorManager", data);
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
    invalidReading.valid = false;
    return invalidReading;
}

std::vector<SensorReading> SensorManager::getAllReadings() const {
    return lastReadings;
}

bool SensorManager::isAllSensorsReady() const {
    for (const auto& sensor : sensors) {
        if (!sensor->isReady()) {
            return false;
        }
    }
    return true;
}

void SensorManager::printSensorStatus() const {
    Logger::info("SensorMgr", "Sensor Status:");
    for (const auto& sensor : sensors) {
        String status = sensor->isReady() ? "READY" : "NOT READY";
        Logger::info("SensorMgr", "  " + sensor->getName() + ": " + status);
    }
}

bool SensorManager::reconfigure() {
    Logger::info("SensorMgr", "Reconfiguring sensors...");
    
    // Clear existing sensors
    sensors.clear();
    lastReadings.clear();
    
    // Reinitialize
    return begin();
}

void SensorManager::shutdown() {
    Logger::info("SensorMgr", "Shutting down sensor manager");
    sensors.clear();
    lastReadings.clear();
    isInitialized = false;
}