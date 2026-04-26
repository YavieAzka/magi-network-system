#include "core/Interface.hpp"
#include "core/Node.hpp"
#include "core/Link.hpp"

#include <iostream>
#include <stdexcept>

Interface::Interface(Node *owner, int portNumber, const string &macAddress)
    : owner(owner),
      portNumber(portNumber),
      macAddress(macAddress),
      link(nullptr),
      up(true),
      mtu(1500),
      sentBytes(0),
      receivedBytes(0)
{
    if (owner == nullptr)
    {
        throw invalid_argument("Interface owner cannot be null.");
    }

    if (portNumber <= 0)
    {
        throw invalid_argument("Port number must be positive.");
    }

    if (macAddress.empty())
    {
        throw invalid_argument("MAC address cannot be empty.");
    }
}

Node *Interface::getOwner() const
{
    return owner;
}

int Interface::getPortNumber() const
{
    return portNumber;
}

string Interface::getMacAddress() const
{
    return macAddress;
}

bool Interface::isConnected() const
{
    return link != nullptr;
}

void Interface::attachLink(Link *newLink)
{
    if (newLink == nullptr)
    {
        throw invalid_argument("Cannot attach null link.");
    }

    if (link != nullptr)
    {
        throw runtime_error(
            "Interface " + owner->getName() + ":" +
            to_string(portNumber) + " is already connected.");
    }

    link = newLink;
}

void Interface::detachLink()
{
    link = nullptr;
}

bool Interface::isUp() const
{
    return up;
}

void Interface::setUp(bool value)
{
    up = value;
}

int Interface::getMtu() const
{
    return mtu;
}

void Interface::setMtu(int newMtu)
{
    if (newMtu <= 0)
    {
        throw invalid_argument("MTU must be positive.");
    }

    mtu = newMtu;
}

size_t Interface::getSentBytes() const
{
    return sentBytes;
}

size_t Interface::getReceivedBytes() const
{
    return receivedBytes;
}

void Interface::send(const vector<uint8_t> &rawBytes)
{
    if (!up)
    {
        cout << "[" << owner->getName() << ":" << portNumber
             << "] send failed: interface is down."
             << endl;
        return;
    }

    if (link == nullptr)
    {
        cout << "[" << owner->getName() << ":" << portNumber
             << "] send failed: interface is not connected."
             << endl;
        return;
    }

    if (rawBytes.size() > static_cast<size_t>(mtu))
    {
        cout << "[" << owner->getName() << ":" << portNumber
             << "] send failed: packet size "
             << rawBytes.size()
             << " exceeds MTU "
             << mtu
             << "."
             << endl;
        return;
    }

    sentBytes += rawBytes.size();

    cout << "[" << owner->getName() << ":" << portNumber
         << "] sending "
         << rawBytes.size()
         << " bytes."
         << endl;

    link->transmit(this, rawBytes);
}

void Interface::receive(const vector<uint8_t> &rawBytes)
{
    if (!up)
    {
        cout << "[" << owner->getName() << ":" << portNumber
             << "] dropped packet: interface is down."
             << endl;
        return;
    }

    receivedBytes += rawBytes.size();

    owner->handleReceive(this, rawBytes);
}