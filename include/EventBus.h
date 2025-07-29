// File: src/core/EventBus.h
#ifndef CORE_EVENTBUS_H
#define CORE_EVENTBUS_H

#include <Arduino.h>
#include <functional>
#include <map>
#include <vector>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// Event structure for system communication
struct Event {
    String type;                // Event type identifier
    String source;              // Component that generated event
    String data;               // JSON data payload
    unsigned long timestamp;    // Event timestamp
    
    Event(const String& t, const String& s, const String& d = "") 
        : type(t), source(s), data(d), timestamp(millis()) {}
};

// Event handler function type
using EventHandler = std::function<void(const Event&)>;

// Core event types (device-agnostic)
namespace CoreEventTypes {
    // Sensor events (generic)
    constexpr const char* SENSOR_READING = "sensor.reading";
    constexpr const char* SENSOR_ERROR = "sensor.error";
    constexpr const char* SENSOR_CONNECTED = "sensor.connected";
    constexpr const char* SENSOR_DISCONNECTED = "sensor.disconnected";
    
    // Actuator events (generic)
    constexpr const char* ACTUATOR_ACTIVATED = "actuator.activated";
    constexpr const char* ACTUATOR_DEACTIVATED = "actuator.deactivated";
    constexpr const char* ACTUATOR_ERROR = "actuator.error";
    
    // System events
    constexpr const char* SYSTEM_STARTUP = "system.startup";
    constexpr const char* SYSTEM_SHUTDOWN = "system.shutdown";
    constexpr const char* SYSTEM_ERROR = "system.error";
    constexpr const char* SYSTEM_WIFI_CONNECTED = "system.wifi.connected";
    constexpr const char* SYSTEM_WIFI_DISCONNECTED = "system.wifi.disconnected";
    
    // Command events
    constexpr const char* COMMAND_RECEIVED = "command.received";
    constexpr const char* COMMAND_EXECUTED = "command.executed";
    constexpr const char* COMMAND_FAILED = "command.failed";
    
    // Configuration events
    constexpr const char* CONFIG_LOADED = "config.loaded";
    constexpr const char* CONFIG_CHANGED = "config.changed";
    constexpr const char* CONFIG_SAVED = "config.saved";
}

// Thread-safe event bus for component communication
class EventBus {
private:
    std::map<String, std::vector<EventHandler>> subscribers;
    SemaphoreHandle_t mutex;
    static EventBus* instance;
    
    EventBus();
    
    // Prevent copying
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;
    
public:
    static EventBus& getInstance();
    ~EventBus();
    
    // Subscribe to events of a specific type
    bool subscribe(const String& eventType, EventHandler handler);
    
    // Unsubscribe from events (for cleanup)
    bool unsubscribe(const String& eventType);
    
    // Publish event to all subscribers
    void publish(const Event& event);
    void publish(const String& eventType, const String& source, const String& data = "");
    
    // Diagnostics
    size_t getSubscriberCount(const String& eventType) const;
    size_t getTotalEventTypes() const;
    void printSubscribers() const;
    
    // Cleanup
    void shutdown();
};

// Global event bus instance access
extern EventBus& eventBus;

// Helper macros for common event publishing
#define PUBLISH_SENSOR_READING(sensorName, value, unit) \
    eventBus.publish(CoreEventTypes::SENSOR_READING, sensorName, \
    "{\"value\":" + String(value) + ",\"unit\":\"" + unit + "\"}")

#define PUBLISH_ACTUATOR_STATE(actuatorName, state) \
    eventBus.publish(state ? CoreEventTypes::ACTUATOR_ACTIVATED : CoreEventTypes::ACTUATOR_DEACTIVATED, \
    actuatorName, "{\"state\":" + String(state ? "true" : "false") + "}")

#define PUBLISH_SYSTEM_ERROR(component, message) \
    eventBus.publish(CoreEventTypes::SYSTEM_ERROR, component, \
    "{\"error\":\"" + message + "\"}")

#endif // CORE_EVENTBUS_H