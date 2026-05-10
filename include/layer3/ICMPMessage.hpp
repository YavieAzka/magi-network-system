#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include "core/Packet.hpp"

namespace ICMP
{
    constexpr uint8_t TYPE_ECHO_REPLY    = 0;
    constexpr uint8_t TYPE_UNREACHABLE   = 3;
    constexpr uint8_t TYPE_ECHO_REQUEST  = 8;
    constexpr uint8_t TYPE_TIME_EXCEEDED = 11;
 
    constexpr uint8_t CODE_NET_UNREACHABLE  = 0;
    constexpr uint8_t CODE_HOST_UNREACHABLE = 1;
    constexpr uint8_t CODE_PORT_UNREACHABLE = 3;

    constexpr uint8_t CODE_TTL_EXCEEDED = 0;

    constexpr uint8_t CODE_DEFAULT = 0;
}

class ICMPMessage : public Packet{
    private:
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    std::vector<uint8_t> icmpPayload;  // 4 byte pertama = extended header, sisanya = data
    uint16_t calculateChecksum() const;
    bool verifyChecksum() const;

    public:
    ICMPMessage();
    ICMPMessage(uint8_t type, uint8_t code, std::vector<uint8_t> icmpPayload);
    ~ICMPMessage() override;

    uint8_t  getICMPType() const { return type; }
    uint8_t  getCode() const { return code; }
    uint16_t getChecksum() const { return checksum; }
    const std::vector<uint8_t>& getICMPPayload() const { return icmpPayload; }

    uint16_t getIdentifier() const;
    uint16_t getSequenceNumber() const;
    std::vector<uint8_t> getEchoData() const;

    void deserialize(const std::vector<uint8_t> &raw) override;
    std::vector<uint8_t> toBytes() const override;
    std::string getType() const override { return "ICMP"; }

    bool isChecksumValid() const;

    static ICMPMessage buildEchoRequest(uint16_t identifier,
                                        uint16_t sequenceNum,
                                        const std::vector<uint8_t>& data = {});
 
    static ICMPMessage buildEchoReply(const ICMPMessage& request);
 
    // originalHeader = IP header asli + 8 byte pertama datagram yang gagal
    static ICMPMessage buildTimeExceeded(const std::vector<uint8_t>& originalHeader);
 
    static ICMPMessage buildUnreachable(const std::vector<uint8_t>& originalHeader,
                                        uint8_t code = ICMP::CODE_NET_UNREACHABLE);
};