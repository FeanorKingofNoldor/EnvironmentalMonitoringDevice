// File: src/device/EnvironmentalDevice.cpp
#include "EnvironmentalDevice.h"
#include "core/EventBus.h"

// EnvironmentalDevice implementation
std::unique_ptr<BaseSensor> EnvironmentalDevice::createSensor(const SensorConfig& config) const {
    if (config.type == "SHT3x" || config.type == "SHT30") {
        return std::make_unique<SHT3xSensor>(config);
    } else if (config.type == "AnalogPressure") {
        return std::make_unique<AnalogPressureSensor>(config);
    }
    
    Serial.printf("ERROR: Unknown sensor type: %s\n", config.type.c_str());
    return nullptr;
}

std::unique_ptr<BaseActuator> EnvironmentalDevice::createActuator(const ActuatorConfig& config) const {
    if (config.type == "Relay") {
        return std::make_unique<RelayActuator>(config);
    } else if (config.type == "PWMOutput") {
        return std::make_unique<PWMActuator>(config);
    } else if (config.type == "VenturiNozzle") {
        return std::make_unique<VenturiNozzleActuator>(config);
    }
    
    Serial.printf("ERROR: Unknown actuator type: %s\n", config.type.c_str());
    return nullptr;
}

bool EnvironmentalDevice::validateSensorConfig(const SensorConfig& config) const {
    // Environmental-specific sensor validation
    if (config.type == "SHT3x" || config.type == "SHT30") {
        return config.i2cAddress >= 0x44 && config.i2cAddress <= 0x45; // Valid SHT3x addresses
    } else if (config.type == "AnalogPressure") {
        return config.pin >= 32 && config.pin <= 39; // ADC1 pins only
    }
    
    return true; // Default validation passes
}

bool EnvironmentalDevice::validateActuatorConfig(const ActuatorConfig& config) const {
    // Environmental-specific actuator validation
    if (config.type == "Relay") {
        return config.pin >= 0 && config.pin <= 33; // Valid GPIO pins
    } else if (config.type == "PWMOutput") {
        return config.pin >= 0 && config.pin <= 33; // Valid GPIO pins
    } else if (config.type == "VenturiNozzle") {
        return config.pin >= 0 && config.pin <= 33 && config.pulseWidthMs > 0;
    }
    
    return true;
}

void EnvironmentalDevice::createDefaultSensors(JsonArray& sensors) const {
    // SHT3x Temperature/Humidity sensor
    JsonObject sht3x = sensors.add<JsonObject>();
    sht3x["name"] = "sht3x";
    sht3x["type"] = "SHT3x";
    sht3x["pin"] = -1; // I2C sensor
    sht3x["i2c_address"] = 0x44;
    sht3x["enabled"] = true;
    sht3x["calibration_offset"] = 0.0;
    sht3x["calibration_scale"] = 1.0;
    sht3x["read_interval_ms"] = 2000;
    
    // Analog pressure sensor
    JsonObject pressure = sensors.add<JsonObject>();
    pressure["name"] = "pressure";
    pressure["type"] = "AnalogPressure";
    pressure["pin"] = 36; // ADC1_CH0
    pressure["i2c_address"] = 0;
    pressure["enabled"] = true;
    pressure["calibration_offset"] = 0.0;
    pressure["calibration_scale"] = 1.0;
    pressure["read_interval_ms"] = 1000;
}

void EnvironmentalDevice::createDefaultActuators(JsonArray& actuators) const {
    // Lights relay
    JsonObject lights = actuators.add<JsonObject>();
    lights["name"] = "lights";
    lights["type"] = "Relay";
    lights["pin"] = 23;
    lights["enabled"] = true;
    lights["invert_logic"] = false;
    lights["pulse_width_ms"] = 0;
    
    // Spray nozzle
    JsonObject spray = actuators.add<JsonObject>();
    spray["name"] = "spray";
    spray["type"] = "VenturiNozzle";
    spray["pin"] = 22;
    spray["enabled"] = true;
    spray["invert_logic"] = false;
    spray["pulse_width_ms"] = 5000; // 5 second spray cycles
    
    // Circulation fan (PWM controlled)
    JsonObject fan = actuators.add<JsonObject>();
    fan["name"] = "fan";
    fan["type"] = "PWMOutput";
    fan["pin"] = 21;
    fan["enabled"] = true;
    fan["invert_logic"] = false;
    fan["pulse_width_ms"] = 0;
}

void EnvironmentalDevice::createDefaultSafety(JsonObject& safety) const {
    // Environmental-specific safety limits
    safety["max_temperature_c"] = 45.0;  // Lower than general default
    safety["min_temperature_c"] = 5.0;   // Higher than general default
    safety["max_humidity_percent"] = 90.0; // Slightly lower for plant health
    safety["max_pressure_psi"] = 80.0;   // Appropriate for aeroponic systems
}

// SHT3xSensor implementation
bool SHT3xSensor::begin() {
    Serial.printf("Initializing SHT3x sensor: %s (addr: 0x%02X)\n", 
                 config.name.c_str(), i2cAddress);
    
    Wire.begin();
    
    // Perform soft reset
    if (!performSoftReset()) {
        lastReading.errorMessage = "Soft reset failed";
        return false;
    }
    
    delay(10); // Wait for reset to complete
    
    // Test communication with a measurement
    if (!sendCommand(SHT3X_CMD_MEASURE_HIGH_REP)) {
        lastReading.errorMessage = "Communication test failed";
        return false;
    }
    
    delay(20); // Wait for measurement
    
    uint8_t buffer[6];
    if (!readData(buffer, 6)) {
        lastReading.errorMessage = "Initial read test failed";
        return false;
    }
    
    initialized = true;
    Serial.printf("SHT3x sensor %s initialized successfully\n", config.name.c_str());
    return true;
}

SensorReading SHT3xSensor::read() {
    if (!initialized) {
        return SensorReading(); // Invalid reading
    }
    
    // Send measurement command
    if (!sendCommand(SHT3X_CMD_MEASURE_HIGH_REP)) {
        lastReading = SensorReading();
        lastReading.errorMessage = "Failed to send measurement command";
        return lastReading;
    }
    
    delay(20); // Wait for measurement to complete
    
    // Read 6 bytes (temp + CRC + humidity + CRC)
    uint8_t buffer[6];
    if (!readData(buffer, 6)) {
        lastReading = SensorReading();
        lastReading.errorMessage = "Failed to read sensor data";
        return lastReading;
    }
    
    // Verify CRCs
    if (calculateCRC8(&buffer[0], 2) != buffer[2] ||
        calculateCRC8(&buffer[3], 2) != buffer[5]) {
        lastReading = SensorReading();
        lastReading.errorMessage = "CRC check failed";
        return lastReading;
    }
    
    // Convert raw values
    uint16_t tempRaw = (buffer[0] << 8) | buffer[1];
    uint16_t humRaw = (buffer[3] << 8) | buffer[4];
    
    lastTemperature = -45 + 175 * (tempRaw / 65535.0);
    lastHumidity = 100 * (humRaw / 65535.0);
    
    // Apply calibration
    lastTemperature = (lastTemperature + config.calibrationOffset) * config.calibrationScale;
    lastHumidity = (lastHumidity + config.calibrationOffset) * config.calibrationScale;
    
    // Create combined reading - we'll publish separate events for each value
    lastReading = SensorReading(config.name + "_temp", "temperature", lastTemperature, "°C");
    lastReadTime = millis();
    
    // Publish separate events for temperature and humidity
    eventBus.publish(EnvEventTypes::SENSOR_TEMPERATURE, config.name,
                    "{\"value\":" + String(lastTemperature) + ",\"unit\":\"°C\"}");
    eventBus.publish(EnvEventTypes::SENSOR_HUMIDITY, config.name,
                    "{\"value\":" + String(lastHumidity) + ",\"unit\":\"%\"}");
    
    return lastReading;
}

void SHT3xSensor::shutdown() {
    initialized = false;
    Serial.printf("SHT3x sensor %s shutdown\n", config.name.c_str());
}

bool SHT3xSensor::performSoftReset() {
    return sendCommand(SHT3X_CMD_SOFT_RESET);
}

bool SHT3xSensor::sendCommand(uint16_t command) {
    Wire.beginTransmission(i2cAddress);
    Wire.write((command >> 8) & 0xFF); // MSB
    Wire.write(command & 0xFF);        // LSB
    return Wire.endTransmission() == 0;
}

bool SHT3xSensor::readData(uint8_t* buffer, size_t length) {
    Wire.requestFrom(i2cAddress, length);
    
    size_t bytesRead = 0;
    while (Wire.available() && bytesRead < length) {
        buffer[bytesRead++] = Wire.read();
    }
    
    return bytesRead == length;
}

uint8_t SHT3xSensor::calculateCRC8(const uint8_t* data, size_t length) {
    const uint8_t polynomial = 0x31;
    uint8_t crc = 0xFF;
    
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t bit = 8; bit > 0; --bit) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ polynomial;
            } else {
                crc = (crc << 1);
            }
        }
    }
    
    return crc;
}

// AnalogPressureSensor implementation
bool AnalogPressureSensor::begin() {
    Serial.printf("Initializing pressure sensor: %s (pin: %d)\n", 
                 config.name.c_str(), analogPin);
    
    // Configure ADC
    analogReadResolution(12); // 12-bit resolution
    analogSetAttenuation(ADC_11db); // 0-3.3V range
    
    // Test reading
    int rawValue = analogRead(analogPin);
    if (rawValue < 0) {
        lastReading.errorMessage = "Failed to read analog pin";
        return false;
    }
    
    initialized = true;
    Serial.printf("Pressure sensor %s initialized successfully\n", config.name.c_str());
    return true;
}

SensorReading AnalogPressureSensor::read() {
    if (!initialized) {
        return SensorReading();
    }
    
    // Read analog value
    int rawValue = analogRead(analogPin);
    if (rawValue < 0) {
        lastReading = SensorReading();
        lastReading.errorMessage = "Analog read failed";
        return lastReading;
    }
    
    // Convert to voltage (0-3.3V with 12-bit resolution)
    float voltage = (rawValue / 4095.0) * 3.3;
    
    // Convert voltage to pressure (MPa)
    float pressureMPa = voltageToMPa(voltage);
    
    // Convert to PSI
    float pressurePSI = MPaToPSI(pressureMPa);
    
    // Apply calibration
    pressurePSI = (pressurePSI + config.calibrationOffset) * config.calibrationScale;
    
    lastReading = SensorReading(config.name, "pressure", pressurePSI, "PSI");
    lastReadTime = millis();
    
    // Publish event
    eventBus.publish(EnvEventTypes::SENSOR_PRESSURE, config.name,
                    "{\"value\":" + String(pressurePSI) + ",\"unit\":\"PSI\"}");
    
    return lastReading;
}

void AnalogPressureSensor::shutdown() {
    initialized = false;
    Serial.printf("Pressure sensor %s shutdown\n", config.name.c_str());
}

float AnalogPressureSensor::voltageToMPa(float voltage) {
    // Linear interpolation between min/max voltage and pressure
    if (voltage <= minVoltage) return minPressure;
    if (voltage >= maxVoltage) return maxPressure;
    
    float ratio = (voltage - minVoltage) / (maxVoltage - minVoltage);
    return minPressure + ratio * (maxPressure - minPressure);
}

float AnalogPressureSensor::MPaToPSI(float mpa) {
    return mpa * 145.038; // 1 MPa = 145.038 PSI
}

// RelayActuator implementation
bool RelayActuator::begin() {
    Serial.printf("Initializing relay actuator: %s (pin: %d)\n", 
                 config.name.c_str(), relayPin);
    
    pinMode(relayPin, OUTPUT);
    
    // Set initial state (inactive)
    digitalWrite(relayPin, invertLogic ? HIGH : LOW);
    currentState = false;
    
    initialized = true;
    Serial.printf("Relay actuator %s initialized successfully\n", config.name.c_str());
    return true;
}

bool RelayActuator::activate() {
    if (!initialized) return false;
    
    digitalWrite(relayPin, invertLogic ? LOW : HIGH);
    currentState = true;
    lastActivationTime = millis();
    activationStartTime = lastActivationTime;
    
    // Publish event
    eventBus.publish(CoreEventTypes::ACTUATOR_ACTIVATED, config.name,
                    "{\"state\":true}");
    
    Serial.printf("Relay %s activated\n", config.name.c_str());
    return true;
}

bool RelayActuator::deactivate() {
    if (!initialized) return false;
    
    digitalWrite(relayPin, invertLogic ? HIGH : LOW);
    currentState = false;
    
    // Calculate activation duration
    unsigned long duration = millis() - activationStartTime;
    
    // Publish event
    eventBus.publish(CoreEventTypes::ACTUATOR_DEACTIVATED, config.name,
                    "{\"state\":false,\"duration_ms\":" + String(duration) + "}");
    
    Serial.printf("Relay %s deactivated (was active for %lu ms)\n", 
                 config.name.c_str(), duration);
    return true;
}

void RelayActuator::shutdown() {
    if (currentState) {
        deactivate();
    }
    initialized = false;
    Serial.printf("Relay actuator %s shutdown\n", config.name.c_str());
}

bool RelayActuator::pulse(unsigned long durationMs) {
    if (!activate()) return false;
    
    delay(durationMs);
    
    return deactivate();
}

// PWMActuator implementation
bool PWMActuator::begin() {
    Serial.printf("Initializing PWM actuator: %s (pin: %d, channel: %d)\n", 
                 config.name.c_str(), pwmPin, pwmChannel);
    
    // Configure PWM channel
    ledcSetup(pwmChannel, pwmFrequency, pwmResolution);
    ledcAttachPin(pwmPin, pwmChannel);
    
    // Set initial duty cycle to 0
    ledcWrite(pwmChannel, 0);
    currentDutyCycle = 0.0;
    currentState = false;
    
    initialized = true;
    Serial.printf("PWM actuator %s initialized successfully\n", config.name.c_str());
    return true;
}

bool PWMActuator::activate() {
    if (!initialized) return false;
    
    // Default to 50% duty cycle on activation
    return setDutyCycle(50.0);
}

bool PWMActuator::deactivate() {
    if (!initialized) return false;
    
    return setDutyCycle(0.0);
}

void PWMActuator::shutdown() {
    if (currentState) {
        deactivate();
    }
    ledcDetachPin(pwmPin);
    initialized = false;
    Serial.printf("PWM actuator %s shutdown\n", config.name.c_str());
}

bool PWMActuator::setDutyCycle(float dutyCycle) {
    if (!initialized) return false;
    
    // Clamp duty cycle to valid range
    dutyCycle = constrain(dutyCycle, 0.0, 100.0);
    
    // Calculate PWM value
    int pwmValue = (dutyCycle / 100.0) * ((1 << pwmResolution) - 1);
    
    ledcWrite(pwmChannel, pwmValue);
    currentDutyCycle = dutyCycle;
    currentState = (dutyCycle > 0);
    lastActivationTime = millis();
    
    // Publish event
    String eventType = currentState ? CoreEventTypes::ACTUATOR_ACTIVATED : 
                                     CoreEventTypes::ACTUATOR_DEACTIVATED;
    eventBus.publish(eventType, config.name,
                    "{\"duty_cycle\":" + String(dutyCycle) + ",\"state\":" + 
                    String(currentState ? "true" : "false") + "}");
    
    Serial.printf("PWM %s set to %.1f%% duty cycle\n", 
                 config.name.c_str(), dutyCycle);
    return true;
}

bool PWMActuator::setFrequency(int frequency) {
    if (!initialized) return false;
    
    pwmFrequency = frequency;
    ledcSetup(pwmChannel, pwmFrequency, pwmResolution);
    
    // Reapply current duty cycle
    return setDutyCycle(currentDutyCycle);
}

// VenturiNozzleActuator implementation
bool VenturiNozzleActuator::begin() {
    Serial.printf("Initializing venturi nozzle: %s (pin: %d)\n", 
                 config.name.c_str(), nozzlePin);
    
    pinMode(nozzlePin, OUTPUT);
    digitalWrite(nozzlePin, LOW); // Nozzle off initially
    
    sprayActive = false;
    currentState = false;
    
    initialized = true;
    Serial.printf("Venturi nozzle %s initialized successfully\n", config.name.c_str());
    return true;
}

bool VenturiNozzleActuator::activate() {
    return startSpray(sprayDurationMs);
}

bool VenturiNozzleActuator::deactivate() {
    return stopSpray();
}

void VenturiNozzleActuator::shutdown() {
    if (sprayActive) {
        stopSpray();
    }
    initialized = false;
    Serial.printf("Venturi nozzle %s shutdown\n", config.name.c_str());
}

bool VenturiNozzleActuator::startSpray(unsigned long durationMs) {
    if (!initialized) return false;
    
    if (durationMs == 0) {
        durationMs = sprayDurationMs;
    }
    
    digitalWrite(nozzlePin, HIGH);
    sprayActive = true;
    currentState = true;
    sprayStartTime = millis();
    sprayDurationMs = durationMs;
    lastActivationTime = sprayStartTime;
    
    // Update spray duration in config for future use
    if (durationMs != config.pulseWidthMs) {
        // This is a custom duration spray
    }
    
    // Publish event
    eventBus.publish(EnvEventTypes::ACTUATOR_SPRAY, config.name,
                    "{\"state\":true,\"duration_ms\":" + String(durationMs) + "}");
    
    Serial.printf("Spray %s started for %lu ms\n", config.name.c_str(), durationMs);
    return true;
}

bool VenturiNozzleActuator::stopSpray() {
    if (!initialized || !sprayActive) return false;
    
    digitalWrite(nozzlePin, LOW);
    sprayActive = false;
    currentState = false;
    
    unsigned long actualDuration = millis() - sprayStartTime;
    
    // Publish event
    eventBus.publish(EnvEventTypes::ACTUATOR_SPRAY, config.name,
                    "{\"state\":false,\"actual_duration_ms\":" + String(actualDuration) + "}");
    
    Serial.printf("Spray %s stopped (ran for %lu ms)\n", 
                 config.name.c_str(), actualDuration);
    return true;
}

void VenturiNozzleActuator::update() {
    if (!initialized || !sprayActive) return;
    
    // Check if spray duration has elapsed
    if (millis() - sprayStartTime >= sprayDurationMs) {
        stopSpray();
    }
}

// Global instance
EnvironmentalDevice environmentalDevice;