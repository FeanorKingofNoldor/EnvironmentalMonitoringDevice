#include "EventBus.h"

EventBus eventBus;

EventBus::EventBus() {
    mutex = xSemaphoreCreateMutex();
}

EventBus::~EventBus() {
    if (mutex != NULL) {
        vSemaphoreDelete(mutex);
    }
}

void EventBus::subscribe(const String& eventType, EventHandler handler) {
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        subscribers[eventType].push_back(handler);
        xSemaphoreGive(mutex);
    }
}

void EventBus::publish(const Event& event) {
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        notifySubscribers(event);
        xSemaphoreGive(mutex);
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
            handler(event);
        }
    }
}