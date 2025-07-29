#include "SHT3xSensor.h"
#include "Logger.h"

SHT30Sensor::SHT30Sensor(const SensorConfig& cfg) 
    : config(cfg), initialized(false), lastReadTime(0), 
      lastTemperature(0.0), lastHumidity(0.0) {
    // Constructor - initialization happens in begin()
}

bool SHT30Sensor::begin() {
    Logger::info("SHT30", "Initializing SHT30 sensor: " + config.name);
    
    // Initialize the sensor (DFRobot library uses default I2C address)
    int result = sht3x.begin();
    if (result != 0) {
        Logger::error("SHT30", "Failed to initialize SHT30, error code: " + String(result));
        return false;
    }
    
    // Try to read serial number to verify communication
    uint32_t serialNumber = sht3x.readSerialNumber();
    if (serialNumber == 0) {
        Logger::error("SHT30", "Failed to read serial number");
        return false;
    }
    
    Logger::info("SHT30", "SHT30 initialized successfully, Serial: " + String(serialNumber, HEX));
    
    // Perform a soft reset
    if (!sht3x.softReset()) {
        Logger::warn("SHT30", "Soft reset failed, continuing anyway");
    }
    
    // Clear any existing alerts
    sht3x.clearStatusRegister();
    
    initialized = true;
    return true;
}

SensorReading SHT30Sensor::read() {
    SensorReading reading;
    reading.sensorName = config.name;
    reading.valid = false;
    
    if (!initialized) {
        Logger::error("SHT30", "Sensor not initialized");
        return reading;
    }
    
    // Check if enough time has passed since last reading
    unsigned long currentTime = millis();
    if (currentTime - lastReadTime < READ_INTERVAL_MS) {
        // Return cached reading
        reading.type = "temperature";
        reading.value = lastTemperature;
        reading.unit = "°C";
        reading.timestamp = lastReadTime;
        reading.valid = true;
        return reading;
    }
    
    // Read temperature and humidity from sensor
    DFRobot_SHT3x::sRHAndTemp_t data = sht3x.readTemperatureAndHumidity(DFRobot_SHT3x::eRepeatability_High);
    
    if (data.ERR == 0) {
        // Apply calibration
        lastTemperature = applyCalibration(data.TemperatureC, true);
        lastHumidity = applyCalibration(data.Humidity, false);
        lastReadTime = currentTime;
        
        Logger::debug("SHT30", "T: " + String(lastTemperature) + "°C, H: " + String(lastHumidity) + "%");
        
        // Return temperature reading (humidity will be read separately)
        reading.type = "temperature";
        reading.value = lastTemperature;
        reading.unit = "°C";
        reading.timestamp = currentTime;
        reading.valid = true;
        
    } else {
        Logger::error("SHT30", "Failed to read sensor data, error: " + String(data.ERR));
    }
    
    return reading;
}

bool SHT30Sensor::readTemperature(float& temperature) {
    if (!initialized) return false;
    
    // Check if we have recent data
    unsigned long currentTime = millis();
    if (currentTime - lastReadTime < READ_INTERVAL_MS) {
        temperature = lastTemperature;
        return true;
    }
    
    // Read fresh data
    DFRobot_SHT3x::sRHAndTemp_t data = sht3x.readTemperatureAndHumidity(DFRobot_SHT3x::eRepeatability_High);
    
    if (data.ERR == 0) {
        lastTemperature = applyCalibration(data.TemperatureC, true);
        lastHumidity = applyCalibration(data.Humidity, false);
        lastReadTime = currentTime;
        temperature = lastTemperature;
        return true;
    }
    
    return false;
}

bool SHT30Sensor::readHumidity(float& humidity) {
    if (!initialized) return false;
    
    // Check if we have recent data
    unsigned long currentTime = millis();
    if (currentTime - lastReadTime < READ_INTERVAL_MS) {
        humidity = lastHumidity;
        return true;
    }
    
    // Read fresh data
    DFRobot_SHT3x::sRHAndTemp_t data = sht3x.readTemperatureAndHumidity(DFRobot_SHT3x::eRepeatability_High);
    
    if (data.ERR == 0) {
        lastTemperature = applyCalibration(data.TemperatureC, true);
        lastHumidity = applyCalibration(data.Humidity, false);
        lastReadTime = currentTime;
        humidity = lastHumidity;
        return true;
    }
    
    return false;
}

bool SHT30Sensor::readBoth(float& temperature, float& humidity) {
    if (!initialized) return false;
    
    DFRobot_SHT3x::sRHAndTemp_t data = sht3x.readTemperatureAndHumidity(DFRobot_SHT3x::eRepeatability_High);
    
    if (data.ERR == 0) {
        temperature = applyCalibration(data.TemperatureC, true);
        humidity = applyCalibration(data.Humidity, false);
        
        lastTemperature = temperature;
        lastHumidity = humidity;
        lastReadTime = millis();
        
        return true;
    }
    
    return false;
}

float SHT30Sensor::applyCalibration(float rawValue, bool isTemperature) const {
    float calibratedValue = rawValue;
    
    // Apply offset and scale calibration
    calibratedValue = (calibratedValue + config.calibrationOffset) * config.calibrationScale;
    
    // Sanity checks
    if (isTemperature) {
        // Temperature should be within reasonable range
        if (calibratedValue < -40.0 || calibratedValue > 125.0) {
            Logger::warn("SHT30", "Temperature out of range: " + String(calibratedValue));
        }
    } else {
        // Humidity should be 0-100%
        calibratedValue = constrain(calibratedValue, 0.0, 100.0);
    }
    
    return calibratedValue;
}

bool SHT30Sensor::enableHeater() {
    if (!initialized) return false;
    
    bool result = sht3x.heaterEnable();
    if (result) {
        Logger::info("SHT30", "Heater enabled");
    } else {
        Logger::error("SHT30", "Failed to enable heater");
    }
    return result;
}

bool SHT30Sensor::disableHeater() {
    if (!initialized) return false;
    
    bool result = sht3x.heaterDisable();
    if (result) {
        Logger::info("SHT30", "Heater disabled");
    } else {
        Logger::error("SHT30", "Failed to disable heater");
    }
    return result;
}

uint32_t SHT30Sensor::getSerialNumber() {
    if (!initialized) return 0;
    return sht3x.readSerialNumber();
}