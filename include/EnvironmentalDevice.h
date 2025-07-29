// File: src/device/EnvironmentalDevice.h
#ifndef ENVIRONMENTAL_DEVICE_H
#define ENVIRONMENTAL_DEVICE_H

#include "core/BaseClasses.h"
#include "core/Config.h"

// Environmental-specific event types
namespace EnvEventTypes {
    // Environmental sensor events
    constexpr const char* SENSOR_TEMPERATURE = "env.sensor.temperature";
    constexpr const char* SENSOR_HUMIDITY = "env.sensor.humidity";
    constexpr const char* SENSOR_PRESSURE = "env.sensor.pressure";
    constexpr const char* SENSOR_LIGHT = "env.sensor.light";
    
    // Environmental actuator events
    constexpr const char* ACTUATOR_LIGHTS = "env.actuator.lights";
    constexpr const char* ACTUATOR_SPRAY = "env.actuator.spray";
    constexpr const char* ACTUATOR_FAN = "env.actuator.fan";
    constexpr const char* ACTUATOR_HEATER = "env.actuator.heater";
    
    // Environmental control events
    constexpr const char* CLIMATE_CONTROL_ACTIVE = "env.climate.active";
    constexpr const char* GROWTH_CYCLE_STARTED = "env.growth.started";
    constexpr const char* GROWTH_CYCLE_STOPPED = "env.growth.stopped";
    
    // Environmental safety events
    constexpr const char* TEMPERATURE_ALARM = "env.safety.temperature";
    constexpr const char* HUMIDITY_ALARM = "env.safety.humidity";
    constexpr const char* PRESSURE_ALARM = "env.safety.pressure";
}

// Environmental device capabilities
class EnvironmentalDevice : public IDeviceCapabilities {
public:
    // IDeviceCapabilities implementation
    String getDeviceType() const override { return "environmental"; }
    String getDeviceName() const override { return "AeroEnv Environmental Controller"; }
    String getFirmwareVersion() const override { return "1.0.0"; }
    
    std::vector<String> getSupportedSensorTypes() const override {
        return {"SHT3x", "BME280", "AnalogPressure", "LightSensor", "DS18B20"};
    }
    
    std::vector<String> getSupportedActuatorTypes() const override {
        return {"Relay", "PWMOutput", "VenturiNozzle"};
    }
    
    std::vector<String> getDeviceEventTypes() const override {
        return {
            EnvEventTypes::SENSOR_TEMPERATURE,
            EnvEventTypes::SENSOR_HUMIDITY,
            EnvEventTypes::SENSOR_PRESSURE,
            EnvEventTypes::SENSOR_LIGHT,
            EnvEventTypes::ACTUATOR_LIGHTS,
            EnvEventTypes::ACTUATOR_SPRAY,
            EnvEventTypes::ACTUATOR_FAN,
            EnvEventTypes::ACTUATOR_HEATER,
            EnvEventTypes::CLIMATE_CONTROL_ACTIVE,
            EnvEventTypes::GROWTH_CYCLE_STARTED,
            EnvEventTypes::GROWTH_CYCLE_STOPPED,
            EnvEventTypes::TEMPERATURE_ALARM,
            EnvEventTypes::HUMIDITY_ALARM,
            EnvEventTypes::PRESSURE_ALARM
        };
    }
    
    std::unique_ptr<BaseSensor> createSensor(const SensorConfig& config) const override;
    std::unique_ptr<BaseActuator> createActuator(const ActuatorConfig& config) const override;
    
    bool validateSensorConfig(const SensorConfig& config) const override;
    bool validateActuatorConfig(const ActuatorConfig& config) const override;
    
    // Environmental-specific methods
    void createDefaultSensors(JsonArray& sensors) const;
    void createDefaultActuators(JsonArray& actuators) const;
    void createDefaultSafety(JsonObject& safety) const;
};

// Environmental sensor implementations
#include <Wire.h>

// SHT3x Temperature/Humidity Sensor
class SHT3xSensor : public BaseSensor {
private:
    static const uint8_t SHT3X_CMD_MEASURE_HIGH_REP = 0x2C06;
    static const uint8_t SHT3X_CMD_SOFT_RESET = 0x30A2;
    
    uint8_t i2cAddress;
    float lastTemperature;
    float lastHumidity;
    
    bool sendCommand(uint16_t command);
    bool readData(uint8_t* buffer, size_t length);
    uint8_t calculateCRC8(const uint8_t* data, size_t length);
    
public:
    SHT3xSensor(const SensorConfig& config) : BaseSensor(config), 
                                              i2cAddress(config.i2cAddress),
                                              lastTemperature(0), lastHumidity(0) {}
    
    bool begin() override;
    SensorReading read() override;
    void shutdown() override;
    
    // SHT3x specific methods
    float getTemperature() const { return lastTemperature; }
    float getHumidity() const { return lastHumidity; }
    bool performSoftReset();
};

// Analog Pressure Transducer
class AnalogPressureSensor : public BaseSensor {
private:
    int analogPin;
    float minPressure;
    float maxPressure;
    float minVoltage;
    float maxVoltage;
    
    float voltageToMPa(float voltage);
    float MPaToPSI(float mpa);
    
public:
    AnalogPressureSensor(const SensorConfig& config) : BaseSensor(config),
                                                       analogPin(config.pin),
                                                       minPressure(0.0), maxPressure(1.0),
                                                       minVoltage(0.5), maxVoltage(4.5) {}
    
    bool begin() override;
    SensorReading read() override;
    void shutdown() override;
    
    // Configuration methods
    void setPressureRange(float minMPa, float maxMPa) {
        minPressure = minMPa;
        maxPressure = maxMPa;
    }
    
    void setVoltageRange(float minV, float maxV) {
        minVoltage = minV;
        maxVoltage = maxV;
    }
};

// Environmental actuator implementations

// Relay Actuator
class RelayActuator : public BaseActuator {
private:
    int relayPin;
    bool invertLogic;
    unsigned long activationStartTime;
    
public:
    RelayActuator(const ActuatorConfig& config) : BaseActuator(config),
                                                  relayPin(config.pin),
                                                  invertLogic(config.invertLogic),
                                                  activationStartTime(0) {}
    
    bool begin() override;
    bool activate() override;
    bool deactivate() override;
    void shutdown() override;
    
    // Relay-specific methods
    bool pulse(unsigned long durationMs);
    bool isInverted() const { return invertLogic; }
};

// PWM Output Actuator (for fans, heaters with variable control)
class PWMActuator : public BaseActuator {
private:
    int pwmPin;
    int pwmChannel;
    int pwmFrequency;
    int pwmResolution;
    float currentDutyCycle;
    
public:
    PWMActuator(const ActuatorConfig& config) : BaseActuator(config),
                                                pwmPin(config.pin),
                                                pwmChannel(0), pwmFrequency(5000),
                                                pwmResolution(8), currentDutyCycle(0.0) {}
    
    bool begin() override;
    bool activate() override;
    bool deactivate() override;
    void shutdown() override;
    
    // PWM-specific methods
    bool setDutyCycle(float dutyCycle); // 0.0 to 100.0
    float getDutyCycle() const { return currentDutyCycle; }
    bool setFrequency(int frequency);
};

// Venturi Nozzle Actuator (spray system)
class VenturiNozzleActuator : public BaseActuator {
private:
    int nozzlePin;
    unsigned long sprayDurationMs;
    unsigned long sprayStartTime;
    bool sprayActive;
    
public:
    VenturiNozzleActuator(const ActuatorConfig& config) : BaseActuator(config),
                                                          nozzlePin(config.pin),
                                                          sprayDurationMs(config.pulseWidthMs),
                                                          sprayStartTime(0), sprayActive(false) {}
    
    bool begin() override;
    bool activate() override;
    bool deactivate() override;
    void shutdown() override;
    
    // Spray-specific methods
    bool startSpray(unsigned long durationMs = 0);
    bool stopSpray();
    bool isSprayActive() const { return sprayActive; }
    void update(); // Call periodically to handle timed sprays
};

// Global environmental device instance
extern EnvironmentalDevice environmentalDevice;

#endif // ENVIRONMENTAL_DEVICE_H