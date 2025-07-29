#ifndef RELAY_H
#define RELAY_H

#include "ActuatorManager.h"

class Relay : public BaseActuator {
private:
    ActuatorConfig config;
    bool initialized;
    bool active;
    unsigned long activationTime;
    
public:
    explicit Relay(const ActuatorConfig& cfg);
    ~Relay() override = default;
    
    bool begin() override;
    void activate() override;
    void deactivate() override;
    bool isActive() const override { return active; }
    String getName() const override { return config.name; }
    bool isReady() const override { return initialized; }
    
    // Relay specific methods
    void pulse(int durationMs);
    unsigned long getActivationTime() const { return activationTime; }
};

#endif // RELAY_H