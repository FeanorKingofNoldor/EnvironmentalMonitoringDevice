#include "Config.h"

Config config;
const char* Config::CONFIG_FILE_PATH = "/config.json";

Config::Config() : isLoaded(false) {}

bool Config::begin() {
    Serial.println("Initializing configuration system...");
    
    if (!LittleFS.begin(true)) {
        Serial.println("ERROR: Failed to initialize LittleFS");
        return false;
    }
    
    if (!load()) {
        Serial.println("Creating default configuration...");
        createDefaultConfig();
        if (!save()) {
            Serial.println("ERROR: Failed to save default configuration");
            return false;
        }
    }
    
    if (!validateConfig()) {
        Serial.printf("WARNING: Configuration validation failed: %s\n", 
                     getValidationErrors().c_str());
    }
    
    Serial.println("Configuration system initialized");
    return true;
}

bool Config::load() {
    return loadFromFile();
}

bool Config::save() {
    return saveToFile();
}

bool Config::reset() {
    createDefaultConfig();
    return save();
}

bool Config::loadFromFile() {
    File file = LittleFS.open(CONFIG_FILE_PATH, "r");
    if (!file) {
        Serial.println("Configuration file not found");
        return false;
    }
    
    DeserializationError error = deserializeJson(configDoc, file);
    file.close();
    
    if (error) {
        Serial.printf("Failed to parse configuration: %s\n", error.c_str());
        return false;
    }
    
    isLoaded = true;
    Serial.println("Configuration loaded successfully");
    return true;
}

bool Config::saveToFile() {
    File file = LittleFS.open(CONFIG_FILE_PATH, "w");
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
    
    Serial.printf("Configuration saved (%d bytes)\n", bytesWritten);
    return true;
}

void Config::createDefaultConfig() {
    configDoc.clear();
    
    // Network configuration
    JsonObject network = configDoc["network"].to<JsonObject>();
    network["wifi_ssid"] = "";
    network["wifi_password"] = "";
    network["server_url"] = "http://localhost:3000";
    network["device_token"] = "";
    network["command_poll_interval_ms"] = 5000;
    network["data_upload_interval_ms"] = 30000;
    
    // Sensor configuration
    JsonArray sensors = configDoc["sensors"].to<JsonArray>();
    
    JsonObject tempSensor = sensors.add<JsonObject>();
    tempSensor["name"] = "sht3x";
    tempSensor["type"] = "SHT3x";
    tempSensor["pin"] = -1;
    tempSensor["i2c_address"] = 0x44;
    tempSensor["enabled"] = true;
    tempSensor["calibration_offset"] = 0.0;
    tempSensor["calibration_scale"] = 1.0;
    
    JsonObject pressureSensor = sensors.add<JsonObject>();
    pressureSensor["name"] = "pressure";
    pressureSensor["type"] = "Analog";
    pressureSensor["pin"] = 36;
    pressureSensor["enabled"] = true;
    pressureSensor["calibration_offset"] = 0.0;
    pressureSensor["calibration_scale"] = 1.0;
    
    // Actuator configuration
    JsonArray actuators = configDoc["actuators"].to<JsonArray>();
    
    JsonObject lightsRelay = actuators.add<JsonObject>();
    lightsRelay["name"] = "lights";
    lightsRelay["type"] = "Relay";
    lightsRelay["pin"] = 23;
    lightsRelay["enabled"] = true;
    lightsRelay["invert_logic"] = false;
    
    JsonObject sprayRelay = actuators.add<JsonObject>();
    sprayRelay["name"] = "spray";
    sprayRelay["type"] = "Relay";
    sprayRelay["pin"] = 22;
    sprayRelay["enabled"] = true;
    sprayRelay["invert_logic"] = false;
    sprayRelay["pulse_width_ms"] = 5000;
    
    // Safety configuration
    JsonObject safety = configDoc["safety"].to<JsonObject>();
    safety["max_temperature"] = 50.0;
    safety["min_temperature"] = -10.0;
    safety["max_humidity"] = 95.0;
    safety["max_pressure"] = 100.0;
    safety["enable_emergency_shutdown"] = true;
    
    isLoaded = true;
    Serial.println("Default configuration created");
}

std::vector<SensorConfig> Config::getSensors() {
    std::vector<SensorConfig> sensors;
    JsonArray sensorArray = configDoc["sensors"];
    
    for (JsonObject sensor : sensorArray) {
        SensorConfig config;
        config.name = sensor["name"].as<String>();
        config.type = sensor["type"].as<String>();
        config.pin = sensor["pin"];
        config.i2cAddress = sensor["i2c_address"];
        config.enabled = sensor["enabled"];
        config.calibrationOffset = sensor["calibration_offset"];
        config.calibrationScale = sensor["calibration_scale"];
        sensors.push_back(config);
    }
    
    return sensors;
}

std::vector<ActuatorConfig> Config::getActuators() {
    std::vector<ActuatorConfig> actuators;
    JsonArray actuatorArray = configDoc["actuators"];
    
    for (JsonObject actuator : actuatorArray) {
        ActuatorConfig config;
        config.name = actuator["name"].as<String>();
        config.type = actuator["type"].as<String>();
        config.pin = actuator["pin"];
        config.enabled = actuator["enabled"];
        config.invertLogic = actuator["invert_logic"];
        config.pulseWidthMs = actuator["pulse_width_ms"];
        actuators.push_back(config);
    }
    
    return actuators;
}

NetworkConfig Config::getNetwork() {
    NetworkConfig network;
    JsonObject net = configDoc["network"];
    
    network.wifiSSID = net["wifi_ssid"].as<String>();
    network.wifiPassword = net["wifi_password"].as<String>();
    network.serverURL = net["server_url"].as<String>();
    network.deviceToken = net["device_token"].as<String>();
    network.commandPollIntervalMs = net["command_poll_interval_ms"];
    network.dataUploadIntervalMs = net["data_upload_interval_ms"];
    
    return network;
}

SafetyConfig Config::getSafety() {
    SafetyConfig safety;
    JsonObject saf = configDoc["safety"];
    
    safety.maxTemperature = saf["max_temperature"];
    safety.minTemperature = saf["min_temperature"];
    safety.maxHumidity = saf["max_humidity"];
    safety.maxPressure = saf["max_pressure"];
    safety.enableEmergencyShutdown = saf["enable_emergency_shutdown"];
    
    return safety;
}

bool Config::validateConfig() {
    // Basic validation - could be expanded
    return configDoc.containsKey("network") && 
           configDoc.containsKey("sensors") && 
           configDoc.containsKey("actuators") &&
           configDoc.containsKey("safety");
}

String Config::getValidationErrors() {
    // Placeholder for detailed validation
    return "Basic structure validation";
}

void Config::printConfig() {
    Serial.println("Current Configuration:");
    serializeJsonPretty(configDoc, Serial);
    Serial.println();
}

size_t Config::getConfigSize() {
    return measureJson(configDoc);
}