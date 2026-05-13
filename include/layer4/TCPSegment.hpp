#pragma once
#include <vector>
#include <cstdint>
#include <string>
#include "../util/NetUtils.hpp"

using namespace std;

// Flag bitmask
constexpr uint8_t TCP_FLAG_FIN = 0x01; // finish
constexpr uint8_t TCP_FLAG_SYN = 0x02; // synchronize
constexpr uint8_t TCP_FLAG_RST = 0x04; // reset
constexpr uint8_t TCP_FLAG_PSH = 0x08; // push
constexpr uint8_t TCP_FLAG_ACK = 0x10; // acknowledgment
constexpr uint8_t TCP_FLAG_URG = 0x20; // urgent

/** Struktur Pseudoheader
 * 4 byte src ip
 * 4 byte dst ip
 * 1 byte zero
 * 1 byte protocol: tcp/udp/icmp
 * 2 byte tcp length
 */
struct TCPPseudoheader{
    uint32_t srcIP;
    uint32_t dstIP;
    uint8_t zero;
    uint8_t protocol;
    uint16_t tcplen;
};

struct TCPSegment{
    uint16_t srcPort;
    uint16_t dstPort;
    uint32_t seqNum;
    uint32_t ackNum;
    uint8_t dataOffset; // 4 bit + 4 reserved 0000
    uint8_t flags;
    uint16_t windowSize;
    uint16_t checksum;
    uint16_t urgentPointer;
    vector<uint8_t> payload;

    TCPSegment();
    vector<uint8_t> toBytes() const;
    static TCPSegment deserialize(const vector<uint8_t>& raw);

    static TCPPseudoheader buildPseudoHeader(
        const string& srcIP,
        const string& dstIP,
        uint16_t tcpLen
    );

    void calculateChecksum(const string& srcIP, const string& dstIP);
    bool verifyChecksum(const string& srcIP, const string& dstIP) const;
    void print() const;
};