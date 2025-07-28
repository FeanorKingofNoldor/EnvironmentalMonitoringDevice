#include "PressureTransducer.h"

PressureTransducer::PressureTransducer(adc1_channel_t channel) 
    : adcChannel(channel), pressure(0), connected(false), lastRead(0) {}

bool PressureTransducer::begin() {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(adcChannel, ADC_ATTEN_DB_11);
    
    // Test reading
    float testPressure = readPressure();
    if (testPressure >= 0 && testPressure < 200) { // Reasonable pressure range
        connected = true;
        Serial.println("Pressure transducer initialized successfully");
        return true;
    }
    
    Serial.println("Pressure transducer initialization failed");
    connected = false;
    return false;
}

void PressureTransducer::read() {
    if (!connected || (millis() - lastRead) < 1000) {
        return; // Don't read too frequently
    }
    
    float newPressure = readPressure();
    
    if (newPressure >= 0 && newPressure < 200) { // Sanity check
        pressure = newPressure;
        lastRead = millis();
        
        // Publish sensor event
        String data = "{\"value\":" + String(pressure) + "}";
        eventBus.publish("sensor.pressure", "PressureTransducer", data);
        
        connected = true;
    } else {
        connected = false;
        eventBus.publish("sensor.error", "PressureTransducer", "{\"error\":\"Invalid reading\"}");
    }
}

bool PressureTransducer::isConnected() const {
    return connected;
}

String PressureTransducer::getName() const {
    return "PressureTransducer";
}

float PressureTransducer::getPressure() const {
    return pressure;
}

float PressureTransducer::readPressure() {
    int sensorValue = adc1_get_raw(adcChannel);
    float voltage = sensorValue * (3.3 / 4095.0);
    
    // Honeywell pressure transducer conversion
    // 0.25V = 0 PSI, 2.25V = 100 PSI (example range)
    float pressure_psi = (voltage - 0.25) * (100 / (2.25 - 0.25));
    
    return pressure_psi;
}