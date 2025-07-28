#ifndef RELAY_H
#define RELAY_H

#include "Interfaces.h"
#include "EventBus.h"

class Relay : public IActuator {
private:
    int pin;
    String relayName;
    bool state;
    
public:
    Relay(int pin, const String& name);
    
    bool begin() override;
    void setState(bool state) override;
    bool getState() const override;
    String getName() const override;
    
    void toggle();
};

#endif