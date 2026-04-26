#include "core/Node.hpp"
#include "core/Interface.hpp"

#include <iostream>
#include <iomanip>
#include <stdexcept>
using namespace std;

Node::Node(const string &name)
    : name(name), active(true)
{
    if (name.empty())
    {
        throw invalid_argument("Node name cannot be empty.");
    }
}

Node::~Node() = default;

string Node::getName() const
{
    return name;
}

bool Node::isActive() const
{
    return active;
}

void Node::setActive(bool value)
{
    active = value;
}

Interface *Node::addInterface(int portNumber, const string &macAddress)
{
    if (portNumber <= 0)
    {
        throw invalid_argument("Port number must be positive.");
    }

    if (interfaces.count(portNumber) > 0)
    {
        throw runtime_error(
            "Interface on port " + to_string(portNumber) +
            " already exists in node " + name + ".");
    }

    interfaces[portNumber] = make_unique<Interface>(this, portNumber, macAddress);

    return interfaces[portNumber].get();
}

Interface *Node::getInterface(int portNumber) const
{
    auto it = interfaces.find(portNumber);

    if (it == interfaces.end())
    {
        return nullptr;
    }

    return it->second.get();
}

void Node::removeInterface(int portNumber)
{
    auto it = interfaces.find(portNumber);

    if (it == interfaces.end())
    {
        throw runtime_error(
            "Interface on port " + to_string(portNumber) +
            " does not exist in node " + name + ".");
    }

    if (it->second->isConnected())
    {
        throw runtime_error(
            "Cannot remove interface on port " + to_string(portNumber) +
            " because it is still connected to a link.");
    }

    interfaces.erase(it);
}

string Node::getType() const
{
    return "node";
}

void Node::handleReceive(Interface *inInterface, const vector<uint8_t> &rawBytes)
{
    if (!active)
    {
        cout << "[" << name << "] dropped data: node is inactive." << endl;
        return;
    }

    cout << "[" << name << "] received "
         << rawBytes.size()
         << " bytes on port "
         << inInterface->getPortNumber()
         << endl;

    cout << "[" << name << "] raw bytes: ";

    for (uint8_t byte : rawBytes)
    {
        cout << hex << uppercase << setw(2) << setfill('0')
             << static_cast<int>(byte) << " ";
    }

    cout << dec << endl;
}