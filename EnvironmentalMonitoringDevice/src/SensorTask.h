#ifndef SENSOR_TASK_H
#define SENSOR_TASK_H

#include <Arduino.h>
#include <vector>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Interfaces.h"

class SensorTask {
private:
    std::vector<ISensor*> sensors;
    TaskHandle_t taskHandle;
    bool running;
    
    static void taskWrapper(void* parameter);
    void task();
    
public:
    SensorTask();
    ~SensorTask();
    
    void addSensor(ISensor* sensor);
    void begin();
    void stop();
    bool isRunning() const;
};

extern SensorTask sensorTask;

#endif