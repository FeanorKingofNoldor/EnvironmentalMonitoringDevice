#ifndef VENTURI_NOZZLE_H
#define VENTURI_NOZZLE_H

#include "Interfaces.h"
#include "EventBus.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

enum class NozzleState {
    IDLE,
    PRESSURIZING,
    SPRAYING,
    PURGING
};

class VenturiNozzle : public IActuator {
private:
    int airPin;
    int nutrientPin;
    int nozzleId;
    NozzleState state;
    bool airState;
    bool nutrientState;
    unsigned long stateStartTime;
    TaskHandle_t taskHandle;
    
    // Timing parameters (configurable)
    unsigned long pressurizeDelay;
    unsigned long sprayDuration;
    unsigned long purgeDelay;
    
    static void sprayTaskWrapper(void* parameter);
    void sprayTask();
    void setAirSolenoid(bool state);
    void setNutrientSolenoid(bool state);
    
public:
    VenturiNozzle(int airPin, int nutrientPin, int nozzleId);
    ~VenturiNozzle();
    
    bool begin() override;
    void setState(bool state) override;
    bool getState() const override;
    String getName() const override;
    
    void startSprayCycle();
    void stopSpray();
    NozzleState getCurrentState() const;
    
    // Configuration
    void setPressurizeDelay(unsigned long ms);
    void setSprayDuration(unsigned long ms);
    void setPurgeDelay(unsigned long ms);
};

#endif