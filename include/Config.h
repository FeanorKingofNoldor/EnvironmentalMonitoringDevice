// File: src/core/Config.h
#ifndef CORE_CONFIG_H
#define CORE_CONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <vector>
#include "BaseClasses.h"
#include "EventBus.h"

// Network configuration
struct NetworkConfig {
    String wifiSSID;
    String wifiPassword;
    String serverURL;
    String deviceToken;
    String deviceName;
    unsigned long commandPollIntervalMs;
    unsigned long dataUploadIntervalMs;
    unsigned long connectionTimeoutMs;
    
    NetworkConfig() : wifiSSID(""), wifiPassword(""), serverURL(""), deviceToken(""), 
                     deviceName(""), commandPollIntervalMs(5000), 
                     dataUploadIntervalMs(30000), connectionTimeoutMs(10000) {}
};

// Safety configuration
struct SafetyConfig {
    bool enableEmergencyShutdown;
    float maxTemperatureC;
    float minTemperatureC;
    float maxHumidityPercent;
    float maxPressurePSI;
    unsigned long sensorTimeoutMs;
    
    SafetyConfig() : enableEmergencyShutdown(true), maxTemperatureC(50.0), 
                    minTemperatureC(-10.0), maxHumidityPercent(95.0), 
                    maxPressurePSI(100.0), sensorTimeoutMs(30000) {}
};

// Configuration validation result
struct ValidationResult {
    bool isValid;
    std::vector<String> errors;
    std::vector<String> warnings;
    
    ValidationResult() : isValid(true) {}
    
    void addError(const String& error) {
        errors.push_back(error);
        isValid = false;
    }
    
    void addWarning(const String& warning) {
        warnings.push_back(warning);
    }
    
    bool hasIssues() const {
        return !errors.empty() || !warnings.empty();
    }
};

// Forward declaration for device capabilities
class IDeviceCapabilities;

// Core configuration manager
class Config : public BaseManager {
private:
    static const String CONFIG_FILE;
    static const size_t MAX_CONFIG_SIZE = 8192;
    
    JsonDocument configDoc;
    const IDeviceCapabilities* deviceCapabilities;
    bool isLoaded;
    bool hasUnsavedChanges;
    
    // Internal methods
    bool loadFromFile();
    bool saveToFile();
    void createDefaultConfig();
    ValidationResult validateConfig() const;
    
    // Device-specific config creation
    void createDeviceSensors(JsonArray& sensors);
    void createDeviceActuators(JsonArray& actuators);
    void createDeviceSafety(JsonObject& safety);
    
public:
    Config() : BaseManager("Config"), deviceCapabilities(nullptr), 
               isLoaded(false), hasUnsavedChanges(false) {}
    
    // BaseManager implementation
    bool begin() override;
    void shutdown() override;
    
    // Device capabilities injection
    void setDeviceCapabilities(const IDeviceCapabilities* capabilities) {
        deviceCapabilities = capabilities;
    }
    
    // Configuration loading/saving
    bool load();
    bool save();
    bool reload();
    bool hasChanges() const { return hasUnsavedChanges; }
    
    // Configuration access
    NetworkConfig getNetwork() const;
    SafetyConfig getSafety() const;
    std::vector<SensorConfig> getSensors() const;
    std::vector<ActuatorConfig> getActuators() const;
    
    // Configuration updates
    bool updateNetwork(const NetworkConfig& network);
    bool updateSafety(const SafetyConfig& safety);
    bool updateSensor(const String& name, const SensorConfig& sensor);
    bool updateActuator(const String& name, const ActuatorConfig& actuator);
    
    // Sensor/Actuator management
    bool addSensor(const SensorConfig& sensor);
    bool removeSensor(const String& name);
    bool addActuator(const ActuatorConfig& actuator);
    bool removeActuator(const String& name);
    
    // Validation
    ValidationResult validate() const;
    bool isConfigValid() const;
    
    // Utilities
    void printConfig() const;
    size_t getConfigSize() const;
    String getConfigHash() const;
    
    // Factory reset
    bool resetToDefaults();
    
    // JSON access (for advanced use)
    const JsonDocument& getJson() const { return configDoc; }
    bool updateFromJson(const String& jsonString);
    String exportToJson() const;
};

// Global configuration instance
extern Config config;

// Configuration validation helpers
namespace ConfigValidator {
    bool isValidWiFiSSID(const String& ssid);
    bool isValidURL(const String& url);
    bool isValidPin(int pin);
    bool isValidI2CAddress(int address);
    bool isValidSensorName(const String& name);
    bool isValidActuatorName(const String& name);
}

#endif // CORE_CONFIG_H