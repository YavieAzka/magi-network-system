#ifndef LINK_HPP
#define LINK_HPP

#include <vector>
#include <cstdint>
#include <cstddef>
using namespace std;

class Interface;

class Link
{
private:
    Interface *interfaceA;
    Interface *interfaceB;
    int delayMs;
    bool active = true;
    size_t transmittedBytes = 0;

public:
    Link(Interface *interfaceA, Interface *interfaceB, int delayMs = 0);
    ~Link();

    Interface *getInterfaceA() const;
    Interface *getInterfaceB() const;
    Interface *getOtherInterface(Interface *sender) const;

    bool isActive() const;
    void setActive(bool value);

    int getDelayMs() const;
    void setDelayMs(int delay);

    size_t getTransmittedBytes() const;

    void transmit(Interface *sender, const vector<uint8_t> &rawBytes);
};

#endif