#ifndef ETHERNET_FRAME_HPP
#define ETHERNET_FRAME_HPP

#include "../core/Packet.hpp"
#include <cstdint>
#include <string>
#include <vector>
using namespace std;

// Struktur Ethernet Frame:
// | Destination MAC | Source MAC | EtherType |    Payload   | FCS
// | (6 byte)        | (6 byte)   | (2 byte)  | (46-1500 B)  | (4 byte)|
class EthernetFrame : public Packet
{
private:
    // Helper functions
    static uint32_t crc32(const vector<uint8_t> &data);
    static vector<uint8_t> macToBytes(const string &mac);
    static string bytesToMac(const vector<uint8_t> &bytes, size_t offset);

public:
    string destMac; // "AA:BB:CC:DD:EE:FF"
    string srcMac;
    uint16_t etherType;
    int vlanId; // untagged = -1
    vector<uint8_t> payload;
    uint32_t fcs = 0;

    EthernetFrame();
    EthernetFrame(
        const string &destMac,
        const string &srcMac,
        uint16_t etherType,
        const vector<uint8_t> &payload,
        int vlanId = -1);

    string getType() const override;
    bool isBroadcast() const;
    bool isTagged() const;
    string toString() const;

    vector<uint8_t> toBytes() const override;
    void deserialize(const vector<uint8_t> &raw) override;
    void calculateAndSetFcs();
    bool verifyFcs() const;
};

#endif
