#include "NetworkUtils.h"
#include "Config.h"
#include "Logger.h"

bool NetworkUtils::wifiConnected = false;
String NetworkUtils::lastSSID = "";
unsigned long NetworkUtils::lastConnectionAttempt = 0;
int NetworkUtils::connectionAttempts = 0;

bool NetworkUtils::connectWiFi() {
    NetworkConfig network = config.getNetwork();
    
    if (network.wifiSSID.isEmpty()) {
        Logger::warn("NetworkUtils", "WiFi SSID not configured");
        return false;
    }
    
    return connectWiFi(network.wifiSSID, network.wifiPassword);
}

bool NetworkUtils::connectWiFi(const String& ssid, const String& password) {
    Logger::info("NetworkUtils", "Connecting to WiFi: " + ssid);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    lastSSID = ssid;
    lastConnectionAttempt = millis();
    connectionAttempts = 0;
    
    bool connected = waitForConnection();
    
    if (connected) {
        wifiConnected = true;
        Logger::info("NetworkUtils", "WiFi connected successfully");
        printNetworkInfo();
    } else {
        wifiConnected = false;
        Logger::error("NetworkUtils", "WiFi connection failed");
    }
    
    return connected;
}

bool NetworkUtils::waitForConnection(unsigned long timeoutMs) {
    unsigned long startTime = millis();
    
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < timeoutMs) {
        delay(500);
        connectionAttempts++;
        
        if (connectionAttempts % 10 == 0) {
            Logger::info("NetworkUtils", "Still connecting... (" + String(connectionAttempts/2) + "s)");
        }
        
        if (connectionAttempts > MAX_CONNECTION_ATTEMPTS) {
            Logger::error("NetworkUtils", "Max connection attempts reached");
            return false;
        }
    }
    
    return WiFi.status() == WL_CONNECTED;
}

bool NetworkUtils::isConnected() {
    wifiConnected = (WiFi.status() == WL_CONNECTED);
    return wifiConnected;
}

void NetworkUtils::disconnect() {
    WiFi.disconnect();
    wifiConnected = false;
    Logger::info("NetworkUtils", "WiFi disconnected");
}

String NetworkUtils::getLocalIP() {
    if (isConnected()) {
        return WiFi.localIP().toString();
    }
    return "0.0.0.0";
}

String NetworkUtils::getMacAddress() {
    return WiFi.macAddress();
}

int NetworkUtils::getRSSI() {
    if (isConnected()) {
        return WiFi.RSSI();
    }
    return -100; // No signal
}

void NetworkUtils::printNetworkInfo() {
    if (!isConnected()) {
        Logger::warn("NetworkUtils", "Not connected to WiFi");
        return;
    }
    
    Logger::info("NetworkUtils", "Network Information:");
    Logger::info("NetworkUtils", "  SSID: " + WiFi.SSID());
    Logger::info("NetworkUtils", "  IP: " + getLocalIP());
    Logger::info("NetworkUtils", "  MAC: " + getMacAddress());
    Logger::info("NetworkUtils", "  RSSI: " + String(getRSSI()) + " dBm");
    Logger::info("NetworkUtils", "  Gateway: " + WiFi.gatewayIP().toString());
    Logger::info("NetworkUtils", "  DNS: " + WiFi.dnsIP().toString());
}

void NetworkUtils::handleReconnection() {
    if (!isConnected() && !lastSSID.isEmpty()) {
        unsigned long timeSinceLastAttempt = millis() - lastConnectionAttempt;
        
        if (timeSinceLastAttempt > 30000) { // Try reconnecting every 30 seconds
            Logger::info("NetworkUtils", "Attempting WiFi reconnection...");
            connectWiFi();
        }
    }
}