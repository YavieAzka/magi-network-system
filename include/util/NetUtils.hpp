#pragma once 
#include <string> 
#include <cstdint> 
#include <vector> 
/* basically we need this because in runtime, ip byte conversion is needed, see: ByteUtils and serialization */ 
class NetUtils {    
public: 
    static uint32_t stringToIp(const std::string& ip_string); 
    static std::string ipToString(const uint32_t ip_raw); 
    uint32_t applyMask(uint32_t ip, uint8_t prefix); 

    std::vector<uint8_t> stringToMac(const std::string& mac_string); 
    std::string macToString(const std::vector<uint8_t> mac); 
    bool isBroadcastMac(const std::vector<uint8_t> mac); 
}; 
