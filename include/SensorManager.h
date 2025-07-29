#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <vector>
#include <memory>
#include "EventBus.h"
#include "Config.h"

// Forward declarations
class BaseSensor;

// Sensor reading structure
struct SensorReading {
    String sensorName;
    String type;
    float value;
    String unit;
    unsigned long timestamp;
    bool valid;
    
    SensorReading() : value(0.0), timestamp(0), valid(false) {}
    SensorReading(const String& name, const String& t, float v, const String& u, bool isValid = true) 
        : sensorName(name), type(t), value(v), unit(u), timestamp(millis()), valid(isValid) {}
};

// Base sensor interface
class BaseSensor {
public:
    virtual ~BaseSensor() = default;
    virtual bool begin() = 0;
    virtual SensorReading read() = 0;
    virtual String getName() const = 0;
    virtual bool isReady() const = 0;
};

// Sensor manager for orchestrating all sensors
class SensorManager {
private:
    std::vector<std::unique_ptr<BaseSensor>> sensors;
    std::vector<SensorReading> lastReadings;
    unsigned long lastReadTime;
    bool isInitialized;
    
    // Sensor creation
    std::unique_ptr<BaseSensor> createSensor(const SensorConfig& config);
    
    // Event handling
    void subscribeToEvents();
    void publishSensorEvent(const SensorReading& reading);
    
public:
    SensorManager();
    ~SensorManager() = default;
    
    // Lifecycle
    bool begin();
    void shutdown();
    
    // Sensor operations
    bool readAllSensors();
    SensorReading getReading(const String& sensorName) const;
    std::vector<SensorReading> getAllReadings() const;
    
    // Status
    size_t getSensorCount() const { return sensors.size(); }
    bool isAllSensorsReady() const;
    void printSensorStatus() const;
    
    // Configuration
    bool addSensor(const SensorConfig& config);
    bool removeSensor(const String& name);
    bool reconfigure();
};

#endif // SENSOR_MANAGER_H