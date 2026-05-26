#include "layer7/DHCPMessage.hpp"
#include <sstream>
#include <stdexcept>
#include <iomanip>
#include <cstring>

std::vector<uint8_t> DHCPMessage::ipToBytes(const std::string& ip) {
    std::vector<uint8_t> result;
    std::stringstream ss(ip);
    std::string octet;
    while (std::getline(ss, octet, '.')) {
        result.push_back(static_cast<uint8_t>(std::stoi(octet)));
    }
    // Jika IP kosong atau tidak valid, isi dengan 0.0.0.0
    while (result.size() < 4) result.push_back(0);
    return result;
}

std::string DHCPMessage::bytesToIp(const uint8_t* ptr) {
    return std::to_string(ptr[0]) + "." +
           std::to_string(ptr[1]) + "." +
           std::to_string(ptr[2]) + "." +
           std::to_string(ptr[3]);
}

std::vector<uint8_t> DHCPMessage::macToBytes(const std::string& mac) {
    std::vector<uint8_t> result;
    std::stringstream ss(mac);
    std::string byte;
    while (std::getline(ss, byte, ':')) {
        result.push_back(static_cast<uint8_t>(std::stoi(byte, nullptr, 16)));
    }
    while (result.size() < 6) result.push_back(0);
    return result;
}
std::string DHCPMessage::bytesToMac(const uint8_t* ptr) {
    std::ostringstream oss;
    for (int i = 0; i < 6; ++i) {
        if (i > 0) oss << ":";
        oss << std::uppercase << std::hex
            << std::setw(2) << std::setfill('0')
            << static_cast<int>(ptr[i]);
    }
    return oss.str();
}


void DHCPMessage::setMessageType(uint8_t msgType) {
    options[DHCP_OPT_MESSAGE_TYPE] = {msgType};
}

uint8_t DHCPMessage::getMessageType() const {
    auto it = options.find(DHCP_OPT_MESSAGE_TYPE);
    if (it == options.end() || it->second.empty()) return 0;
    return it->second[0];
}

void DHCPMessage::setSubnetMask(uint8_t prefixLen) {
    uint32_t mask = (prefixLen == 0) ? 0 : (~0u << (32 - prefixLen));
    options[DHCP_OPT_SUBNET_MASK] = {
        static_cast<uint8_t>((mask >> 24) & 0xFF),
        static_cast<uint8_t>((mask >> 16) & 0xFF),
        static_cast<uint8_t>((mask >>  8) & 0xFF),
        static_cast<uint8_t>( mask        & 0xFF)
    };
}

void DHCPMessage::setRouter(const std::string& gatewayIp) {
    options[DHCP_OPT_ROUTER] = ipToBytes(gatewayIp);
}

void DHCPMessage::setLeaseTime(uint32_t seconds) {
    options[DHCP_OPT_LEASE_TIME] = {
        static_cast<uint8_t>((seconds >> 24) & 0xFF),
        static_cast<uint8_t>((seconds >> 16) & 0xFF),
        static_cast<uint8_t>((seconds >>  8) & 0xFF),
        static_cast<uint8_t>( seconds        & 0xFF)
    };
}

void DHCPMessage::setServerId(const std::string& serverIp) {
    options[DHCP_OPT_SERVER_ID] = ipToBytes(serverIp);
}

std::string DHCPMessage::getServerId() const {
    auto it = options.find(DHCP_OPT_SERVER_ID);
    if (it == options.end() || it->second.size() < 4) return "";
    return bytesToIp(it->second.data());
}

std::string DHCPMessage::getRouter() const {
    auto it = options.find(DHCP_OPT_ROUTER);
    if (it == options.end() || it->second.size() < 4) return "";
    return bytesToIp(it->second.data());
}

std::string DHCPMessage::getSubnetMask() const {
    auto it = options.find(DHCP_OPT_SUBNET_MASK);
    if (it == options.end() || it->second.size() < 4) return "";
    return bytesToIp(it->second.data());
}


std::vector<uint8_t> DHCPMessage::toBytes() const {
    std::vector<uint8_t> buf;

    buf.push_back(op);
    buf.push_back(htype);
    buf.push_back(hlen);
    buf.push_back(hops);

    buf.push_back((xid >> 24) & 0xFF);
    buf.push_back((xid >> 16) & 0xFF);
    buf.push_back((xid >>  8) & 0xFF);
    buf.push_back( xid        & 0xFF);

    buf.push_back((secs >> 8) & 0xFF);
    buf.push_back( secs       & 0xFF);

    buf.push_back((flags >> 8) & 0xFF);
    buf.push_back( flags       & 0xFF);

    for (const auto& ip : {ciaddr, yiaddr, siaddr, giaddr}) {
        auto b = ipToBytes(ip.empty() ? "0.0.0.0" : ip);
        buf.insert(buf.end(), b.begin(), b.end());
    }

    auto macBytes = macToBytes(chaddr);
    buf.insert(buf.end(), macBytes.begin(), macBytes.end());
    for (int i = 0; i < 10; ++i) buf.push_back(0); // padding

    for (int i = 0; i < 64; ++i) buf.push_back(0);

    for (int i = 0; i < 128; ++i) buf.push_back(0);


    buf.push_back(0x63);
    buf.push_back(0x82);
    buf.push_back(0x53);
    buf.push_back(0x63);

    for (const auto& [code, value] : options) {
        buf.push_back(code);
        buf.push_back(static_cast<uint8_t>(value.size()));
        buf.insert(buf.end(), value.begin(), value.end());
    }

    buf.push_back(DHCP_OPT_END);

    return buf;
}


DHCPMessage DHCPMessage::deserialize(const std::vector<uint8_t>& raw) {
    if (raw.size() < 240) {
        throw std::invalid_argument("DHCPMessage terlalu pendek: " +
                                    std::to_string(raw.size()) + " bytes");
    }

    DHCPMessage msg;
    size_t i = 0;

    msg.op    = raw[i++];
    msg.htype = raw[i++];
    msg.hlen  = raw[i++];
    msg.hops  = raw[i++];

    msg.xid = (static_cast<uint32_t>(raw[i])   << 24) |
               (static_cast<uint32_t>(raw[i+1]) << 16) |
               (static_cast<uint32_t>(raw[i+2]) <<  8) |
                static_cast<uint32_t>(raw[i+3]);
    i += 4;

    msg.secs  = (static_cast<uint16_t>(raw[i]) << 8) | raw[i+1]; i += 2;
    msg.flags = (static_cast<uint16_t>(raw[i]) << 8) | raw[i+1]; i += 2;

    msg.ciaddr = bytesToIp(&raw[i]); i += 4;
    msg.yiaddr = bytesToIp(&raw[i]); i += 4;
    msg.siaddr = bytesToIp(&raw[i]); i += 4;
    msg.giaddr = bytesToIp(&raw[i]); i += 4;

    msg.chaddr = bytesToMac(&raw[i]); i += 16;

    i += 64 + 128;

    uint32_t cookie = (static_cast<uint32_t>(raw[i])   << 24) |
                      (static_cast<uint32_t>(raw[i+1]) << 16) |
                      (static_cast<uint32_t>(raw[i+2]) <<  8) |
                       static_cast<uint32_t>(raw[i+3]);
    if (cookie != DHCP_MAGIC_COOKIE) {
        throw std::invalid_argument("Magic cookie DHCP tidak valid");
    }
    i += 4;

    while (i < raw.size()) {
        uint8_t code = raw[i++];
        if (code == DHCP_OPT_END) break;  
        if (code == 0) continue;          

        if (i >= raw.size()) break;
        uint8_t len = raw[i++];

        if (i + len > raw.size()) break; 
        msg.options[code] = std::vector<uint8_t>(raw.begin() + i,
                                                  raw.begin() + i + len);
        i += len;
    }

    return msg;
}