#pragma once
#include <cstdint>
#include <vector>
#include <string>

struct UDPSegment
{
    uint16_t srcPort;
    uint16_t dstPort;
    uint16_t length;
    uint16_t checksum;
    std::vector<uint8_t> payload;

    std::vector<uint8_t> toBytes() const;

    static UDPSegment deserialize(const std::vector<uint8_t> &raw);

    static std::vector<uint8_t> buildPseudoHeader(
        const std::string &srcIP,
        const std::string &dstIP,
        uint16_t udpLength);

    static uint16_t calculateChecksum(
        const std::string &srcIP,
        const std::string &dstIP,
        const UDPSegment &seg);

    bool verifyChecksum(
        const std::string &srcIP,
        const std::string &dstIP) const;
};