#ifndef EVENTBUS_H
#define EVENTBUS_H

#include <Arduino.h>
#include <vector>
#include <functional>
#include <map>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

struct Event {
    String type;
    String source;
    unsigned long timestamp;
    String data; // JSON string
    
    Event(const String& t, const String& s, const String& d = "") 
        : type(t), source(s), data(d), timestamp(millis()) {}
};

typedef std::function<void(const Event&)> EventHandler;

class EventBus {
private:
    std::map<String, std::vector<EventHandler>> subscribers;
    SemaphoreHandle_t mutex;
    
public:
    EventBus();
    ~EventBus();
    
    void subscribe(const String& eventType, EventHandler handler);
    void publish(const Event& event);
    void publish(const String& eventType, const String& source, const String& data = "");
    
private:
    void notifySubscribers(const Event& event);
};

extern EventBus eventBus;

#endif