#ifndef PRESSURE_TRANSDUCER_H
#define PRESSURE_TRANSDUCER_H

#include "Interfaces.h"
#include "EventBus.h"
#include "driver/adc.h"

class PressureTransducer : public ISensor {
private:
    adc1_channel_t adcChannel;
    float pressure;
    bool connected;
    unsigned long lastRead;
    
    float readPressure();
    
public:
    PressureTransducer(adc1_channel_t channel);
    
    bool begin() override;
    void read() override;
    bool isConnected() const override;
    String getName() const override;
    
    float getPressure() const;
};

#endif