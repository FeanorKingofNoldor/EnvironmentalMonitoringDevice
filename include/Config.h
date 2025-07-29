#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <vector>

// Configuration structures
struct SensorConfig {
    String name;
    String type;
    int pin;
    int i2cAddress;
    bool enabled;
    float calibrationOffset;
    float calibrationScale;
    
    SensorConfig() : pin(-1), i2cAddress(0), enabled(false), 
                     calibrationOffset(0.0), calibrationScale(1.0) {}
};

struct ActuatorConfig {
    String name;
    String type;
    int pin;
    bool enabled;
    bool invertLogic;
    int pulseWidthMs;
    
    ActuatorConfig() : pin(-1), enabled(false), invertLogic(false), pulseWidthMs(0) {}
};

struct NetworkConfig {
    String wifiSSID;
    String wifiPassword;
    String serverURL;
    String deviceToken;
    int commandPollIntervalMs;
    int dataUploadIntervalMs;
    
    NetworkConfig() : commandPollIntervalMs(5000), dataUploadIntervalMs(30000) {}
};

struct SafetyConfig {
    float maxTemperature;
    float minTemperature;
    float maxHumidity;
    float maxPressure;
    bool enableEmergencyShutdown;
    
    SafetyConfig() : maxTemperature(50.0), minTemperature(-10.0), 
                     maxHumidity(95.0), maxPressure(100.0), 
                     enableEmergencyShutdown(true) {}
};

// Configuration manager class
class Config {
private:
    JsonDocument configDoc;
    bool isLoaded;
    static const char* CONFIG_FILE_PATH;
    
    // File operations
    bool loadFromFile();
    bool saveToFile();
    void createDefaultConfig();
    
    // Validation
    bool validateConfig();
    String getValidationErrors();
    
public:
    Config();
    
    // Lifecycle
    bool begin();
    bool load();
    bool save();
    bool reset();
    
    // Configuration sections
    std::vector<SensorConfig> getSensors();
    void setSensorConfig(const String& name, const SensorConfig& config);
    
    std::vector<ActuatorConfig> getActuators();
    void setActuatorConfig(const String& name, const ActuatorConfig& config);
    
    NetworkConfig getNetwork();
    void setNetwork(const NetworkConfig& config);
    
    SafetyConfig getSafety();
    void setSafety(const SafetyConfig& config);
    
    // Generic getters/setters
    int getInt(const String& path, int defaultValue = 0);
    float getFloat(const String& path, float defaultValue = 0.0);
    String getString(const String& path, const String& defaultValue = "");
    bool getBool(const String& path, bool defaultValue = false);
    
    void set(const String& path, int value);
    void set(const String& path, float value);
    void set(const String& path, const String& value);
    void set(const String& path, bool value);
    
    // Utilities
    void printConfig();
    size_t getConfigSize();
    bool isConfigLoaded() const { return isLoaded; }
};

// Global configuration instance
extern Config config;

#endif // CONFIG_H