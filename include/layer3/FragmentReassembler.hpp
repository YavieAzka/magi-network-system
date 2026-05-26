#pragma once

#include "layer3/IPv4Packet.hpp"
#include <map>
#include <vector>
#include <string>
#include <cstdint>
#include <chrono>
#include <optional>

struct FragmentKey
{
    std::string srcIP;
    std::string dstIP;
    uint16_t identification;
    uint8_t protocol;

    bool operator<(const FragmentKey &o) const
    {
        if (srcIP != o.srcIP) return srcIP < o.srcIP;
        if (dstIP != o.dstIP) return dstIP < o.dstIP;
        if (identification != o.identification) return identification < o.identification;
        return protocol < o.protocol;
    }
};

struct ReassemblyBuffer
{
    std::vector<IPv4Packet> fragments;
    std::chrono::steady_clock::time_point createdAt;
    bool complete;
};

class FragmentReassembler
{
public:
    static constexpr size_t MAX_BUFFER_SIZE = 65536;

    std::optional<IPv4Packet> addFragment(const IPv4Packet &frag);

    void cleanup(std::chrono::seconds timeout = std::chrono::seconds(60));

    size_t pendingCount() const;

private:
    std::map<FragmentKey, ReassemblyBuffer> buffers_;

    IPv4Packet reassemble(const FragmentKey &key);
};