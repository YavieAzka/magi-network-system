#ifndef INTERFACE_HPP
#define INTERFACE_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>
using namespace std;

class Node;
class Link;

class Interface
{
private:
    Node *owner;
    int portNumber;
    string macAddress;
    Link *link;

    bool up = true;
    int mtu = 1500;
    size_t sentBytes = 0;
    size_t receivedBytes = 0;

public:
    Interface(Node *owner, int portNumber, const string &macAddress);

    Node *getOwner() const;
    int getPortNumber() const;
    string getMacAddress() const;

    bool isConnected() const;
    void attachLink(Link *newLink);
    void detachLink();

    bool isUp() const;
    void setUp(bool value);

    int getMtu() const;
    void setMtu(int mtu);

    size_t getSentBytes() const;
    size_t getReceivedBytes() const;

    void send(const vector<uint8_t> &rawBytes);
    void receive(const vector<uint8_t> &rawBytes);
};

#endif