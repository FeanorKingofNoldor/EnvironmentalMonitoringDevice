#include "Logger.h"
#include <esp_system.h>

LogLevel Logger::currentLevel = LogLevel::INFO;
unsigned long Logger::bootTime = 0;

void Logger::init() {
    bootTime = millis();
    
#ifdef CORE_DEBUG_LEVEL
    if (CORE_DEBUG_LEVEL >= 5) {
        currentLevel = LogLevel::DEBUG;
    } else if (CORE_DEBUG_LEVEL >= 3) {
        currentLevel = LogLevel::INFO;
    } else if (CORE_DEBUG_LEVEL >= 1) {
        currentLevel = LogLevel::WARN;
    } else {
        currentLevel = LogLevel::ERROR;
    }
#endif
    
    Serial.println("\n=== AeroEnv Environmental Controller ===");
    printSystemInfo();
}

void Logger::setLevel(LogLevel level) {
    currentLevel = level;
}

void Logger::log(LogLevel level, const String& component, const String& message) {
    if (level <= currentLevel) {
        Serial.println(formatMessage(level, component, message));
    }
}

void Logger::error(const String& component, const String& message) {
    log(LogLevel::ERROR, component, message);
}

void Logger::warn(const String& component, const String& message) {
    log(LogLevel::WARN, component, message);
}

void Logger::info(const String& component, const String& message) {
    log(LogLevel::INFO, component, message);
}

void Logger::debug(const String& component, const String& message) {
    log(LogLevel::DEBUG, component, message);
}

String Logger::formatMessage(LogLevel level, const String& component, const String& message) {
    return String("[") + getTimestamp() + "] " + 
           getLevelString(level) + " " + 
           component + ": " + message;
}

String Logger::getLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::ERROR: return "[ERROR]";
        case LogLevel::WARN:  return "[WARN ]";
        case LogLevel::INFO:  return "[INFO ]";
        case LogLevel::DEBUG: return "[DEBUG]";
        default: return "[?????]";
    }
}

String Logger::getTimestamp() {
    unsigned long uptime = millis() - bootTime;
    unsigned long seconds = uptime / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    
    return String(hours) + ":" + 
           String(minutes % 60).substring(0, 2) + ":" + 
           String(seconds % 60).substring(0, 2);
}

void Logger::printSystemInfo() {
    Serial.println("System Information:");
    Serial.printf("  Chip: %s\n", ESP.getChipModel());
    Serial.printf("  CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("  Flash Size: %d MB\n", ESP.getFlashChipSize() / (1024 * 1024));
    Serial.printf("  Free Heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("  SDK Version: %s\n", ESP.getSdkVersion());
}

void Logger::printMemoryInfo() {
    Serial.printf("Memory: Free Heap: %d, Min Free: %d, Largest Block: %d\n",
                 ESP.getFreeHeap(), 
                 ESP.getMinFreeHeap(),
                 ESP.getMaxAllocHeap());
}