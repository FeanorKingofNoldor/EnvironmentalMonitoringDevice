#ifndef INTERFACES_H
#define INTERFACES_H

#include <Arduino.h>

class IComponent {
public:
    virtual ~IComponent() = default;
    virtual bool begin() = 0;
    virtual String getName() const = 0;
};

class ISensor : public IComponent {
public:
    virtual ~ISensor() = default;
    virtual void read() = 0;
    virtual bool isConnected() const = 0;
};

class IActuator : public IComponent {
public:
    virtual ~IActuator() = default;
    virtual void setState(bool state) = 0;
    virtual bool getState() const = 0;
};

#endif