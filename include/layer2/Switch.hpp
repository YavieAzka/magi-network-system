#ifndef SWITCH_HPP
#define SWITCH_HPP

#include "../core/Node.hpp"
#include "../core/Interface.hpp"
#include "EthernetFrame.hpp"
#include <chrono>
#include <map>
#include <iostream>
#include <vector>
#include <algorithm>
#include <iomanip>

class Switch : public Node
{
public:
    enum PortType
    {
        ACCESS,
        TRUNK,
    };

    struct PortConfig
    {
        PortType type;
        uint16_t pvid;
        std::vector<int> allowedVlans;

        PortConfig() : type(ACCESS), pvid(1) {}
        PortConfig(PortType t, int v, std::vector<int> allowed = {})
            : type(t), pvid(v), allowedVlans(allowed) {}
    };

    struct MACEntry
    {
        int port;
        std::chrono::steady_clock::time_point lastSeen;
    };

    Switch(const std::string &name, int numPorts = 24);
    void configurePort(int port, PortType type, uint16_t pvid, std::vector<int> allowedVlans = {});
    void learnMAC(uint16_t vlanId, const std::string &mac, int port);
    int lookupMAC(uint16_t vlanId, const std::string &mac);
    void printMACTable() const;
    void handleReceive(Interface *inInterface, const std::vector<uint8_t> &rawBytes) override;
    std::string getType() const override;

private:
    int numPorts;
    std::map<int, PortConfig> portConfigs;
    std::map<std::pair<std::string, uint16_t>, MACEntry> macTable;

    void forwardFrame(int port, EthernetFrame &frame, uint16_t ingressVlan);
    void flood(uint16_t ingressVlan, EthernetFrame &frame, int excludePort);
    bool isVlanAllowed(int port, uint16_t vlanId) const;
    bool isValidPort(int port) const;

    static std::string normalizeMac(const std::string &mac);
};

#endif