#ifndef EVENTBUS_H
#define EVENTBUS_H

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

// Event type constants
namespace EventTypes {
    // Sensor events
    constexpr const char* SENSOR_TEMPERATURE = "sensor.temperature";
    constexpr const char* SENSOR_HUMIDITY = "sensor.humidity";
    constexpr const char* SENSOR_PRESSURE = "sensor.pressure";
    constexpr const char* SENSOR_ERROR = "sensor.error";
    
    // Actuator events
    constexpr const char* ACTUATOR_LIGHTS_ON = "actuator.lights.on";
    constexpr const char* ACTUATOR_LIGHTS_OFF = "actuator.lights.off";
    constexpr const char* ACTUATOR_SPRAY_START = "actuator.spray.start";
    constexpr const char* ACTUATOR_SPRAY_STOP = "actuator.spray.stop";
    
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
}

// Thread-safe event bus for component communication
class EventBus {
private:
    std::map<String, std::vector<EventHandler>> subscribers;
    SemaphoreHandle_t mutex;
    static EventBus* instance;
    
    EventBus();
    void notifySubscribers(const Event& event);
    
public:
    static EventBus& getInstance();
    ~EventBus();
    
    // Subscribe to events of a specific type
    void subscribe(const String& eventType, EventHandler handler);
    
    // Publish event to all subscribers
    void publish(const Event& event);
    void publish(const String& eventType, const String& source, const String& data = "");
    
    // Get subscriber count for debugging
    size_t getSubscriberCount(const String& eventType) const;
    void printSubscribers() const;
};

// Global event bus instance
extern EventBus& eventBus;

#endif // EVENTBUS_H