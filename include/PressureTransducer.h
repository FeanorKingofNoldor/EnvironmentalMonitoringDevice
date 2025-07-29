#ifndef PRESSURE_TRANSDUCER_H
#define PRESSURE_TRANSDUCER_H

#include "SensorManager.h"
#include "driver/adc.h"

class PressureTransducer : public BaseSensor {
private:
    SensorConfig config;
    adc1_channel_t adcChannel;
    bool initialized;
    float lastPressure;
    unsigned long lastReadTime;
    
    static const unsigned long READ_INTERVAL_MS = 1000; // 1 second
    
    // ADC and conversion methods
    bool initializeADC();
    float readRawPressure();
    float convertToPSI(float voltage);
    
public:
    explicit PressureTransducer(const SensorConfig& cfg);
    ~PressureTransducer() override = default;
    
    bool begin() override;
    SensorReading read() override;
    String getName() const override { return config.name; }
    bool isReady() const override { return initialized; }
    
    // Pressure specific methods
    float getPressure();
    float applyCalibration(float rawValue) const;
};

#endif // PRESSURE_TRANSDUCER_H