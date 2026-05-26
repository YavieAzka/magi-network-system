#include "layer7/DHCPClient.hpp"
#include "util/Exceptions.hpp"
#include "core/Host.hpp"
#include "core/Interface.hpp"
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <stdexcept>

DHCPClient::DHCPClient(Host* host)
    : host_(host), socket_(nullptr), xid_(0)
{
}

DHCPClient::~DHCPClient() {
    if (socket_) {
        socket_->close();
        delete socket_;
        socket_ = nullptr;
    }
}


bool DHCPClient::discover() {
    socket_ = new MagiSocket(host_, MAGI_SOCK_DGRAM, MAGI_AF_INET);
    socket_->bind("0.0.0.0", DHCP_CLIENT_PORT);

    xid_ = static_cast<uint32_t>(std::rand());

    Interface* ifc = host_->getInterface(1);
    std::string myMac = ifc ? ifc->getMacAddress() : "00:00:00:00:00:00";

    std::cout << "[DHCPClient:" << host_->getName() << "] "
              << "Memulai DORA (xid=" << xid_ << ", MAC=" << myMac << ")\n";

    // discover
    DHCPMessage discover;
    discover.op = DHCP_OP_REQUEST;
    discover.htype = DHCP_HTYPE_ETHERNET;
    discover.hlen = DHCP_HLEN_ETHERNET;
    discover.hops = 0;
    discover.xid = xid_;
    discover.secs = 0;
    discover.flags  = 0;
    discover.ciaddr = "0.0.0.0"; 
    discover.yiaddr = "0.0.0.0";
    discover.siaddr = "0.0.0.0";
    discover.giaddr = "0.0.0.0";
    discover.chaddr = myMac;
    discover.setMessageType(DHCP_MSG_DISCOVER);

    std::cout << "[DHCPClient:" << host_->getName() << "] "
              << "Mengirim DHCP Discover (broadcast)...\n";
    sendMessage(discover, "255.255.255.255");

    std::cout << "[DHCPClient:" << host_->getName() << "] "
              << "Menunggu DHCP Offer...\n";

    // Offer
    DHCPMessage offer = recvMessage();
    if (offer.getMessageType() != DHCP_MSG_OFFER) {
        socket_->close();
        delete socket_;
        socket_ = nullptr;
        throw NetworkException("DHCPClient on " + host_->getName() + " did not receive DHCP Offer, DORA failed");
    }

    if (offer.xid != xid_) {
        throw NetworkException("DHCPClient on " + host_->getName() + " xid Offer mismatch, ignored");
    }

    std::string offeredIp  = offer.yiaddr;
    std::string serverId   = offer.getServerId();
    std::string gateway    = offer.getRouter();
    std::string subnetMask = offer.getSubnetMask();

    std::cout << "[DHCPClient:" << host_->getName() << "] "
              << "Menerima DHCP Offer: IP=" << offeredIp
              << " GW=" << gateway
              << " dari server " << serverId << "\n";

    // request
    DHCPMessage request;
    request.op = DHCP_OP_REQUEST;
    request.htype = DHCP_HTYPE_ETHERNET;
    request.hlen = DHCP_HLEN_ETHERNET;
    request.hops = 0;
    request.xid = xid_;
    request.secs = 0;
    request.flags = 0;
    request.ciaddr = "0.0.0.0"; 
    request.yiaddr = "0.0.0.0";
    request.siaddr = serverId;
    request.giaddr = "0.0.0.0";
    request.chaddr = myMac;
    request.setMessageType(DHCP_MSG_REQUEST);
    request.setServerId(serverId); 

    std::cout << "[DHCPClient:" << host_->getName() << "] "
              << "Mengirim DHCP Request untuk IP=" << offeredIp << "...\n";
    sendMessage(request, "255.255.255.255");


    std::cout << "[DHCPClient:" << host_->getName() << "] "
              << "Menunggu DHCP ACK...\n";

    DHCPMessage ack = recvMessage();
    if (ack.getMessageType() != DHCP_MSG_ACK) {
        socket_->close();
        delete socket_;
        socket_ = nullptr;
        throw NetworkException("DHCPClient on " + host_->getName() + " did not receive DHCP ACK, DORA failed");
    }

    if (ack.xid != xid_) {
        throw NetworkException("DHCPClient on " + host_->getName() + " xid ACK mismatch, ignored");
    }

    // dora done
    std::string finalIp = ack.yiaddr;

    std::string cidrIp = finalIp;
    if (!subnetMask.empty()) {
        uint8_t prefix = 0;
        std::stringstream ss(subnetMask);
        std::string octet;
        while (std::getline(ss, octet, '.')) {
            uint8_t val = static_cast<uint8_t>(std::stoi(octet));
            while (val) { prefix += (val & 1); val >>= 1; }
        }
        cidrIp = finalIp + "/" + std::to_string(prefix);
    }

    std::cout << "[DHCPClient:" << host_->getName() << "] "
              << "DHCP ACK diterima! IP=" << cidrIp
              << " GW=" << ack.getRouter() << "\n";
    host_->setIpAddress(cidrIp);
    if (!ack.getRouter().empty()) {
        host_->setDefaultGateway(ack.getRouter());
    }

    std::cout << "[DHCPClient:" << host_->getName() << "] "
              << "Konfigurasi jaringan berhasil diperbarui.\n";

    socket_->close();
    delete socket_;
    socket_ = nullptr;

    return true;
}

void DHCPClient::sendMessage(const DHCPMessage&  msg,
                             const std::string&  dstIp) {
    std::vector<uint8_t> payload = msg.toBytes();
    socket_->connect(dstIp, DHCP_SERVER_PORT);
    socket_->send(payload);
}


DHCPMessage DHCPClient::recvMessage() {
    // Dalam simulator single-threaded, paket sudah ada di buffer
    // setelah send() karena pengiriman bersifat synchronous (blocking)
    // Jadi recv() di sini akan langsung dapat data tanpa perlu loop tunggu
    // Later kalau mau implement bonus multithreading nanti ini bisa di-adjust lagi
    std::vector<uint8_t> raw = socket_->recv(4096);

    if (raw.empty()) {
        return DHCPMessage{};
    }

    try {
        return DHCPMessage::deserialize(raw);
    } catch (const std::exception& e) {
        throw ParseException("DHCPClient failed to parse DHCP reply: " + std::string(e.what()));
    }
}