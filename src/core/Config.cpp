// File: src/core/Config.cpp
#include "Config.h"
#include "BaseClasses.h"
#include <WiFi.h>

const String Config::CONFIG_FILE = "/config.json";

bool Config::begin() {
    setState(ManagerState::INITIALIZING);
    initTime = millis();
    
    Serial.println("Initializing configuration system...");
    
    // Initialize filesystem
    if (!LittleFS.begin()) {
        setError("Failed to initialize LittleFS");
        return false;
    }
    
    // Load configuration
    if (!load()) {
        Serial.println("No valid config found, creating defaults");
        createDefaultConfig();
        if (!save()) {
            setError("Failed to save default configuration");
            return false;
        }
    }
    
    // Validate configuration
    ValidationResult validation = validate();
    if (!validation.isValid) {
        Serial.println("Configuration validation failed:");
        for (const String& error : validation.errors) {
            Serial.println("  ERROR: " + error);
        }
        setError("Configuration validation failed");
        return false;
    }
    
    // Print warnings
    for (const String& warning : validation.warnings) {
        Serial.println("  WARNING: " + warning);
    }
    
    setState(ManagerState::READY);
    eventBus.publish(CoreEventTypes::CONFIG_LOADED, "Config", "{}");
    
    Serial.println("Configuration system ready");
    return true;
}

void Config::shutdown() {
    if (hasUnsavedChanges) {
        Serial.println("Saving configuration before shutdown...");
        save();
    }
    
    LittleFS.end();
    setState(ManagerState::SHUTDOWN);
}

bool Config::load() {
    return loadFromFile();
}

bool Config::loadFromFile() {
    if (!LittleFS.exists(CONFIG_FILE)) {
        Serial.println("Configuration file not found");
        return false;
    }
    
    File file = LittleFS.open(CONFIG_FILE, "r");
    if (!file) {
        Serial.println("Failed to open configuration file for reading");
        return false;
    }
    
    DeserializationError error = deserializeJson(configDoc, file);
    file.close();
    
    if (error) {
        Serial.printf("Failed to parse configuration: %s\n", error.c_str());
        return false;
    }
    
    isLoaded = true;
    hasUnsavedChanges = false;
    Serial.printf("Configuration loaded (%d bytes)\n", configDoc.memoryUsage());
    return true;
}

bool Config::save() {
    return saveToFile();
}

bool Config::saveToFile() {
    File file = LittleFS.open(CONFIG_FILE, "w");
    if (!file) {
        Serial.println("Failed to open configuration file for writing");
        return false;
    }
    
    size_t bytesWritten = serializeJson(configDoc, file);
    file.close();
    
    if (bytesWritten == 0) {
        Serial.println("Failed to write configuration");
        return false;
    }
    
    hasUnsavedChanges = false;
    Serial.printf("Configuration saved (%d bytes)\n", bytesWritten);
    eventBus.publish(CoreEventTypes::CONFIG_SAVED, "Config", "{}");
    return true;
}

bool Config::reload() {
    configDoc.clear();
    isLoaded = false;
    return load();
}

void Config::createDefaultConfig() {
    configDoc.clear();
    
    // Device info (will be overridden by device-specific implementation)
    JsonObject device = configDoc["device"].to<JsonObject>();
    device["type"] = "unknown";
    device["name"] = "Unknown Device";
    device["version"] = "1.0.0";
    
    // Network configuration
    JsonObject network = configDoc["network"].to<JsonObject>();
    network["wifi_ssid"] = "";
    network["wifi_password"] = "";
    network["server_url"] = "http://localhost:3000";
    network["device_token"] = "";
    network["device_name"] = "";
    network["command_poll_interval_ms"] = 5000;
    network["data_upload_interval_ms"] = 30000;
    network["connection_timeout_ms"] = 10000;
    
    // Safety configuration
    JsonObject safety = configDoc["safety"].to<JsonObject>();
    safety["enable_emergency_shutdown"] = true;
    safety["max_temperature_c"] = 50.0;
    safety["min_temperature_c"] = -10.0;
    safety["max_humidity_percent"] = 95.0;
    safety["max_pressure_psi"] = 100.0;
    safety["sensor_timeout_ms"] = 30000;
    
    // Create device-specific configurations
    JsonArray sensors = configDoc["sensors"].to<JsonArray>();
    createDeviceSensors(sensors);
    
    JsonArray actuators = configDoc["actuators"].to<JsonArray>();
    createDeviceActuators(actuators);
    
    createDeviceSafety(safety);
    
    isLoaded = true;
    hasUnsavedChanges = true;
    Serial.println("Default configuration created");
}

void Config::createDeviceSensors(JsonArray& sensors) {
    // Default implementation - override in device-specific config
    if (deviceCapabilities) {
        // Device will provide its own sensor defaults
        // This is just a fallback
        return;
    }
    
    // Generic sensor as fallback
    JsonObject tempSensor = sensors.add<JsonObject>();
    tempSensor["name"] = "temperature";
    tempSensor["type"] = "Generic";
    tempSensor["pin"] = -1;
    tempSensor["i2c_address"] = 0;
    tempSensor["enabled"] = false;
    tempSensor["calibration_offset"] = 0.0;
    tempSensor["calibration_scale"] = 1.0;
    tempSensor["read_interval_ms"] = 1000;
}

void Config::createDeviceActuators(JsonArray& actuators) {
    // Default implementation - override in device-specific config
    if (deviceCapabilities) {
        // Device will provide its own actuator defaults
        return;
    }
    
    // Generic actuator as fallback
    JsonObject relay = actuators.add<JsonObject>();
    relay["name"] = "relay1";
    relay["type"] = "Generic";
    relay["pin"] = -1;
    relay["enabled"] = false;
    relay["invert_logic"] = false;
    relay["pulse_width_ms"] = 0;
}

void Config::createDeviceSafety(JsonObject& safety) {
    // Device-specific safety settings can be added here
    // Base implementation already set in createDefaultConfig
}

NetworkConfig Config::getNetwork() const {
    NetworkConfig network;
    JsonObject net = configDoc["network"];
    
    network.wifiSSID = net["wifi_ssid"].as<String>();
    network.wifiPassword = net["wifi_password"].as<String>();
    network.serverURL = net["server_url"].as<String>();
    network.deviceToken = net["device_token"].as<String>();
    network.deviceName = net["device_name"].as<String>();
    network.commandPollIntervalMs = net["command_poll_interval_ms"] | 5000;
    network.dataUploadIntervalMs = net["data_upload_interval_ms"] | 30000;
    network.connectionTimeoutMs = net["connection_timeout_ms"] | 10000;
    
    return network;
}

SafetyConfig Config::getSafety() const {
    SafetyConfig safety;
    JsonObject saf = configDoc["safety"];
    
    safety.enableEmergencyShutdown = saf["enable_emergency_shutdown"] | true;
    safety.maxTemperatureC = saf["max_temperature_c"] | 50.0;
    safety.minTemperatureC = saf["min_temperature_c"] | -10.0;
    safety.maxHumidityPercent = saf["max_humidity_percent"] | 95.0;
    safety.maxPressurePSI = saf["max_pressure_psi"] | 100.0;
    safety.sensorTimeoutMs = saf["sensor_timeout_ms"] | 30000;
    
    return safety;
}

std::vector<SensorConfig> Config::getSensors() const {
    std::vector<SensorConfig> sensors;
    JsonArray sensorArray = configDoc["sensors"];
    
    for (JsonObject sensor : sensorArray) {
        SensorConfig config;
        config.name = sensor["name"].as<String>();
        config.type = sensor["type"].as<String>();
        config.pin = sensor["pin"] | -1;
        config.i2cAddress = sensor["i2c_address"] | 0;
        config.enabled = sensor["enabled"] | false;
        config.calibrationOffset = sensor["calibration_offset"] | 0.0;
        config.calibrationScale = sensor["calibration_scale"] | 1.0;
        config.readIntervalMs = sensor["read_interval_ms"] | 1000;
        sensors.push_back(config);
    }
    
    return sensors;
}

std::vector<ActuatorConfig> Config::getActuators() const {
    std::vector<ActuatorConfig> actuators;
    JsonArray actuatorArray = configDoc["actuators"];
    
    for (JsonObject actuator : actuatorArray) {
        ActuatorConfig config;
        config.name = actuator["name"].as<String>();
        config.type = actuator["type"].as<String>();
        config.pin = actuator["pin"] | -1;
        config.enabled = actuator["enabled"] | false;
        config.invertLogic = actuator["invert_logic"] | false;
        config.pulseWidthMs = actuator["pulse_width_ms"] | 0;
        actuators.push_back(config);
    }
    
    return actuators;
}

ValidationResult Config::validate() const {
    ValidationResult result;
    
    // Validate network configuration
    NetworkConfig network = getNetwork();
    if (network.serverURL.isEmpty()) {
        result.addWarning("Server URL not configured");
    } else if (!ConfigValidator::isValidURL(network.serverURL)) {
        result.addError("Invalid server URL format");
    }
    
    if (network.wifiSSID.length() > 32) {
        result.addError("WiFi SSID too long (max 32 characters)");
    }
    
    if (network.wifiPassword.length() > 64) {
        result.addError("WiFi password too long (max 64 characters)");
    }
    
    // Validate sensors
    std::vector<SensorConfig> sensors = getSensors();
    for (const auto& sensor : sensors) {
        if (!sensor.enabled) continue;
        
        if (!ConfigValidator::isValidSensorName(sensor.name)) {
            result.addError("Invalid sensor name: " + sensor.name);
        }
        
        if (sensor.pin != -1 && !ConfigValidator::isValidPin(sensor.pin)) {
            result.addError("Invalid pin for sensor " + sensor.name + ": " + String(sensor.pin));
        }
        
        if (sensor.i2cAddress != 0 && !ConfigValidator::isValidI2CAddress(sensor.i2cAddress)) {
            result.addError("Invalid I2C address for sensor " + sensor.name + ": 0x" + String(sensor.i2cAddress, HEX));
        }
        
        // Device-specific validation
        if (deviceCapabilities && !deviceCapabilities->validateSensorConfig(sensor)) {
            result.addError("Device validation failed for sensor: " + sensor.name);
        }
    }
    
    // Validate actuators
    std::vector<ActuatorConfig> actuators = getActuators();
    for (const auto& actuator : actuators) {
        if (!actuator.enabled) continue;
        
        if (!ConfigValidator::isValidActuatorName(actuator.name)) {
            result.addError("Invalid actuator name: " + actuator.name);
        }
        
        if (!ConfigValidator::isValidPin(actuator.pin)) {
            result.addError("Invalid pin for actuator " + actuator.name + ": " + String(actuator.pin));
        }
        
        // Device-specific validation
        if (deviceCapabilities && !deviceCapabilities->validateActuatorConfig(actuator)) {
            result.addError("Device validation failed for actuator: " + actuator.name);
        }
    }
    
    return result;
}

bool Config::isConfigValid() const {
    return validate().isValid;
}

void Config::printConfig() const {
    Serial.println("=== Current Configuration ===");
    serializeJsonPretty(configDoc, Serial);
    Serial.println("\n=============================");
}

size_t Config::getConfigSize() const {
    return measureJson(configDoc);
}

String Config::getConfigHash() const {
    // Simple hash based on config size and first few bytes
    String configStr;
    serializeJson(configDoc, configStr);
    
    uint32_t hash = 0;
    for (char c : configStr) {
        hash = hash * 31 + c;
    }
    
    return String(hash, HEX);
}

bool Config::resetToDefaults() {
    Serial.println("Resetting configuration to defaults...");
    createDefaultConfig();
    return save();
}

// Global instance
Config config;

// Validation helpers implementation
namespace ConfigValidator {
    bool isValidWiFiSSID(const String& ssid) {
        return !ssid.isEmpty() && ssid.length() <= 32;
    }
    
    bool isValidURL(const String& url) {
        return url.startsWith("http://") || url.startsWith("https://");
    }
    
    bool isValidPin(int pin) {
        return pin >= 0 && pin <= 39; // ESP32 pin range
    }
    
    bool isValidI2CAddress(int address) {
        return address >= 0x08 && address <= 0x77; // Valid I2C address range
    }
    
    bool isValidSensorName(const String& name) {
        return !name.isEmpty() && name.length() <= 32 && 
               name.indexOf(' ') == -1; // No spaces
    }
    
    bool isValidActuatorName(const String& name) {
        return !name.isEmpty() && name.length() <= 32 && 
               name.indexOf(' ') == -1; // No spaces
    }
}