#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H

#include <Arduino.h>
#include <WiFi.h>

class NetworkUtils {
private:
    static bool wifiConnected;
    static String lastSSID;
    static unsigned long lastConnectionAttempt;
    static int connectionAttempts;
    
    static const int MAX_CONNECTION_ATTEMPTS = 20;
    static const unsigned long CONNECTION_TIMEOUT = 30000; // 30 seconds
    
public:
    static bool connectWiFi();
    static bool connectWiFi(const String& ssid, const String& password);
    static bool isConnected();
    static void disconnect();
    static String getLocalIP();
    static String getMacAddress();
    static int getRSSI();
    static void printNetworkInfo();
    static bool waitForConnection(unsigned long timeoutMs = CONNECTION_TIMEOUT);
    static void handleReconnection();
};

#endif // NETWORK_UTILS_H