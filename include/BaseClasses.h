// File: src/core/BaseClasses.h
#ifndef CORE_BASE_CLASSES_H
#define CORE_BASE_CLASSES_H

#include <Arduino.h>
#include <vector>
#include <memory>
#include "EventBus.h"

// Forward declarations
struct SensorConfig;
struct ActuatorConfig;

// Base sensor reading structure
struct SensorReading {
    String sensorName;
    String sensorType;
    float value;
    String unit;
    bool valid;
    unsigned long timestamp;
    String errorMessage;
    
    SensorReading() : sensorName(""), sensorType(""), value(0.0), unit(""), 
                     valid(false), timestamp(0), errorMessage("") {}
    
    SensorReading(const String& name, const String& type, float val, const String& u) 
        : sensorName(name), sensorType(type), value(val), unit(u), 
          valid(true), timestamp(millis()), errorMessage("") {}
};

// Base sensor configuration
struct SensorConfig {
    String name;
    String type;
    int pin;
    int i2cAddress;
    bool enabled;
    float calibrationOffset;
    float calibrationScale;
    unsigned long readIntervalMs;
    
    SensorConfig() : name(""), type(""), pin(-1), i2cAddress(0), enabled(false),
                    calibrationOffset(0.0), calibrationScale(1.0), readIntervalMs(1000) {}
};

// Base actuator configuration
struct ActuatorConfig {
    String name;
    String type;
    int pin;
    bool enabled;
    bool invertLogic;
    unsigned long pulseWidthMs;
    
    ActuatorConfig() : name(""), type(""), pin(-1), enabled(false),
                      invertLogic(false), pulseWidthMs(0) {}
};

// Abstract base sensor class
class BaseSensor {
protected:
    SensorConfig config;
    bool initialized;
    unsigned long lastReadTime;
    SensorReading lastReading;
    
public:
    BaseSensor(const SensorConfig& cfg) : config(cfg), initialized(false), lastReadTime(0) {}
    virtual ~BaseSensor() = default;
    
    // Pure virtual methods - must be implemented by device-specific sensors
    virtual bool begin() = 0;
    virtual SensorReading read() = 0;
    virtual void shutdown() = 0;
    
    // Common interface methods
    virtual bool isReady() const { return initialized; }
    virtual String getName() const { return config.name; }
    virtual String getType() const { return config.type; }
    virtual unsigned long getLastReadTime() const { return lastReadTime; }
    virtual SensorReading getLastReading() const { return lastReading; }
    
    // Configuration access
    virtual const SensorConfig& getConfig() const { return config; }
    virtual bool updateConfig(const SensorConfig& newConfig) {
        config = newConfig;
        return true; // Override if special handling needed
    }
    
    // Diagnostics
    virtual void printDiagnostics() const {
        Serial.printf("=== %s Sensor Diagnostics ===\n", config.name.c_str());
        Serial.printf("Type: %s\n", config.type.c_str());
        Serial.printf("Initialized: %s\n", initialized ? "Yes" : "No");
        Serial.printf("Last Read: %lu ms ago\n", millis() - lastReadTime);
        if (lastReading.valid) {
            Serial.printf("Last Value: %.2f %s\n", lastReading.value, lastReading.unit.c_str());
        } else {
            Serial.printf("Last Error: %s\n", lastReading.errorMessage.c_str());
        }
        Serial.println("===========================");
    }
};

// Abstract base actuator class
class BaseActuator {
protected:
    ActuatorConfig config;
    bool initialized;
    bool currentState;
    unsigned long lastActivationTime;
    
public:
    BaseActuator(const ActuatorConfig& cfg) : config(cfg), initialized(false), 
                                             currentState(false), lastActivationTime(0) {}
    virtual ~BaseActuator() = default;
    
    // Pure virtual methods
    virtual bool begin() = 0;
    virtual bool activate() = 0;
    virtual bool deactivate() = 0;
    virtual void shutdown() = 0;
    
    // Common interface methods
    virtual bool isReady() const { return initialized; }
    virtual bool isActive() const { return currentState; }
    virtual String getName() const { return config.name; }
    virtual String getType() const { return config.type; }
    virtual unsigned long getLastActivationTime() const { return lastActivationTime; }
    
    // Configuration access
    virtual const ActuatorConfig& getConfig() const { return config; }
    virtual bool updateConfig(const ActuatorConfig& newConfig) {
        config = newConfig;
        return true;
    }
    
    // Diagnostics
    virtual void printDiagnostics() const {
        Serial.printf("=== %s Actuator Diagnostics ===\n", config.name.c_str());
        Serial.printf("Type: %s\n", config.type.c_str());
        Serial.printf("Initialized: %s\n", initialized ? "Yes" : "No");
        Serial.printf("Current State: %s\n", currentState ? "Active" : "Inactive");
        Serial.printf("Last Activation: %lu ms ago\n", millis() - lastActivationTime);
        Serial.println("============================");
    }
};

// Device capability interface - implemented by device-specific classes
class IDeviceCapabilities {
public:
    virtual ~IDeviceCapabilities() = default;
    
    // Device info
    virtual String getDeviceType() const = 0;
    virtual String getDeviceName() const = 0;
    virtual String getFirmwareVersion() const = 0;
    
    // Sensor management
    virtual std::vector<String> getSupportedSensorTypes() const = 0;
    virtual std::unique_ptr<BaseSensor> createSensor(const SensorConfig& config) const = 0;
    
    // Actuator management
    virtual std::vector<String> getSupportedActuatorTypes() const = 0;
    virtual std::unique_ptr<BaseActuator> createActuator(const ActuatorConfig& config) const = 0;
    
    // Device-specific event types
    virtual std::vector<String> getDeviceEventTypes() const = 0;
    
    // Safety and validation
    virtual bool validateSensorConfig(const SensorConfig& config) const = 0;
    virtual bool validateActuatorConfig(const ActuatorConfig& config) const = 0;
};

// Manager state for diagnostics
enum class ManagerState {
    UNINITIALIZED,
    INITIALIZING,
    READY,
    ERROR,
    SHUTDOWN
};

// Base manager class with common functionality
class BaseManager {
protected:
    ManagerState state;
    String managerName;
    unsigned long initTime;
    String lastError;
    
public:
    BaseManager(const String& name) : state(ManagerState::UNINITIALIZED), 
                                     managerName(name), initTime(0), lastError("") {}
    
    virtual ~BaseManager() = default;
    
    // Common manager interface
    virtual bool begin() = 0;
    virtual void shutdown() = 0;
    virtual void update() {} // Optional periodic update
    
    // State management
    ManagerState getState() const { return state; }
    String getManagerName() const { return managerName; }
    String getLastError() const { return lastError; }
    unsigned long getUptime() const { return initTime > 0 ? millis() - initTime : 0; }
    
    bool isReady() const { return state == ManagerState::READY; }
    bool hasError() const { return state == ManagerState::ERROR; }
    
protected:
    void setState(ManagerState newState) { state = newState; }
    void setError(const String& error) { 
        lastError = error; 
        state = ManagerState::ERROR;
        PUBLISH_SYSTEM_ERROR(managerName, error);
    }
    void clearError() { 
        lastError = ""; 
        if (state == ManagerState::ERROR) {
            state = ManagerState::READY;
        }
    }
};

#endif // CORE_BASE_CLASSES_H