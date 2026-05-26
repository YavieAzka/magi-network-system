#pragma once
#include <string>
#include <map>
#include <cstdint>

struct NATEntry {
    std::string privateIp;
    uint16_t    privatePort;
    std::string publicIp;
    uint16_t    publicPort;  // mapped port
};

class NAT {
public:
    NAT(const std::string& publicIp,
        uint16_t portStart = 40000,
        uint16_t portEnd   = 50000);

    uint16_t translateOutbound(const std::string& privateIp,
                               uint16_t privatePort);

    bool translateInbound(uint16_t publicPort,
                          std::string& outPrivateIp,
                          uint16_t&    outPrivatePort) const;

    void printTable() const;
    const std::string& getPublicIp() const { return publicIp_; }
    bool translateInboundICMP(uint16_t identifier,
                          std::string& outPrivateIp,
                          uint16_t& outPrivatePort) const;

private:
    std::string publicIp_;
    uint16_t    portStart_;
    uint16_t    portEnd_;
    uint16_t    nextPort_;

    // key: "privateIp:privatePort" -> NATEntry
    std::map<std::string, NATEntry> outboundTable_;

    // key: publicPort -> NATEntry
    std::map<uint16_t, NATEntry> inboundTable_;

    std::string makeKey(const std::string& ip, uint16_t port) const;
};