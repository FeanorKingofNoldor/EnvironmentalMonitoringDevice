#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

enum class LogLevel {
    ERROR = 0,
    WARN = 1,
    INFO = 2,
    DEBUG = 3
};

class Logger {
private:
    static LogLevel currentLevel;
    static unsigned long bootTime;
    
    static String formatMessage(LogLevel level, const String& component, const String& message);
    static String getLevelString(LogLevel level);
    static String getTimestamp();
    
public:
    static void setLevel(LogLevel level);
    static void init();
    
    static void error(const String& component, const String& message);
    static void warn(const String& component, const String& message);
    static void info(const String& component, const String& message);
    static void debug(const String& component, const String& message);
    
    // Convenience methods
    static void log(LogLevel level, const String& component, const String& message);
    
    // System status logging
    static void printSystemInfo();
    static void printMemoryInfo();
};

#endif // LOGGER_H