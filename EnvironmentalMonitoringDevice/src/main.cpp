#include <WiFi.h>
#include <Wire.h>
#include <vector>
#include "driver/adc.h"

// Core system
#include "EventBus.h"
#include "Config.h"
#include "CommandHandler.h"

// Sensors
#include "SHT3xSensor.h"
#include "PressureTransducer.h"

// Actuators
#include "VenturiNozzle.h"
#include "Relay.h"

// Tasks
#include "SensorTask.h"
#include "CommunicationTask.h"

// Display Communication
#include "DisplayUARTHandler.h"

// System components
SHT3xSensor* sht3x = nullptr;
PressureTransducer* pressureTransducer = nullptr;
std::vector<VenturiNozzle*> nozzles;
Relay* lightsRelay = nullptr;
Relay* compressorRelay = nullptr;

void setup() {
    Serial.begin(115200);
    Serial.println("AeroEnv ESP32 Starting...");
    
    // Initialize configuration
    if (!config.load()) {
        Serial.println("Using default configuration");
        createDefaultConfig();
    }
    
    // Initialize WiFi
    initializeWiFi();
    
    // Initialize I2C
    Wire.begin();
    
    // Initialize sensors
    initializeSensors();
    
    // Initialize actuators
    initializeActuators();
    
    // Initialize command handler
    commandHandler.begin();
    
    // Initialize display communication
    displayUARTHandler.begin();
    
    // Subscribe to actuator events
    subscribeToActuatorEvents();
    
    // Create display communication task
    xTaskCreatePinnedToCore(
        displayCommTask,
        "DisplayComm",
        4096,
        nullptr,
        10,                      // Medium priority
        nullptr,
        0                        // Core 0 (communication/protocol core)
    );
    
    // Start tasks
    sensorTask.begin();
    
    NetworkConfig network = config.getNetwork();
    communicationTask.begin(network.server_url, network.device_token);
    
    Serial.println("AeroEnv ESP32 initialization complete");
}

void loop() {
    // Main loop is minimal - everything runs in FreeRTOS tasks
    vTaskDelay(pdMS_TO_TICKS(1000));
}

void displayCommTask(void* pvParameters) {
    while (true) {
        displayUARTHandler.processDisplayMessages();
        vTaskDelay(pdMS_TO_TICKS(100));  // Check for display messages every 100ms
    }
}

void createDefaultConfig() {
    // Create default configuration for development
    config.set("network/wifi_ssid", "");
    config.set("network/wifi_password", "");
    config.set("network/server_url", "http://localhost:3000");
    config.set("network/device_token", "");
    
    config.save();
    Serial.println("Default configuration created");
}

void initializeWiFi() {
    NetworkConfig network = config.getNetwork();
    
    if (network.wifi_ssid.isEmpty()) {
        Serial.println("WiFi not configured, skipping connection");
        return;
    }
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(network.wifi_ssid.c_str(), network.wifi_password.c_str());
    
    Serial.print("Connecting to WiFi");
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        Serial.print(".");
        vTaskDelay(pdMS_TO_TICKS(500));
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\nWiFi connected: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("\nWiFi connection failed");
    }
}

void initializeSensors() {
    // Initialize SHT3x sensor
    sht3x = new SHT3xSensor(Wire, 0x45);
    sensorTask.addSensor(sht3x);
    
    // Initialize pressure transducer (example pin)
    pressureTransducer = new PressureTransducer(ADC1_CHANNEL_0);
    sensorTask.addSensor(pressureTransducer);
    
    Serial.println("Sensors initialized");
}

void initializeActuators() {
    // Initialize 4 venturi nozzles (example pins)
    for (int i = 0; i < 4; i++) {
        VenturiNozzle* nozzle = new VenturiNozzle(
            16 + i * 2,     // Air solenoid pins: 16, 18, 20, 22
            17 + i * 2,     // Nutrient solenoid pins: 17, 19, 21, 23
            i + 1           // Nozzle ID: 1, 2, 3, 4
        );
        nozzle->begin();
        nozzles.push_back(nozzle);
    }
    
    // Initialize relays (example pins)
    lightsRelay = new Relay(32, "lights");
    lightsRelay->begin();
    
    compressorRelay = new Relay(33, "compressor");
    compressorRelay->begin();
    
    Serial.println("Actuators initialized");
}

void subscribeToActuatorEvents() {
    // Subscribe to relay control events
    eventBus.subscribe("actuator.relay.set", [](const Event& event) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, event.data);
        
        if (!error) {
            String relayName = doc["relay"];
            bool state = doc["state"];
            
            if (relayName == "lights" && lightsRelay) {
                lightsRelay->setState(state);
            } else if (relayName == "compressor" && compressorRelay) {
                compressorRelay->setState(state);
            }
        }
    });
    
    // Subscribe to relay toggle events (for display manual control)
    eventBus.subscribe("actuator.relay.toggle", [](const Event& event) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, event.data);
        
        if (!error) {
            String relayName = doc["relay"];
            
            if (relayName == "lights" && lightsRelay) {
                lightsRelay->toggle();
            } else if (relayName == "compressor" && compressorRelay) {
                compressorRelay->toggle();
            }
        }
    });
    
    // Subscribe to nozzle activation events
    eventBus.subscribe("actuator.nozzle.activate", [](const Event& event) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, event.data);
        
        if (!error) {
            int nozzleId = doc["nozzle"];
            if (nozzleId >= 1 && nozzleId <= nozzles.size()) {
                nozzles[nozzleId - 1]->startSprayCycle();
            }
        }
    });
    
    // Subscribe to spray control events
    eventBus.subscribe("actuator.spray.start", [](const Event& event) {
        // Start continuous spraying on all nozzles
        for (VenturiNozzle* nozzle : nozzles) {
            nozzle->startSprayCycle();
        }
    });
    
    eventBus.subscribe("actuator.spray.stop", [](const Event& event) {
        // Stop all nozzles
        for (VenturiNozzle* nozzle : nozzles) {
            nozzle->stopSpray();
        }
    });
    
    Serial.println("Actuator event subscriptions configured");
}