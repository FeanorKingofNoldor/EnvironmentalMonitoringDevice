#include "ActuatorManager.h"
#include "Relay.h"
#include "VenturiNozzle.h"
#include "Logger.h"

ActuatorManager::ActuatorManager() : isInitialized(false) {}

bool ActuatorManager::begin() {
    Logger::info("ActuatorMgr", "Initializing actuator manager...");
    
    subscribeToEvents();
    
    std::vector<ActuatorConfig> actuatorConfigs = config.getActuators();
    
    for (const auto& actuatorConfig : actuatorConfigs) {
        if (!actuatorConfig.enabled) {
            Logger::debug("ActuatorMgr", "Actuator " + actuatorConfig.name + " disabled, skipping");
            continue;
        }
        
        auto actuator = createActuator(actuatorConfig);
        if (actuator && actuator->begin()) {
            Logger::info("ActuatorMgr", "Initialized actuator: " + actuatorConfig.name);
            actuators.push_back(std::move(actuator));
        } else {
            Logger::error("ActuatorMgr", "Failed to initialize actuator: " + actuatorConfig.name);
        }
    }
    
    if (actuators.empty()) {
        Logger::warn("ActuatorMgr", "No actuators initialized");
    }
    
    isInitialized = true;
    Logger::info("ActuatorMgr", "Actuator manager ready with " + String(actuators.size()) + " actuators");
    return true;
}

std::unique_ptr<BaseActuator> ActuatorManager::createActuator(const ActuatorConfig& config) {
    Logger::debug("ActuatorMgr", "Creating actuator: " + config.name + " (type: " + config.type + ")");
    
    if (config.type == "Relay") {
        return std::make_unique<Relay>(config);
    } else if (config.type == "VenturiNozzle") {
        return std::make_unique<VenturiNozzle>(config);
    } else {
        Logger::error("ActuatorMgr", "Unknown actuator type: " + config.type);
        return nullptr;
    }
}

void ActuatorManager::subscribeToEvents() {
    eventBus.subscribe(EventTypes::ACTUATOR_LIGHTS_ON, [this](const Event& event) {
        handleLightCommand(event);
    });
    
    eventBus.subscribe(EventTypes::ACTUATOR_LIGHTS_OFF, [this](const Event& event) {
        handleLightCommand(event);
    });
    
    eventBus.subscribe(EventTypes::ACTUATOR_SPRAY_START, [this](const Event& event) {
        handleSprayCommand(event);
    });
    
    eventBus.subscribe(EventTypes::ACTUATOR_SPRAY_STOP, [this](const Event& event) {
        handleSprayCommand(event);
    });
}

void ActuatorManager::handleLightCommand(const Event& event) {
    BaseActuator* lights = getActuator("lights");
    if (!lights) {
        Logger::error("ActuatorMgr", "Lights actuator not found");
        return;
    }
    
    if (event.type == EventTypes::ACTUATOR_LIGHTS_ON) {
        lights->activate();
        Logger::info("ActuatorMgr", "Lights turned ON");
    } else if (event.type == EventTypes::ACTUATOR_LIGHTS_OFF) {
        lights->deactivate();
        Logger::info("ActuatorMgr", "Lights turned OFF");
    }
}

void ActuatorManager::handleSprayCommand(const Event& event) {
    BaseActuator* spray = getActuator("spray");
    if (!spray) {
        Logger::error("ActuatorMgr", "Spray actuator not found");
        return;
    }
    
    if (event.type == EventTypes::ACTUATOR_SPRAY_START) {
        spray->activate();
        Logger::info("ActuatorMgr", "Spray system started");
    } else if (event.type == EventTypes::ACTUATOR_SPRAY_STOP) {
        spray->deactivate();
        Logger::info("ActuatorMgr", "Spray system stopped");
    }
}

bool ActuatorManager::activateActuator(const String& name) {
    BaseActuator* actuator = getActuator(name);
    if (!actuator) {
        Logger::error("ActuatorMgr", "Actuator not found: " + name);
        return false;
    }
    
    actuator->activate();
    Logger::info("ActuatorMgr", "Activated actuator: " + name);
    return true;
}

bool ActuatorManager::deactivateActuator(const String& name) {
    BaseActuator* actuator = getActuator(name);
    if (!actuator) {
        Logger::error("ActuatorMgr", "Actuator not found: " + name);
        return false;
    }
    
    actuator->deactivate();
    Logger::info("ActuatorMgr", "Deactivated actuator: " + name);
    return true;
}

bool ActuatorManager::toggleActuator(const String& name) {
    BaseActuator* actuator = getActuator(name);
    if (!actuator) {
        Logger::error("ActuatorMgr", "Actuator not found: " + name);
        return false;
    }
    
    if (actuator->isActive()) {
        actuator->deactivate();
        Logger::info("ActuatorMgr", "Toggled OFF actuator: " + name);
    } else {
        actuator->activate();
        Logger::info("ActuatorMgr", "Toggled ON actuator: " + name);
    }
    
    return true;
}

BaseActuator* ActuatorManager::getActuator(const String& name) {
    for (auto& actuator : actuators) {
        if (actuator->getName() == name) {
            return actuator.get();
        }
    }
    return nullptr;
}

bool ActuatorManager::isAllActuatorsReady() const {
    for (const auto& actuator : actuators) {
        if (!actuator->isReady()) {
            return false;
        }
    }
    return true;
}

void ActuatorManager::printActuatorStatus() const {
    Logger::info("ActuatorMgr", "Actuator Status:");
    for (const auto& actuator : actuators) {
        String status = actuator->isReady() ? "READY" : "NOT READY";
        String state = actuator->isActive() ? "ACTIVE" : "INACTIVE";
        Logger::info("ActuatorMgr", "  " + actuator->getName() + ": " + status + " (" + state + ")");
    }
}

void ActuatorManager::shutdown() {
    Logger::info("ActuatorMgr", "Shutting down actuator manager");
    
    for (auto& actuator : actuators) {
        actuator->deactivate();
    }
    
    actuators.clear();
    isInitialized = false;
}