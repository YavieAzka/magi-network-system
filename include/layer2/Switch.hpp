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
using namespace std;

class Switch: public Node{

    public:
        enum PortType{
            ACCESS,
            TRUNK,
        };

        struct PortConfig{
            PortType type;
            uint16_t pvid;
            vector<int> allowedVlans; // Buat trunk

            PortConfig(): type(PortType::ACCESS), pvid(1){};
            PortConfig(PortType t, int v, vector<int> allowed = {}): type(t), pvid(v), allowedVlans(allowed){};
        };

        struct MACEntry{
            int port;
            chrono::steady_clock::time_point lastSeen; // untuk aging
        };

        
        Switch(const string& name, int numPorts = 24);
        void configurePort(int port, PortType type, uint16_t pvid, vector<int> allowedVlans = {});
        void learnMAC(uint16_t vlanId, const string& mac, int port);
        int lookupMAC(uint16_t vlanId, const string& mac);
        void printMACTable() const;
        void handleReceive(int ingressPort, const vector<uint8_t>& rawBytes);
        string getType() const override;

    private:
        int numPorts;
        map<int, PortConfig> portConfigs;
        map<pair<string, uint16_t>, MACEntry> macTable;

        void forwardFrame(int port, EthernetFrame& frame, uint16_t ingressVlan);
        void flood(uint16_t ingressVlan, EthernetFrame& frame, int excludePort);
        bool isVlanAllowed(int port, uint16_t vlanId) const;
        bool isValidPort(int port) const;
};

#endif