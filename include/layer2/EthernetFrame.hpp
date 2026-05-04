#ifndef ETHERNET_FRAME_HPP
#define ETHERNET_FRAME_HPP

#include "../core/Packet.hpp"
#include <cstdint>
#include <string>
#include <vector>
// Struktur Ethernet Frame:
// | Destination MAC | Source MAC | EtherType |    Payload   | FCS
// | (6 byte)        | (6 byte)   | (2 byte)  | (46-1500 B)  | (4 byte)|
class EthernetFrame : public Packet
{
private:
    static uint32_t crc32(const std::vector<uint8_t> &data);
    static std::vector<uint8_t> macToBytes(const std::string &mac);
    static std::string bytesToMac(const std::vector<uint8_t> &bytes, size_t offset);

public:
    std::string destMac; // "AA:BB:CC:DD:EE:FF"
    std::string srcMac;
    uint16_t etherType;
    int vlanId;// untagged = -1
    uint32_t fcs;

    EthernetFrame();
    EthernetFrame(const std::string &destMac, const std::string &srcMac,
                  uint16_t etherType, const std::vector<uint8_t> &payload,
                  int vlanId = -1);

    std::string getType() const override;
    bool isBroadcast() const;
    bool isTagged() const;
    std::string toString() const;

    std::vector<uint8_t> toBytes() const override;
    void deserialize(const std::vector<uint8_t> &raw) override;
    void calculateAndSetFcs();
    bool verifyFcs() const;
};

#endif