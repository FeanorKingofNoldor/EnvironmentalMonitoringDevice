#include "EventBus.h"

EventBus* EventBus::instance = nullptr;

EventBus& EventBus::getInstance() {
    if (instance == nullptr) {
        instance = new EventBus();
    }
    return *instance;
}

EventBus::EventBus() {
    mutex = xSemaphoreCreateMutex();
    if (mutex == nullptr) {
        Serial.println("ERROR: Failed to create EventBus mutex");
    }
}

EventBus::~EventBus() {
    if (mutex != nullptr) {
        vSemaphoreDelete(mutex);
    }
}

void EventBus::subscribe(const String& eventType, EventHandler handler) {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        subscribers[eventType].push_back(handler);
        xSemaphoreGive(mutex);
        
        Serial.printf("EventBus: Subscribed to '%s' (total: %d)\n", 
                     eventType.c_str(), subscribers[eventType].size());
    } else {
        Serial.printf("ERROR: EventBus subscribe timeout for '%s'\n", eventType.c_str());
    }
}

void EventBus::publish(const Event& event) {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        notifySubscribers(event);
        xSemaphoreGive(mutex);
    } else {
        Serial.printf("ERROR: EventBus publish timeout for '%s'\n", event.type.c_str());
    }
}

void EventBus::publish(const String& eventType, const String& source, const String& data) {
    Event event(eventType, source, data);
    publish(event);
}

void EventBus::notifySubscribers(const Event& event) {
    auto it = subscribers.find(event.type);
    if (it != subscribers.end()) {
        for (const auto& handler : it->second) {
            try {
                handler(event);
            } catch (const std::exception& e) {
                Serial.printf("ERROR: Event handler exception for '%s': %s\n", 
                             event.type.c_str(), e.what());
            } catch (...) {
                Serial.printf("ERROR: Unknown event handler exception for '%s'\n", 
                             event.type.c_str());
            }
        }
    }
}

size_t EventBus::getSubscriberCount(const String& eventType) const {
    auto it = subscribers.find(eventType);
    return (it != subscribers.end()) ? it->second.size() : 0;
}

void EventBus::printSubscribers() const {
    Serial.println("EventBus Subscribers:");
    for (const auto& pair : subscribers) {
        Serial.printf("  %s: %d handlers\n", pair.first.c_str(), pair.second.size());
    }
}

// Global event bus instance
EventBus& eventBus = EventBus::getInstance();