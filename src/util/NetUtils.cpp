#include "util/NetUtils.hpp"

#include <sstream>
#include <iomanip>
#include <stdexcept>

uint32_t NetUtils::stringToIp(const std::string &ip_string)
{
    std::vector<int> parts;
    std::stringstream ss(ip_string);
    std::string segment;

    while (std::getline(ss, segment, '.'))
    {
        try
        {
            parts.push_back(std::stoi(segment));
        }
        catch (...)
        {
            return 0;
        }
    }

    if (parts.size() != 4)
        return 0;
    return (static_cast<uint32_t>(parts[0]) << 24) |
           (static_cast<uint32_t>(parts[1]) << 16) |
           (static_cast<uint32_t>(parts[2]) << 8) |
           (static_cast<uint32_t>(parts[3]));
}

std::string NetUtils::ipToString(uint32_t ip_raw)
{
    std::ostringstream oss;
    oss << ((ip_raw >> 24) & 0xFF) << "."
        << ((ip_raw >> 16) & 0xFF) << "."
        << ((ip_raw >> 8) & 0xFF) << "."
        << (ip_raw & 0xFF);
    return oss.str();
}

uint32_t NetUtils::cidrToMask(uint8_t prefix)
{
    if (prefix == 0)
        return 0;
    if (prefix >= 32)
        return 0xFFFFFFFF;
    return 0xFFFFFFFF << (32 - prefix);
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

std::pair<uint32_t, uint8_t> NetUtils::parseCIDR(const std::string &cidr)
{
    auto slash = cidr.find('/');
    if (slash == std::string::npos)
        throw std::runtime_error("parseCIDR: tidak ada '/' pada: " + cidr);
    std::string ip_str = cidr.substr(0, slash);
    uint8_t prefix = static_cast<uint8_t>(std::stoi(cidr.substr(slash + 1)));
    return {stringToIp(ip_str), prefix};
}

bool NetUtils::isInNetwork(uint32_t ip, uint32_t network, uint8_t prefix)
{
    return applyMask(ip, prefix) == applyMask(network, prefix);
}