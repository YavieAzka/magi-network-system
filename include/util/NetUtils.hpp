#ifndef NET_UTILS_HPP
#define NET_UTILS_HPP

#include <string>
#include <cstdint>
#include <vector>

class NetUtils
{
public:
    static uint32_t stringToIp(const std::string &ip_string);
    static std::string ipToString(uint32_t ip_raw);
    static uint32_t applyMask(uint32_t ip, uint8_t prefix);
    static std::vector<uint8_t> stringToMac(const std::string &mac_string);
    static std::string macToString(const std::vector<uint8_t> &mac);
    static bool isBroadcastMac(const std::vector<uint8_t> &mac);
    static std::pair<uint32_t, uint8_t> parseCIDR(const std::string &cidr);
    static bool isInNetwork(uint32_t ip, uint32_t network, uint8_t prefix);
    static uint32_t cidrToMask(uint8_t prefix);
};

#endif
