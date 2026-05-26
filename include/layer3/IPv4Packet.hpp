// include/layer3/IPv4Packet.hpp
#ifndef IPV4_PACKET_HPP
#define IPV4_PACKET_HPP
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <iostream>

using namespace std;

class IPv4Packet {
public:
    std::uint8_t ttl;
    std::uint8_t protocol; // 1=ICMP, 6=TCP, 17=UDP (ikutin spek)
    uint16_t checksum;
    std::string srcIP; 
    std::string dstIP;
    std::vector<std::uint8_t> payload;

    std::uint8_t version = 4;
    std::uint8_t ihl = 5;
    std::uint8_t tos = 0;
    uint16_t totalLen;
    uint16_t identification = 0;
    uint16_t flagsFragment  = 0;

    IPv4Packet() = default;
    IPv4Packet(const std::string& src, const std::string& dst,
               std::uint8_t proto, std::uint8_t ttl,
               const std::vector<std::uint8_t>& payload);

    // serialization
    std::vector<std::uint8_t> toBytes() const;
    void deserialize(const std::vector<std::uint8_t>& raw);

    // checksum
    uint16_t calculateChecksum() const;
    bool verifyChecksum() const;

    // TTL
    void decrementTTL();

    // fragmentation helpers
    bool isFragment() const;
    bool isMoreFragments() const;
    bool isDontFragment() const;
    uint16_t getFragmentOffset() const;

    // fragment this packet for a given path MTU; returns empty if DF set
    std::vector<IPv4Packet> fragment(uint16_t pathMTU) const;

    // utils
    static constexpr std::uint8_t PROTOCOL_ICMP = 1;
    static constexpr std::uint8_t PROTOCOL_TCP  = 6;
    static constexpr std::uint8_t PROTOCOL_UDP  = 17;

    // IP flags
    static constexpr uint16_t FLAG_DF = 0x4000;
    static constexpr uint16_t FLAG_MF = 0x2000;
    static constexpr uint16_t FRAGMENT_OFFSET_MASK = 0x1FFF;
};

#endif