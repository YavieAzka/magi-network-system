#include "layer7/DHCPServer.hpp"
#include "util/Exceptions.hpp"
#include "core/Host.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>

uint32_t DHCPServer::ipToUint32(const std::string& ip) {
    uint32_t result = 0;
    std::stringstream ss(ip);
    std::string octet;
    while (std::getline(ss, octet, '.')) {
        result = (result << 8) | static_cast<uint8_t>(std::stoi(octet));
    }
    return result;
}

std::string DHCPServer::uint32ToIp(uint32_t ip) {
    return std::to_string((ip >> 24) & 0xFF) + "." +
           std::to_string((ip >> 16) & 0xFF) + "." +
           std::to_string((ip >>  8) & 0xFF) + "." +
           std::to_string( ip        & 0xFF);
}

DHCPServer::DHCPServer(Host*              host,
                       const std::string& poolStart,
                       const std::string& poolEnd,
                       const std::string& gateway,
                       uint8_t            prefixLen)
    : host_(host),
      socket_(nullptr),
      gateway_(gateway),
      prefixLen_(prefixLen)
{
    serverIp_ = host_->getIpAddress();
    uint32_t start = ipToUint32(poolStart);
    uint32_t end   = ipToUint32(poolEnd);
    for (uint32_t ip = start; ip <= end; ++ip) {
        ipPool_.push_back(uint32ToIp(ip));
    }

    std::cout << "[DHCPServer] Inisialisasi di " << serverIp_
              << ", pool: " << poolStart << " - " << poolEnd
              << " (" << ipPool_.size() << " IP tersedia)\n";
}

DHCPServer::~DHCPServer() {
    if (socket_) {
        socket_->close();
        delete socket_;
        socket_ = nullptr;
    }
}


void DHCPServer::start() {
    socket_ = new MagiSocket(host_, MAGI_SOCK_DGRAM, MAGI_AF_INET);
    socket_->bind("0.0.0.0", DHCP_SERVER_PORT);

    // Daftarkan callback agar setiap data masuk langsung diproses
    socket_->setOnReceive([this](const std::string& srcIp, const std::vector<uint8_t>& data) {
        this->handleIncoming(srcIp, data);
    });

    std::cout << "[DHCPServer] Siap menerima request di port "
              << DHCP_SERVER_PORT << "\n";
}


void DHCPServer::handleIncoming(const std::string& srcIp,
                                const std::vector<uint8_t>& udpPayload) {
    DHCPMessage msg;
    try {
        msg = DHCPMessage::deserialize(udpPayload);
    } catch (const std::exception& e) {
        throw ParseException("DHCPServer failed to parse DHCP message: " + std::string(e.what()));
    }

    uint8_t msgType = msg.getMessageType();
    switch (msgType) {
        case DHCP_MSG_DISCOVER:
            std::cout << "[DHCPServer] Menerima DHCP Discover dari MAC "
                      << msg.chaddr << "\n";
            handleDiscover(msg, srcIp);
            break;
        case DHCP_MSG_REQUEST:
            std::cout << "[DHCPServer] Menerima DHCP Request dari MAC "
                      << msg.chaddr << "\n";
            handleRequest(msg, srcIp);
            break;
        default:
            std::cout << "[DHCPServer] Tipe pesan tidak dikenal: "
                      << static_cast<int>(msgType) << ", diabaikan.\n";
            break;
    }
}

std::string DHCPServer::allocateIp(const std::string& clientMac) {
    // Jika client ini sudah pernah dapat IP sebelumnya, kembalikan IP yang sama
    // (ini memastikan client selalu dapat IP yang konsisten)
    auto existingLease = leaseTable_.find(clientMac);
    if (existingLease != leaseTable_.end()) {
        return existingLease->second;
    }

    // Kumpulkan semua IP yang sudah terpakai
    std::vector<std::string> usedIps;
    for (const auto& [mac, ip] : leaseTable_) {
        usedIps.push_back(ip);
    }
    for (const auto& [mac, ip] : pendingOffers_) {
        usedIps.push_back(ip);
    }
    for (const auto& ip : ipPool_) {
        bool used = false;
        for (const auto& usedIp : usedIps) {
            if (usedIp == ip) { used = true; break; }
        }
        if (!used) return ip;
    }

    throw NetworkException("DHCPServer IP pool exhausted, cannot allocate new IP");
}


void DHCPServer::handleDiscover(const DHCPMessage& msg,
                                const std::string& /*srcIp*/) {
    std::string offeredIp = allocateIp(msg.chaddr);
    if (offeredIp.empty()) {
        return;
    }

    pendingOffers_[msg.chaddr] = offeredIp;

    DHCPMessage offer;
    offer.op     = DHCP_OP_REPLY;
    offer.htype  = DHCP_HTYPE_ETHERNET;
    offer.hlen   = DHCP_HLEN_ETHERNET;
    offer.hops   = 0;
    offer.xid    = msg.xid;
    offer.secs   = 0;
    offer.flags  = 0;
    offer.ciaddr = "0.0.0.0";
    offer.yiaddr = offeredIp;
    offer.siaddr = serverIp_;
    offer.giaddr = "0.0.0.0";
    offer.chaddr = msg.chaddr;

    // Set options
    offer.setMessageType(DHCP_MSG_OFFER);
    offer.setSubnetMask(prefixLen_);
    offer.setRouter(gateway_);
    offer.setLeaseTime(86400);    // 24 jam
    offer.setServerId(serverIp_);

    std::cout << "[DHCPServer] Mengirim DHCP Offer: IP=" << offeredIp
              << " GW=" << gateway_ << " ke MAC " << msg.chaddr << "\n";

    sendReply(offer, "255.255.255.255");
}


void DHCPServer::handleRequest(const DHCPMessage& msg,
                               const std::string& /*srcIp*/) {
    // Cek apakah Request ini ditujukan ke server ini
    // (client bisa broadcast Request ke semua server yang tadi Offer)
    std::string requestedServerId = msg.getServerId();
    if (!requestedServerId.empty() && requestedServerId != serverIp_) {
        std::cout << "[DHCPServer] Request ditujukan ke server lain ("
                  << requestedServerId << "), diabaikan.\n";
        return;
    }

    auto it = pendingOffers_.find(msg.chaddr);
    std::string assignedIp;

    if (it != pendingOffers_.end()) {
        assignedIp = it->second;
        pendingOffers_.erase(it);  
    } else {
        auto leaseIt = leaseTable_.find(msg.chaddr);
        if (leaseIt != leaseTable_.end()) {
            assignedIp = leaseIt->second;
        } else {
            throw NetworkException("DHCPServer has no offer for MAC " + msg.chaddr);
        }
    }

    leaseTable_[msg.chaddr] = assignedIp;

    DHCPMessage ack;
    ack.op = DHCP_OP_REPLY;
    ack.htype = DHCP_HTYPE_ETHERNET;
    ack.hlen = DHCP_HLEN_ETHERNET;
    ack.hops = 0;
    ack.xid = msg.xid; 
    ack.secs = 0;
    ack.flags = 0;
    ack.ciaddr = "0.0.0.0";
    ack.yiaddr = assignedIp;   
    ack.siaddr = serverIp_;
    ack.giaddr = "0.0.0.0";
    ack.chaddr = msg.chaddr;

    ack.setMessageType(DHCP_MSG_ACK);
    ack.setSubnetMask(prefixLen_);
    ack.setRouter(gateway_);
    ack.setLeaseTime(86400);
    ack.setServerId(serverIp_);

    std::cout << "[DHCPServer] Mengirim DHCP ACK: IP=" << assignedIp
              << " resmi diberikan ke MAC " << msg.chaddr << "\n";

    sendReply(ack, "255.255.255.255");
}

void DHCPServer::sendReply(const DHCPMessage& reply, const std::string& dstIp) {
    std::vector<uint8_t> payload = reply.toBytes();
    host_->sendUDPBroadcast(DHCP_SERVER_PORT, DHCP_CLIENT_PORT, payload);
}

// debug helper
void DHCPServer::printLeases() const {
    std::cout << "\n--- DHCP Lease Table ---\n";
    if (leaseTable_.empty()) {
        std::cout << "  (kosong)\n";
    }
    for (const auto& [mac, ip] : leaseTable_) {
        std::cout << "  " << mac << " → " << ip << "\n";
    }
    std::cout << "------------------------\n\n";
}