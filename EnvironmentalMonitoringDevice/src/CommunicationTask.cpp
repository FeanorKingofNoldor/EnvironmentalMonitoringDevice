#include "CommunicationTask.h"

CommunicationTask communicationTask;

CommunicationTask::CommunicationTask() 
    : commandTaskHandle(nullptr), sensorDataTaskHandle(nullptr), running(false) {
    latestSensorData = {0, 0, 0, 0};
}

CommunicationTask::~CommunicationTask() {
    stop();
}

void CommunicationTask::begin(const String& serverUrl, const String& deviceToken) {
    if (running) {
        Serial.println("CommunicationTask already running");
        return;
    }
    
    this->serverUrl = serverUrl;
    this->deviceToken = deviceToken;
    
    // Subscribe to sensor events
    eventBus.subscribe("sensor.temperature", [this](const Event& event) {
        this->onSensorEvent(event);
    });
    eventBus.subscribe("sensor.humidity", [this](const Event& event) {
        this->onSensorEvent(event);
    });
    eventBus.subscribe("sensor.pressure", [this](const Event& event) {
        this->onSensorEvent(event);
    });
    
    // Subscribe to command status events
    eventBus.subscribe("command.status", [this](const Event& event) {
        this->onCommandStatus(event);
    });
    
    // Create command polling task
    xTaskCreate(
        commandPollingTaskWrapper,
        "CommandPolling",
        4096,
        this,
        10, // Medium priority
        &commandTaskHandle
    );
    
    // Create sensor data task
    xTaskCreate(
        sensorDataTaskWrapper,
        "SensorData",
        4096,
        this,
        5, // Low priority
        &sensorDataTaskHandle
    );
    
    running = true;
    Serial.println("CommunicationTask started");
}

void CommunicationTask::stop() {
    running = false;
    
    if (commandTaskHandle != nullptr) {
        vTaskDelete(commandTaskHandle);
        commandTaskHandle = nullptr;
    }
    
    if (sensorDataTaskHandle != nullptr) {
        vTaskDelete(sensorDataTaskHandle);
        sensorDataTaskHandle = nullptr;
    }
    
    Serial.println("CommunicationTask stopped");
}

bool CommunicationTask::isRunning() const {
    return running;
}

void CommunicationTask::commandPollingTaskWrapper(void* parameter) {
    CommunicationTask* task = static_cast<CommunicationTask*>(parameter);
    task->commandPollingTask();
}

void CommunicationTask::sensorDataTaskWrapper(void* parameter) {
    CommunicationTask* task = static_cast<CommunicationTask*>(parameter);
    task->sensorDataTask();
}

void CommunicationTask::commandPollingTask() {
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(5000); // Poll every 5 seconds
    
    while (running) {
        if (WiFi.status() == WL_CONNECTED) {
            pollCommands();
        }
        
        vTaskDelayUntil(&lastWakeTime, frequency);
    }
    
    vTaskDelete(nullptr);
}

void CommunicationTask::sensorDataTask() {
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(60000); // Send every 60 seconds
    
    while (running) {
        if (WiFi.status() == WL_CONNECTED) {
            sendSensorData();
        }
        
        vTaskDelayUntil(&lastWakeTime, frequency);
    }
    
    vTaskDelete(nullptr);
}

bool CommunicationTask::pollCommands() {
    String response;
    bool success = makeHttpRequest("/api/v1/esp32/devices/commands", "GET", "", &response);
    
    if (success && !response.isEmpty()) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, response);
        
        if (!error) {
            JsonArray commands = doc["commands"];
            for (JsonObject command : commands) {
                String commandJson;
                serializeJson(command, commandJson);
                processCommand(commandJson);
            }
        }
    }
    
    return success;
}

bool CommunicationTask::sendSensorData() {
    String payload = createSensorPayload();
    return makeHttpRequest("/api/v1/esp32/sensor_data", "POST", payload);
}

void CommunicationTask::processCommand(const String& commandJson) {
    eventBus.publish("command.received", "CommunicationTask", commandJson);
}

String CommunicationTask::createSensorPayload() {
    JsonDocument doc;
    doc["timestamp"] = latestSensorData.timestamp;
    doc["temp"] = latestSensorData.temperature;
    doc["hum"] = latestSensorData.humidity;
    doc["press"] = latestSensorData.pressure;
    
    String payload;
    serializeJson(doc, payload);
    return payload;
}

bool CommunicationTask::makeHttpRequest(const String& endpoint, const String& method, const String& payload, String* response) {
    HTTPClient http;
    http.begin(serverUrl + endpoint);
    http.addHeader("Content-Type", "application/json");
    
    if (!deviceToken.isEmpty()) {
        http.addHeader("Authorization", "Bearer " + deviceToken);
    }
    
    int httpCode = -1;
    
    if (method == "GET") {
        httpCode = http.GET();
    } else if (method == "POST") {
        httpCode = http.POST(payload);
    } else if (method == "PATCH") {
        httpCode = http.PATCH(payload);
    }
    
    if (response != nullptr && httpCode == 200) {
        *response = http.getString();
    }
    
    http.end();
    
    if (httpCode != 200) {
        Serial.printf("HTTP request failed: %d\n", httpCode);
    }
    
    return (httpCode == 200);
}

void CommunicationTask::onSensorEvent(const Event& event) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, event.data);
    
    if (!error) {
        float value = doc["value"];
        
        if (event.type == "sensor.temperature") {
            latestSensorData.temperature = value;
        } else if (event.type == "sensor.humidity") {
            latestSensorData.humidity = value;
        } else if (event.type == "sensor.pressure") {
            latestSensorData.pressure = value;
        }
        
        latestSensorData.timestamp = event.timestamp / 1000; // Convert to seconds
    }
}

void CommunicationTask::onCommandStatus(const Event& event) {
    // Send command status confirmation back to server
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, event.data);
    
    if (!error) {
        String commandId = doc["command_id"];
        String endpoint = "/api/v1/esp32/devices/commands/" + commandId;
        makeHttpRequest(endpoint, "PATCH", event.data);
    }
}

void CommunicationTask::updateSensorData(float temp, float hum, float press) {
    latestSensorData.temperature = temp;
    latestSensorData.humidity = hum;
    latestSensorData.pressure = press;
    latestSensorData.timestamp = millis() / 1000;
}