#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

struct SensorConfig {
    String name;
    String type;
    int pin;
    int address; // I2C address if applicable
    bool enabled;
};

struct ActuatorConfig {
    String name;
    String type;
    int pin;
    bool enabled;
    JsonObject params;
};

struct NetworkConfig {
    String wifi_ssid;
    String wifi_password;
    String server_url;
    String device_token;
};

class Config {
private:
    JsonDocument doc;
    bool loaded;
    
public:
    Config();
    
    bool load();
    bool save();
    
    std::vector<SensorConfig> getSensors();
    std::vector<ActuatorConfig> getActuators();
    NetworkConfig getNetwork();
    
    int getInt(const String& path, int defaultValue = 0);
    String getString(const String& path, const String& defaultValue = "");
    bool getBool(const String& path, bool defaultValue = false);
    
    void set(const String& path, const String& value);
    void set(const String& path, int value);
    void set(const String& path, bool value);
};

extern Config config;

#endif