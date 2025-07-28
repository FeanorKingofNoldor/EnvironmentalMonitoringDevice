#include "VenturiNozzle.h"

VenturiNozzle::VenturiNozzle(int airPin, int nutrientPin, int nozzleId) 
    : airPin(airPin), nutrientPin(nutrientPin), nozzleId(nozzleId),
      state(NozzleState::IDLE), airState(false), nutrientState(false),
      stateStartTime(0), taskHandle(nullptr),
      pressurizeDelay(1000), sprayDuration(5000), purgeDelay(1000) {}

VenturiNozzle::~VenturiNozzle() {
    if (taskHandle != nullptr) {
        vTaskDelete(taskHandle);
    }
}

bool VenturiNozzle::begin() {
    pinMode(airPin, OUTPUT);
    pinMode(nutrientPin, OUTPUT);
    
    // Ensure both solenoids start closed (safe state)
    setAirSolenoid(false);
    setNutrientSolenoid(false);
    
    Serial.printf("Venturi nozzle %d initialized\n", nozzleId);
    return true;
}

void VenturiNozzle::setState(bool state) {
    if (state) {
        startSprayCycle();
    } else {
        stopSpray();
    }
}

bool VenturiNozzle::getState() const {
    return state != NozzleState::IDLE;
}

String VenturiNozzle::getName() const {
    return "VenturiNozzle" + String(nozzleId);
}

void VenturiNozzle::startSprayCycle() {
    if (state != NozzleState::IDLE) {
        Serial.printf("Nozzle %d already active\n", nozzleId);
        return;
    }
    
    Serial.printf("Starting spray cycle for nozzle %d\n", nozzleId);
    
    // Create spray task
    String taskName = "Spray" + String(nozzleId);
    xTaskCreate(
        sprayTaskWrapper,
        taskName.c_str(),
        2048,
        this,
        5, // Medium priority
        &taskHandle
    );
}

void VenturiNozzle::stopSpray() {
    if (taskHandle != nullptr) {
        vTaskDelete(taskHandle);
        taskHandle = nullptr;
    }
    
    // Immediately close both solenoids
    setAirSolenoid(false);
    setNutrientSolenoid(false);
    state = NozzleState::IDLE;
    
    Serial.printf("Spray stopped for nozzle %d\n", nozzleId);
}

void VenturiNozzle::sprayTaskWrapper(void* parameter) {
    VenturiNozzle* nozzle = static_cast<VenturiNozzle*>(parameter);
    nozzle->sprayTask();
}

void VenturiNozzle::sprayTask() {
    // Step 1: Open air solenoid (pressurize)
    state = NozzleState::PRESSURIZING;
    setAirSolenoid(true);
    stateStartTime = millis();
    
    eventBus.publish("actuator.nozzle.air.open", getName(), 
                    "{\"nozzle\":" + String(nozzleId) + "}");
    
    // Step 2: Wait for pressurization
    vTaskDelay(pdMS_TO_TICKS(pressurizeDelay));
    
    // Step 3: Open nutrient solenoid (spray)
    state = NozzleState::SPRAYING;
    setNutrientSolenoid(true);
    
    eventBus.publish("actuator.nozzle.nutrient.open", getName(), 
                    "{\"nozzle\":" + String(nozzleId) + "}");
    
    // Step 4: Spray for specified duration
    vTaskDelay(pdMS_TO_TICKS(sprayDuration));
    
    // Step 5: Close nutrient solenoid first
    setNutrientSolenoid(false);
    
    eventBus.publish("actuator.nozzle.nutrient.close", getName(), 
                    "{\"nozzle\":" + String(nozzleId) + "}");
    
    // Step 6: Air purge (self-cleaning)
    state = NozzleState::PURGING;
    vTaskDelay(pdMS_TO_TICKS(purgeDelay));
    
    // Step 7: Close air solenoid
    setAirSolenoid(false);
    state = NozzleState::IDLE;
    
    eventBus.publish("actuator.nozzle.air.close", getName(), 
                    "{\"nozzle\":" + String(nozzleId) + "}");
    
    Serial.printf("Spray cycle completed for nozzle %d\n", nozzleId);
    
    // Task will self-delete
    taskHandle = nullptr;
    vTaskDelete(nullptr);
}

void VenturiNozzle::setAirSolenoid(bool state) {
    airState = state;
    digitalWrite(airPin, state ? HIGH : LOW);
}

void VenturiNozzle::setNutrientSolenoid(bool state) {
    nutrientState = state;
    digitalWrite(nutrientPin, state ? HIGH : LOW);
}

NozzleState VenturiNozzle::getCurrentState() const {
    return state;
}

void VenturiNozzle::setPressurizeDelay(unsigned long ms) {
    pressurizeDelay = ms;
}

void VenturiNozzle::setSprayDuration(unsigned long ms) {
    sprayDuration = ms;
}

void VenturiNozzle::setPurgeDelay(unsigned long ms) {
    purgeDelay = ms;
}