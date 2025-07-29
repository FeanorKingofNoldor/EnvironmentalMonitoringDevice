#ifndef SHT30_SENSOR_H
#define SHT30_SENSOR_H

#include "SensorManager.h"
#include "DFRobot_SHT3x.h"

class SHT30Sensor : public BaseSensor {
private:
    DFRobot_SHT3x sht3x;
    SensorConfig config;
    bool initialized;
    unsigned long lastReadTime;
    float lastTemperature;
    float lastHumidity;
    
    static const unsigned long READ_INTERVAL_MS = 2000; // Minimum time between reads
    
public:
    explicit SHT30Sensor(const SensorConfig& cfg);
    ~SHT30Sensor() override = default;
    
    bool begin() override;
    SensorReading read() override;
    String getName() const override { return config.name; }
    bool isReady() const override { return initialized; }
    
    // SHT30 specific methods using DFRobot API
    bool readTemperature(float& temperature);
    bool readHumidity(float& humidity);
    bool readBoth(float& temperature, float& humidity);
    
    // Calibration
    float applyCalibration(float rawValue, bool isTemperature) const;
    
    // DFRobot specific features
    bool enableHeater();
    bool disableHeater();
    uint32_t getSerialNumber();
};

#endif // SHT30_SENSOR_H