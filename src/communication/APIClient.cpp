#include "APIClient.h"
#include "Logger.h"
#include "NetworkUtils.h"

APIClient::APIClient() : initialized(false), lastPollTime(0), lastUploadTime(0), 
                         pollIntervalMs(5000), uploadIntervalMs(30000) {}

APIClient::~APIClient() {
    shutdown();
}

bool APIClient::begin() {
    Logger::info("APIClient", "Initializing API client...");
    
    NetworkConfig network = config.getNetwork();
    serverURL = network.serverURL;
    deviceToken = network.deviceToken;
    pollIntervalMs = network.commandPollIntervalMs;
    uploadIntervalMs = network.dataUploadIntervalMs;
    
    if (serverURL.isEmpty()) {
        Logger::error("APIClient", "Server URL not configured");
        return false;
    }
    
    Logger::info("APIClient", "Server: " + serverURL);
    Logger::info("APIClient", "Poll interval: " + String(pollIntervalMs) + "ms");
    Logger::info("APIClient", "Upload interval: " + String(uploadIntervalMs) + "ms");
    
    initialized = true;
    return true;
}

void APIClient::shutdown() {
    http.end();
    initialized = false;
    Logger::info("APIClient", "API client shutdown");
}

bool APIClient::pollCommands() {
    if (!initialized || !NetworkUtils::isConnected()) {
        return false;
    }
    
    unsigned long currentTime = millis();
    if (currentTime - lastPollTime < pollIntervalMs) {
        return true; // Not time to poll yet
    }
    
    Logger::debug("APIClient", "Polling for commands...");
    
    String response;
    bool success = get("/api/v1/commands", &response);
    
    if (success && !response.isEmpty()) {
        processCommands(response);
        lastPollTime = currentTime;
        Logger::debug("APIClient", "Command poll successful");
    } else {
        Logger::warn("APIClient", "Command poll failed");
    }
    
    return success;
}

bool APIClient::uploadSensorData() {
    if (!initialized || !NetworkUtils::isConnected()) {
        return false;
    }
    
    unsigned long currentTime = millis();
    if (currentTime - lastUploadTime < uploadIntervalMs) {
        return true; // Not time to upload yet
    }
    
    Logger::debug("APIClient", "Uploading sensor data...");
    
    String payload = createSensorPayload();
    String response;
    bool success = post("/api/v1/sensor-data", payload, &response);
    
    if (success) {
        lastUploadTime = currentTime;
        Logger::debug("APIClient", "Sensor data upload successful");
    } else {
        Logger::warn("APIClient", "Sensor data upload failed");
    }
    
    return success;
}

bool APIClient::uploadStatus() {
    if (!initialized || !NetworkUtils::isConnected()) {
        return false;
    }
    
    Logger::debug("APIClient", "Uploading status...");
    
    String payload = createStatusPayload();
    String response;
    bool success = post("/api/v1/status", payload, &response);
    
    if (success) {
        Logger::debug("APIClient", "Status upload successful");
    } else {
        Logger::warn("APIClient", "Status upload failed");
    }
    
    return success;
}

bool APIClient::makeRequest(const String& endpoint, const String& method, const String& payload, String* response) {
    if (!NetworkUtils::isConnected()) {
        Logger::error("APIClient", "No network connection");
        return false;
    }
    
    String url = serverURL + endpoint;
    Logger::debug("APIClient", method + " " + url);
    
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("User-Agent", "AeroEnv-ESP32/1.0");
    
    if (!deviceToken.isEmpty()) {
        http.addHeader("Authorization", "Bearer " + deviceToken);
    }
    
    int httpCode = -1;
    
    if (method == "GET") {
        httpCode = http.GET();
    } else if (method == "POST") {
        httpCode = http.POST(payload);
    }
    
    bool success = (httpCode >= 200 && httpCode < 300);
    
    if (success && response) {
        *response = http.getString();
    }
    
    if (!success) {
        Logger::error("APIClient", "HTTP " + String(httpCode) + " for " + endpoint);
    }
    
    http.end();
    return success;
}

bool APIClient::get(const String& endpoint, String* response) {
    return makeRequest(endpoint, "GET", "", response);
}

bool APIClient::post(const String& endpoint, const String& payload, String* response) {
    return makeRequest(endpoint, "POST", payload, response);
}

String APIClient::createSensorPayload() {
    JsonDocument doc;
    doc["device_id"] = WiFi.macAddress();
    doc["timestamp"] = millis();
    doc["device_type"] = "environmental";
    
    // TODO: Get actual sensor readings from SensorManager
    JsonObject sensors = doc["sensors"].to<JsonObject>();
    sensors["temperature"] = 0.0;
    sensors["humidity"] = 0.0;
    sensors["pressure"] = 0.0;
    
    String payload;
    serializeJson(doc, payload);
    return payload;
}

String APIClient::createStatusPayload() {
    JsonDocument doc;
    doc["device_id"] = WiFi.macAddress();
    doc["timestamp"] = millis();
    doc["uptime"] = millis();
    doc["free_heap"] = ESP.getFreeHeap();
    doc["wifi_rssi"] = NetworkUtils::getRSSI();
    doc["wifi_ip"] = NetworkUtils::getLocalIP();
    
    String payload;
    serializeJson(doc, payload);
    return payload;
}

void APIClient::processCommands(const String& response) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    
    if (error) {
        Logger::error("APIClient", "Failed to parse commands: " + String(error.c_str()));
        return;
    }
    
    JsonArray commands = doc["commands"];
    for (JsonObject command : commands) {
        processCommand(command);
    }
}

void APIClient::processCommand(const JsonObject& command) {
    String commandType = command["type"];
    String commandAction = command["action"];
    
    Logger::info("APIClient", "Processing command: " + commandType + "." + commandAction);
    
    // Publish command event for other components to handle
    String commandData;
    serializeJson(command, commandData);
    eventBus.publish(EventTypes::COMMAND_RECEIVED, "APIClient", commandData);
}