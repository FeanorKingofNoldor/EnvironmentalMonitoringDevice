// File: src/core/EventBus.cpp
#include "EventBus.h"

// Static instance
EventBus* EventBus::instance = nullptr;

EventBus::EventBus() {
    mutex = xSemaphoreCreateMutex();
    if (mutex == nullptr) {
        Serial.println("FATAL: Failed to create EventBus mutex");
    }
}

EventBus& EventBus::getInstance() {
    if (instance == nullptr) {
        instance = new EventBus();
    }
    return *instance;
}

EventBus::~EventBus() {
    shutdown();
}

bool EventBus::subscribe(const String& eventType, EventHandler handler) {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        subscribers[eventType].push_back(handler);
        xSemaphoreGive(mutex);
        Serial.printf("EventBus: Subscribed to '%s' (total: %d)\n", 
                     eventType.c_str(), subscribers[eventType].size());
        return true;
    }
    Serial.printf("EventBus: Failed to subscribe to '%s' - mutex timeout\n", eventType.c_str());
    return false;
}

bool EventBus::unsubscribe(const String& eventType) {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        auto it = subscribers.find(eventType);
        if (it != subscribers.end()) {
            subscribers.erase(it);
            xSemaphoreGive(mutex);
            Serial.printf("EventBus: Unsubscribed from '%s'\n", eventType.c_str());
            return true;
        }
        xSemaphoreGive(mutex);
    }
    return false;
}

void EventBus::publish(const Event& event) {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        auto it = subscribers.find(event.type);
        if (it != subscribers.end()) {
            // Call all handlers for this event type
            for (const auto& handler : it->second) {
                try {
                    handler(event);
                } catch (const std::exception& e) {
                    Serial.printf("EventBus: Handler exception for '%s': %s\n", 
                                 event.type.c_str(), e.what());
                } catch (...) {
                    Serial.printf("EventBus: Unknown handler exception for '%s'\n", 
                                 event.type.c_str());
                }
            }
        }
        xSemaphoreGive(mutex);
    } else {
        Serial.printf("EventBus: Publish timeout for '%s'\n", event.type.c_str());
    }
}

void EventBus::publish(const String& eventType, const String& source, const String& data) {
    Event event(eventType, source, data);
    publish(event);
}

size_t EventBus::getSubscriberCount(const String& eventType) const {
    size_t count = 0;
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        auto it = subscribers.find(eventType);
        if (it != subscribers.end()) {
            count = it->second.size();
        }
        xSemaphoreGive(mutex);
    }
    return count;
}

size_t EventBus::getTotalEventTypes() const {
    size_t count = 0;
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        count = subscribers.size();
        xSemaphoreGive(mutex);
    }
    return count;
}

void EventBus::printSubscribers() const {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        Serial.println("=== EventBus Subscribers ===");
        for (const auto& pair : subscribers) {
            Serial.printf("  %s: %d handlers\n", pair.first.c_str(), pair.second.size());
        }
        Serial.printf("Total event types: %d\n", subscribers.size());
        Serial.println("============================");
        xSemaphoreGive(mutex);
    }
}

void EventBus::shutdown() {
    if (mutex != nullptr) {
        xSemaphoreTake(mutex, portMAX_DELAY);
        subscribers.clear();
        xSemaphoreGive(mutex);
        vSemaphoreDelete(mutex);
        mutex = nullptr;
    }
}

// Global instance
EventBus& eventBus = EventBus::getInstance();