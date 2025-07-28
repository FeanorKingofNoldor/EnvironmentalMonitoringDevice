#ifndef SHT3X_SENSOR_H
#define SHT3X_SENSOR_H

#include "Interfaces.h"
#include "EventBus.h"
#include <Wire.h>

class SHT3xSensor : public ISensor {
private:
    TwoWire* wire;
    uint8_t address;
    float temperature;
    float humidity;
    bool connected;
    unsigned long lastRead;
    
    bool sendCommand(uint16_t command);
    bool readData(uint8_t* data, size_t length);
    uint8_t calculateCRC(uint8_t* data, size_t length);
    
public:
    SHT3xSensor(TwoWire& wire, uint8_t address = 0x45);
    
    bool begin() override;
    void read() override;
    bool isConnected() const override;
    String getName() const override;
    
    float getTemperature() const;
    float getHumidity() const;
};

#endif