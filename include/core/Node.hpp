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
    bool active = true;
    map<int, unique_ptr<Interface>> interfaces;

public:
    explicit Node(const string &name);
    virtual ~Node();

    string getName() const;

    bool isActive() const;
    void setActive(bool value);

    Interface *addInterface(int portNumber, const string &macAddress);
    Interface *getInterface(int portNumber) const;
    void removeInterface(int portNumber);

    virtual string getType() const;

    virtual void handleReceive(Interface *inInterface, const vector<uint8_t> &rawBytes);
};

#endif