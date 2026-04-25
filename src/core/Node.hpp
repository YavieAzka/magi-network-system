#ifndef NODE_HPP
#define NODE_HPP

#include <string>
#include <map>
#include <memory>
#include <vector>
#include <cstdint>
using namespace std;

class Interface;

class Node
{
private:
    string name;
    string type;
    bool active = true;
    map<int, unique_ptr<Interface>> interfaces;

public:
    Node(const string &name, const string &type);
    virtual ~Node();

    string getName() const;
    string getType() const;

    bool isActive() const;
    void setActive(bool value);

    Interface *addInterface(int portNumber, const string &macAddress);
    Interface *getInterface(int portNumber) const;
    void removeInterface(int portNumber);

    virtual void handleReceive(Interface *inInterface, const vector<uint8_t> &rawBytes);
};

#endif