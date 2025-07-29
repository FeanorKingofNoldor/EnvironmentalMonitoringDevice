// File: src/core/Managers.h
#ifndef CORE_MANAGERS_H
#define CORE_MANAGERS_H

#include "BaseClasses.h"
#include "Config.h"
#include "EventBus.h"
#include <vector>
#include <memory>

// Core sensor manager - device agnostic
class SensorManager : public BaseManager {
private:
    std::vector<std::unique_ptr<BaseSensor>> sensors;
    std::vector<SensorReading> lastReadings;
    const IDeviceCapabilities* deviceCapabilities;
    unsigned long lastReadTime;
    unsigned long readIntervalMs;
    
    // Task handle for sensor reading task
    TaskHandle_t sensorTaskHandle;
    static void sensorTask(void* parameter);
    
    void subscribeToEvents();
    std::unique_ptr<BaseSensor> createSensor(const SensorConfig& config);
    
public:
    SensorManager() : BaseManager("SensorManager"), 
                     deviceCapabilities(nullptr), lastReadTime(0), 
                     readIntervalMs(1000), sensorTaskHandle(nullptr) {}
    
    ~SensorManager();
    
    // BaseManager implementation
    bool begin() override;
    void shutdown() override;
    void update() override;
    
    // Device capabilities injection
    void setDeviceCapabilities(const IDeviceCapabilities* capabilities) {
        deviceCapabilities = capabilities;
    }
    
    // Sensor management
    bool addSensor(const SensorConfig& config);
    bool removeSensor(const String& name);
    bool reconfigureSensor(const String& name, const SensorConfig& config);
    
    // Reading access
    SensorReading getReading(const String& sensorName) const;
    std::vector<SensorReading> getAllReadings() const;
    std::vector<SensorReading> getReadingsByType(const String& type) const;
    
    // Status
    bool areAllSensorsReady() const;
    size_t getSensorCount() const { return sensors.size(); }
    std::vector<String> getSensorNames() const;
    
    // Control
    bool readAllSensors();
    void setReadInterval(unsigned long intervalMs) { readIntervalMs = intervalMs; }
    unsigned long getReadInterval() const { return readIntervalMs; }
    
    // Diagnostics
    void printSensorStatus() const;
    void printSensorDiagnostics(const String& sensorName) const;
};

// Core actuator manager - device agnostic
class ActuatorManager : public BaseManager {
private:
    std::vector<std::unique_ptr<BaseActuator>> actuators;
    const IDeviceCapabilities* deviceCapabilities;
    
    void subscribeToEvents();
    std::unique_ptr<BaseActuator> createActuator(const ActuatorConfig& config);
    
public:
    ActuatorManager() : BaseManager("ActuatorManager"), deviceCapabilities(nullptr) {}
    ~ActuatorManager();
    
    // BaseManager implementation
    bool begin() override;
    void shutdown() override;
    void update() override;
    
    // Device capabilities injection
    void setDeviceCapabilities(const IDeviceCapabilities* capabilities) {
        deviceCapabilities = capabilities;
    }
    
    // Actuator management
    bool addActuator(const ActuatorConfig& config);
    bool removeActuator(const String& name);
    bool reconfigureActuator(const String& name, const ActuatorConfig& config);
    
    // Actuator control
    bool activateActuator(const String& name);
    bool deactivateActuator(const String& name);
    bool isActuatorActive(const String& name) const;
    
    // Status
    bool areAllActuatorsReady() const;
    size_t getActuatorCount() const { return actuators.size(); }
    std::vector<String> getActuatorNames() const;
    std::vector<String> getActiveActuatorNames() const;
    
    // Diagnostics
    void printActuatorStatus() const;
    void printActuatorDiagnostics(const String& actuatorName) const;
    
    // Emergency shutdown
    void emergencyStopAll();
};

// System monitor for health checking and diagnostics
class SystemMonitor : public BaseManager {
private:
    unsigned long lastHealthCheck;
    unsigned long healthCheckInterval;
    
    // System metrics
    struct SystemMetrics {
        uint32_t freeHeap;
        uint32_t totalHeap;
        uint32_t minFreeHeap;
        float cpuUsage;
        unsigned long uptime;
        int wifiRSSI;
        String wifiIP;
        unsigned long lastUpdateTime;
        
        SystemMetrics() : freeHeap(0), totalHeap(0), minFreeHeap(0),
                         cpuUsage(0.0), uptime(0), wifiRSSI(0),
                         wifiIP(""), lastUpdateTime(0) {}
    } currentMetrics;
    
    void collectMetrics();
    void checkSystemHealth();
    void publishMetrics();
    
public:
    SystemMonitor() : BaseManager("SystemMonitor"), 
                     lastHealthCheck(0), healthCheckInterval(5000) {}
    
    // BaseManager implementation
    bool begin() override;
    void shutdown() override;
    void update() override;
    
    // Configuration
    void setHealthCheckInterval(unsigned long intervalMs) {
        healthCheckInterval = intervalMs;
    }
    
    // Metrics access
    const SystemMetrics& getMetrics() const { return currentMetrics; }
    
    // Health checks
    bool isSystemHealthy() const;
    String getHealthStatus() const;
    
    // Diagnostics
    void printSystemStatus() const;
    void printDetailedDiagnostics() const;
};

// Device coordinator - orchestrates all managers
class DeviceCoordinator : public BaseManager {
private:
    const IDeviceCapabilities* deviceCapabilities;
    
    SensorManager* sensorManager;
    ActuatorManager* actuatorManager;
    SystemMonitor* systemMonitor;
    
    // Startup sequence
    bool initializeCore();
    bool initializeDevice();
    bool initializeManagers();
    bool startTasks();
    
    // Event handlers
    void handleSystemEvents();
    void handleConfigChanges();
    void handleEmergencyStop();
    
public:
    DeviceCoordinator() : BaseManager("DeviceCoordinator"),
                         deviceCapabilities(nullptr),
                         sensorManager(nullptr),
                         actuatorManager(nullptr),
                         systemMonitor(nullptr) {}
    
    ~DeviceCoordinator();
    
    // BaseManager implementation
    bool begin() override;
    void shutdown() override;
    void update() override;
    
    // Device capabilities injection
    void setDeviceCapabilities(const IDeviceCapabilities* capabilities) {
        deviceCapabilities = capabilities;
    }
    
    // Manager access
    SensorManager* getSensorManager() const { return sensorManager; }
    ActuatorManager* getActuatorManager() const { return actuatorManager; }
    SystemMonitor* getSystemMonitor() const { return systemMonitor; }
    
    // System control
    bool startSystem();
    void stopSystem();
    void restartSystem();
    
    // Status
    bool isSystemReady() const;
    String getSystemStatus() const;
    
    // Diagnostics
    void printSystemOverview() const;
};

// Global instances
extern SensorManager sensorManager;
extern ActuatorManager actuatorManager;
extern SystemMonitor systemMonitor;
extern DeviceCoordinator deviceCoordinator;

#endif // CORE_MANAGERS_H