#include "util/NetUtils.hpp"

#include <arpa/inet.h>
#include <sstream>
#include <iomanip>
#include <stdexcept>

uint32_t NetUtils::stringToIp(const std::string &ip_string)
{
    struct in_addr dst;
    if (inet_pton(AF_INET, ip_string.c_str(), &dst) != 1)
        return 0;
    return ntohl(dst.s_addr);
}

std::string NetUtils::ipToString(uint32_t ip_raw)
{
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%u.%u.%u.%u",
             (ip_raw >> 24) & 0xFF,
             (ip_raw >> 16) & 0xFF,
             (ip_raw >> 8) & 0xFF,
             ip_raw & 0xFF);
    return buffer;
}

uint32_t NetUtils::applyMask(uint32_t ip, uint8_t prefix)
{
    if (prefix == 0)
        return 0;
    uint32_t mask = 0xFFFFFFFF << (32 - prefix);
    return ip & mask;
}

std::vector<uint8_t> NetUtils::stringToMac(const std::string &mac_string)
{
    std::vector<uint8_t> bytes;
    std::istringstream ss(mac_string);
    std::string token;
    while (std::getline(ss, token, ':'))
    {
        bytes.push_back(static_cast<uint8_t>(std::stoul(token, nullptr, 16)));
    }
    if (bytes.size() != 6)
        throw std::runtime_error("Invalid MAC address: " + mac_string);
    return bytes;
}

std::string NetUtils::macToString(const std::vector<uint8_t> &mac)
{
    std::ostringstream oss;
    for (int i = 0; i < 6; i++)
    {
        if (i > 0)
            oss << ":";
        oss << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
            << static_cast<int>(mac.at(i));
    }
    return oss.str();
}

bool NetUtils::isBroadcastMac(const std::vector<uint8_t> &mac)
{
    for (int i = 0; i < 6; i++)
    {
        if (mac.at(i) != 0xFF)
            return false;
    }
    return true;
}