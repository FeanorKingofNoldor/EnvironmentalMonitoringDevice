#include "PressureTransducer.h"
#include "Logger.h"

PressureTransducer::PressureTransducer(const SensorConfig& cfg) 
    : config(cfg), initialized(false), lastPressure(0.0), lastReadTime(0) {
    
    // Map pin to ADC channel
    if (config.pin == 36) {
        adcChannel = ADC1_CHANNEL_0;
    } else if (config.pin == 39) {
        adcChannel = ADC1_CHANNEL_3;
    } else if (config.pin == 34) {
        adcChannel = ADC1_CHANNEL_6;
    } else if (config.pin == 35) {
        adcChannel = ADC1_CHANNEL_7;
    } else {
        Logger::error("PressureTransducer", "Unsupported pin: " + String(config.pin));
        adcChannel = ADC1_CHANNEL_0; // Default fallback
    }
}

bool PressureTransducer::begin() {
    Logger::info("PressureTransducer", "Initializing pressure transducer: " + config.name);
    
    if (!initializeADC()) {
        Logger::error("PressureTransducer", "Failed to initialize ADC");
        return false;
    }
    
    // Test reading
    float testPressure = readRawPressure();
    if (testPressure >= 0 && testPressure < 200) { // Reasonable pressure range
        initialized = true;
        Logger::info("PressureTransducer", "Pressure transducer initialized successfully");
        return true;
    }
    
    Logger::error("PressureTransducer", "Pressure transducer test reading failed: " + String(testPressure));
    return false;
}

SensorReading PressureTransducer::read() {
    SensorReading reading;
    reading.sensorName = config.name;
    reading.type = "pressure";
    reading.unit = "PSI";
    reading.valid = false;
    
    if (!initialized) {
        Logger::error("PressureTransducer", "Sensor not initialized");
        return reading;
    }
    
    // Check if enough time has passed since last reading
    unsigned long currentTime = millis();
    if (currentTime - lastReadTime < READ_INTERVAL_MS) {
        // Return cached reading
        reading.value = lastPressure;
        reading.timestamp = lastReadTime;
        reading.valid = true;
        return reading;
    }
    
    // Read fresh pressure data
    float rawPressure = readRawPressure();
    
    if (rawPressure >= 0 && rawPressure < 200) { // Sanity check
        lastPressure = applyCalibration(rawPressure);
        lastReadTime = currentTime;
        
        reading.value = lastPressure;
        reading.timestamp = currentTime;
        reading.valid = true;
        
        Logger::debug("PressureTransducer", "Pressure: " + String(lastPressure) + " PSI");
    } else {
        Logger::error("PressureTransducer", "Invalid pressure reading: " + String(rawPressure));
    }
    
    return reading;
}

bool PressureTransducer::initializeADC() {
    // Configure ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(adcChannel, ADC_ATTEN_DB_12); // Updated from deprecated ADC_ATTEN_DB_11
    
    Logger::debug("PressureTransducer", "ADC configured for channel " + String(adcChannel));
    return true;
}

float PressureTransducer::readRawPressure() {
    int sensorValue = adc1_get_raw(adcChannel);
    float voltage = sensorValue * (3.3 / 4095.0);
    
    return convertToPSI(voltage);
}

float PressureTransducer::convertToPSI(float voltage) {
    // Honeywell pressure transducer conversion
    // 0.25V = 0 PSI, 2.25V = 100 PSI (typical range)
    // Adjust these values based on your specific transducer specs
    float minVoltage = 0.25;
    float maxVoltage = 2.25;
    float minPressure = 0.0;
    float maxPressure = 100.0;
    
    if (voltage < minVoltage) {
        return minPressure;
    }
    if (voltage > maxVoltage) {
        return maxPressure;
    }
    
    float pressure_psi = minPressure + (voltage - minVoltage) * 
                        (maxPressure - minPressure) / (maxVoltage - minVoltage);
    
    return pressure_psi;
}

float PressureTransducer::getPressure() {
    return lastPressure;
}

float PressureTransducer::applyCalibration(float rawValue) const {
    // Apply offset and scale calibration
    float calibratedValue = (rawValue + config.calibrationOffset) * config.calibrationScale;
    
    // Sanity check for pressure (typically 0-200 PSI range)
    if (calibratedValue < -10.0 || calibratedValue > 300.0) {
        Logger::warn("PressureTransducer", "Pressure out of expected range: " + String(calibratedValue));
    }
    
    return calibratedValue;
}