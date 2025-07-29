#ifndef ACTUATOR_MANAGER_H
#define ACTUATOR_MANAGER_H

#include <Arduino.h>
#include <vector>
#include <memory>
#include "EventBus.h"
#include "Config.h"

// Forward declarations
class BaseActuator;

// Base actuator interface
class BaseActuator {
public:
    virtual ~BaseActuator() = default;
    virtual bool begin() = 0;
    virtual void activate() = 0;
    virtual void deactivate() = 0;
    virtual bool isActive() const = 0;
    virtual String getName() const = 0;
    virtual bool isReady() const = 0;
};

// Actuator manager for orchestrating all actuators
class ActuatorManager {
private:
    std::vector<std::unique_ptr<BaseActuator>> actuators;
    bool isInitialized;
    
    // Actuator creation
    std::unique_ptr<BaseActuator> createActuator(const ActuatorConfig& config);
    
    // Event handling
    void subscribeToEvents();
    void handleLightCommand(const Event& event);
    void handleSprayCommand(const Event& event);
    
public:
    ActuatorManager();
    ~ActuatorManager() = default;
    
    // Lifecycle
    bool begin();
    void shutdown();
    
    // Actuator operations
    bool activateActuator(const String& name);
    bool deactivateActuator(const String& name);
    bool toggleActuator(const String& name);
    BaseActuator* getActuator(const String& name);
    
    // Status
    size_t getActuatorCount() const { return actuators.size(); }
    bool isAllActuatorsReady() const;
    void printActuatorStatus() const;
    
    // Configuration
    bool addActuator(const ActuatorConfig& config);
    bool removeActuator(const String& name);
    bool reconfigure();
};

#endif // ACTUATOR_MANAGER_H