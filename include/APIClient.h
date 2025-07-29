#ifndef API_CLIENT_H
#define API_CLIENT_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "EventBus.h"
#include "Config.h"

class APIClient {
private:
    String serverURL;
    String deviceToken;
    HTTPClient http;
    bool initialized;
    unsigned long lastPollTime;
    unsigned long lastUploadTime;
    int pollIntervalMs;
    int uploadIntervalMs;
    
    // HTTP methods
    bool makeRequest(const String& endpoint, const String& method, const String& payload = "", String* response = nullptr);
    bool get(const String& endpoint, String* response = nullptr);
    bool post(const String& endpoint, const String& payload, String* response = nullptr);
    
    // Data formatting
    String createSensorPayload();
    String createStatusPayload();
    
    // Response processing
    void processCommands(const String& response);
    void processCommand(const JsonObject& command);
    
public:
    APIClient();
    ~APIClient();
    
    bool begin();
    void shutdown();
    
    // Communication methods
    bool pollCommands();
    bool uploadSensorData();
    bool uploadStatus();
    
    // Configuration
    void setServerURL(const String& url) { serverURL = url; }
    void setDeviceToken(const String& token) { deviceToken = token; }
    void setPollInterval(int intervalMs) { pollIntervalMs = intervalMs; }
    void setUploadInterval(int intervalMs) { uploadIntervalMs = intervalMs; }
    
    // Status
    bool isInitialized() const { return initialized; }
    String getServerURL() const { return serverURL; }
    unsigned long getLastPollTime() const { return lastPollTime; }
    unsigned long getLastUploadTime() const { return lastUploadTime; }
};

#endif // API_CLIENT_H