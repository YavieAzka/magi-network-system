#include "Link.hpp"
#include "Interface.hpp"
#include "Node.hpp"

#include <iostream>
#include <thread>
#include <chrono>
#include <stdexcept>

Link::Link(Interface *interfaceA, Interface *interfaceB, int delayMs)
    : interfaceA(interfaceA),
      interfaceB(interfaceB),
      delayMs(delayMs),
      active(true),
      transmittedBytes(0)
{
    if (interfaceA == nullptr || interfaceB == nullptr)
    {
        throw invalid_argument("Cannot create link with null interface.");
    }

    if (interfaceA == interfaceB)
    {
        throw invalid_argument("Cannot create link using the same interface.");
    }

    if (delayMs < 0)
    {
        throw invalid_argument("Link delay cannot be negative.");
    }

    if (interfaceA->isConnected() || interfaceB->isConnected())
    {
        throw runtime_error("Cannot create link: one or both interfaces are already connected.");
    }

    interfaceA->attachLink(this);
    interfaceB->attachLink(this);
}

Link::~Link()
{
    if (interfaceA != nullptr)
    {
        interfaceA->detachLink();
    }

    if (interfaceB != nullptr)
    {
        interfaceB->detachLink();
    }
}

Interface *Link::getInterfaceA() const
{
    return interfaceA;
}

Interface *Link::getInterfaceB() const
{
    return interfaceB;
}

Interface *Link::getOtherInterface(Interface *sender) const
{
    if (sender == interfaceA)
    {
        return interfaceB;
    }

    if (sender == interfaceB)
    {
        return interfaceA;
    }

    throw runtime_error("Sender interface is not part of this link.");
}

bool Link::isActive() const
{
    return active;
}

void Link::setActive(bool value)
{
    active = value;
}

int Link::getDelayMs() const
{
    return delayMs;
}

void Link::setDelayMs(int delay)
{
    if (delay < 0)
    {
        throw invalid_argument("Link delay cannot be negative.");
    }

    delayMs = delay;
}

size_t Link::getTransmittedBytes() const
{
    return transmittedBytes;
}

void Link::transmit(Interface *sender, const vector<uint8_t> &rawBytes)
{
    if (!active)
    {
        cout << "[Link] dropped packet: link is inactive." << endl;
        return;
    }

    Interface *receiver = getOtherInterface(sender);

    transmittedBytes += rawBytes.size();

    cout << "[Link] transmitting "
         << rawBytes.size()
         << " bytes from "
         << sender->getOwner()->getName()
         << ":"
         << sender->getPortNumber()
         << " to "
         << receiver->getOwner()->getName()
         << ":"
         << receiver->getPortNumber()
         << " | delay="
         << delayMs
         << "ms"
         << endl;

    if (delayMs > 0)
    {
        this_thread::sleep_for(chrono::milliseconds(delayMs));
    }

    receiver->receive(rawBytes);
}