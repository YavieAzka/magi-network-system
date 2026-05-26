#include "middleboxes/NAT.hpp"
#include <iostream>
#include <iomanip>

NAT::NAT(const std::string& publicIp, uint16_t portStart, uint16_t portEnd)
    : publicIp_(publicIp), portStart_(portStart),
      portEnd_(portEnd), nextPort_(portStart) {}

std::string NAT::makeKey(const std::string& ip, uint16_t port) const {
    return ip + ":" + std::to_string(port);
}

uint16_t NAT::translateOutbound(const std::string& privateIp,
                                uint16_t privatePort) {
    std::string key = makeKey(privateIp, privatePort);

    // Cek apakah sudah ada mapping
    auto it = outboundTable_.find(key);
    if (it != outboundTable_.end()) {
        return it->second.publicPort;
    }

    // Alokasi port baru
    if (nextPort_ > portEnd_) {
        std::cerr << "[NAT] Port pool habis!\n";
        return 0;
    }

    NATEntry entry;
    entry.privateIp   = privateIp;
    entry.privatePort = privatePort;
    entry.publicIp    = publicIp_;
    entry.publicPort  = nextPort_++;

    outboundTable_[key]              = entry;
    inboundTable_[entry.publicPort]  = entry;

    std::cout << "[NAT] Mapping baru: "
              << privateIp << ":" << privatePort
              << " → " << publicIp_ << ":" << entry.publicPort << "\n";

    return entry.publicPort;
}

bool NAT::translateInbound(uint16_t publicPort,
                           std::string& outPrivateIp,
                           uint16_t&    outPrivatePort) const {
    auto it = inboundTable_.find(publicPort);
    if (it == inboundTable_.end()) return false;

    outPrivateIp   = it->second.privateIp;
    outPrivatePort = it->second.privatePort;
    return true;
}

void NAT::printTable() const {
    std::cout << std::dec << std::setfill(' ');
    std::cout << "\n--- NAT Translation Table ---\n";
    std::cout << std::left
              << std::setw(22) << "Private"
              << std::setw(22) << "Public" << "\n";
    std::cout << std::string(44, '-') << "\n";
    if (outboundTable_.empty()) {
        std::cout << "(kosong)\n";
    } else {
        for (const auto& [key, e] : outboundTable_) {
            std::cout << std::left
                      << std::setw(22) << (e.privateIp + ":" + std::to_string(e.privatePort))
                      << std::setw(22) << (e.publicIp  + ":" + std::to_string(e.publicPort))
                      << "\n";
        }
    }
    std::cout << std::string(44, '-') << "\n\n";
}

bool NAT::translateInboundICMP(uint16_t identifier,
                               std::string& outPrivateIp,
                               uint16_t& outPrivatePort) const {
    for (const auto& [mappedPort, entry] : inboundTable_) {
        if (entry.privatePort == identifier) {
            outPrivateIp   = entry.privateIp;
            outPrivatePort = entry.privatePort;
            return true;
        }
    }
    return false;
}