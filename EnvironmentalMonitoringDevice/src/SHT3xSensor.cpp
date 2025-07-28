#include "SHT3xSensor.h"

SHT3xSensor::SHT3xSensor(TwoWire& wire, uint8_t address) 
    : wire(&wire), address(address), temperature(0), humidity(0), 
      connected(false), lastRead(0) {}

bool SHT3xSensor::begin() {
    wire->begin();
    
    // Test communication
    if (sendCommand(0x2C06)) { // High repeatability measurement
        vTaskDelay(pdMS_TO_TICKS(15)); // Wait for measurement
        uint8_t data[6];
        if (readData(data, 6)) {
            connected = true;
            Serial.println("SHT3x sensor initialized successfully");
            return true;
        }
    }
    
    Serial.println("SHT3x sensor initialization failed");
    connected = false;
    return false;
}

void SHT3xSensor::read() {
    if (!connected || (millis() - lastRead) < 1000) {
        return; // Don't read too frequently
    }
    
    if (sendCommand(0x2C06)) { // High repeatability measurement
        vTaskDelay(pdMS_TO_TICKS(15)); // Wait for measurement
        
        uint8_t data[6];
        if (readData(data, 6)) {
            // Check CRC for temperature
            if (calculateCRC(&data[0], 2) == data[2]) {
                uint16_t tempRaw = (data[0] << 8) | data[1];
                temperature = -45 + (175 * tempRaw / 65535.0);
                
                // Check CRC for humidity
                if (calculateCRC(&data[3], 2) == data[5]) {
                    uint16_t humRaw = (data[3] << 8) | data[4];
                    humidity = 100 * humRaw / 65535.0;
                    
                    lastRead = millis();
                    
                    // Publish sensor events
                    String tempData = "{\"value\":" + String(temperature) + "}";
                    String humData = "{\"value\":" + String(humidity) + "}";
                    
                    eventBus.publish("sensor.temperature", "SHT3xSensor", tempData);
                    eventBus.publish("sensor.humidity", "SHT3xSensor", humData);
                    
                    connected = true;
                    return;
                }
            }
        }
    }
    
    // Communication failed
    connected = false;
    eventBus.publish("sensor.error", "SHT3xSensor", "{\"error\":\"Communication failed\"}");
}

bool SHT3xSensor::isConnected() const {
    return connected;
}

String SHT3xSensor::getName() const {
    return "SHT3x";
}

float SHT3xSensor::getTemperature() const {
    return temperature;
}

float SHT3xSensor::getHumidity() const {
    return humidity;
}

bool SHT3xSensor::sendCommand(uint16_t command) {
    wire->beginTransmission(address);
    wire->write(command >> 8);
    wire->write(command & 0xFF);
    return (wire->endTransmission() == 0);
}

bool SHT3xSensor::readData(uint8_t* data, size_t length) {
    if (wire->requestFrom(address, length) == length) {
        for (size_t i = 0; i < length; i++) {
            data[i] = wire->read();
        }
        return true;
    }
    return false;
}

uint8_t SHT3xSensor::calculateCRC(uint8_t* data, size_t length) {
    uint8_t crc = 0xFF;
    
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc <<= 1;
            }
        }
    }
    
    return crc;
}