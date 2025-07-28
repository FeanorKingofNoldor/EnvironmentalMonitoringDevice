#include "Relay.h"

Relay::Relay(int pin, const String& name) 
    : pin(pin), relayName(name), state(false) {}

bool Relay::begin() {
    pinMode(pin, OUTPUT);
    setState(false); // Start in safe state (off)
    
    Serial.printf("Relay %s initialized on pin %d\n", relayName.c_str(), pin);
    return true;
}

void Relay::setState(bool newState) {
    state = newState;
    digitalWrite(pin, state ? HIGH : LOW);
    
    String data = "{\"relay\":\"" + relayName + "\",\"state\":" + (state ? "true" : "false") + "}";
    eventBus.publish("actuator.relay.changed", getName(), data);
    
    Serial.printf("Relay %s %s\n", relayName.c_str(), state ? "ON" : "OFF");
}

bool Relay::getState() const {
    return state;
}

String Relay::getName() const {
    return relayName;
}

void Relay::toggle() {
    setState(!state);
}