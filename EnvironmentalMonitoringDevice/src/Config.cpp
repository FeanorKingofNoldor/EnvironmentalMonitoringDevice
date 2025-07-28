#include "Config.h"
#include "LittleFS.h"

Config config;

Config::Config() : loaded(false) {}

bool Config::load() {
    if (!LittleFS.begin(true)) {
        Serial.println("Failed to mount LittleFS");
        return false;
    }
    
    File file = LittleFS.open("/config.json", "r");
    if (!file) {
        Serial.println("Config file not found, using defaults");
        return false;
    }
    
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        Serial.printf("Failed to parse config: %s\n", error.c_str());
        return false;
    }
    
    loaded = true;
    return true;
}

bool Config::save() {
    File file = LittleFS.open("/config.json", "w");
    if (!file) {
        Serial.println("Failed to open config file for writing");
        return false;
    }
    
    bool success = serializeJson(doc, file) > 0;
    file.close();
    return success;
}

std::vector<SensorConfig> Config::getSensors() {
    std::vector<SensorConfig> sensors;
    JsonArray sensorArray = doc["sensors"];
    
    for (JsonObject sensor : sensorArray) {
        SensorConfig config;
        config.name = sensor["name"].as<String>();
        config.type = sensor["type"].as<String>();
        config.pin = sensor["pin"];
        config.address = sensor["address"];
        config.enabled = sensor["enabled"];
        sensors.push_back(config);
    }
    
    return sensors;
}

std::vector<ActuatorConfig> Config::getActuators() {
    std::vector<ActuatorConfig> actuators;
    JsonArray actuatorArray = doc["actuators"];
    
    for (JsonObject actuator : actuatorArray) {
        ActuatorConfig config;
        config.name = actuator["name"].as<String>();
        config.type = actuator["type"].as<String>();
        config.pin = actuator["pin"];
        config.enabled = actuator["enabled"];
        config.params = actuator["params"];
        actuators.push_back(config);
    }
    
    return actuators;
}

NetworkConfig Config::getNetwork() {
    NetworkConfig network;
    JsonObject net = doc["network"];
    
    network.wifi_ssid = net["wifi_ssid"].as<String>();
    network.wifi_password = net["wifi_password"].as<String>();
    network.server_url = net["server_url"].as<String>();
    network.device_token = net["device_token"].as<String>();
    
    return network;
}

int Config::getInt(const String& path, int defaultValue) {
    JsonVariant value = doc[path];
    return value.isNull() ? defaultValue : value.as<int>();
}

String Config::getString(const String& path, const String& defaultValue) {
    JsonVariant value = doc[path];
    return value.isNull() ? defaultValue : value.as<String>();
}

bool Config::getBool(const String& path, bool defaultValue) {
    JsonVariant value = doc[path];
    return value.isNull() ? defaultValue : value.as<bool>();
}

void Config::set(const String& path, const String& value) {
    doc[path] = value;
}

void Config::set(const String& path, int value) {
    doc[path] = value;
}

void Config::set(const String& path, bool value) {
    doc[path] = value;
}