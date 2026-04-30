#include "util/NetUtils.hpp" 
#include <arpa/inet.h> 
#include <sstream> 
#include <string> 
#include <iomanip> 

uint32_t NetUtils::stringToIp(const std::string &ip_string){
    struct in_addr dst; 
    uint32_t result =  inet_pton(AF_INET, ip_string.c_str(), &dst); 
    return result; 
} 

std::string NetUtils::ipToString(const uint32_t ip_raw){
    char buffer[16]; 
    snprintf(buffer, sizeof(buffer), "%u.%u.%u.%u.", 
            (ip_raw >> 24) & 0xFF, 
            (ip_raw >> 16) & 0xFF, 
            (ip_raw >> 8) & 0xFF, 
            (ip_raw) & 0xFF);
    return buffer; 
} 


uint32_t NetUtils::applyMask(uint32_t ip, uint8_t prefix){    
    if (prefix == 0) return 0; 

    uint32_t mask = 0xFFFFFFFF << (32 - prefix);
    return ip & mask; 
} 

std::vector<uint8_t> NetUtils::stringToMac(const std::string& mac){
    std::vector<uint8_t> bytes; 
    std::stringstream ss; 
    std::string segment; 
    while (std::getline(ss, segment, ':')){
        unsigned int byte; 
        std::stringstream bStream; 
        bStream << std::hex << segment; 
        bStream >> byte; 
        bytes.push_back(static_cast<uint8_t>(byte)); 
    }
    return bytes;
     
} 

std::string NetUtils::macToString(const std::vector<uint8_t> mac){
    std::ostringstream oss; 
    for (int i = 0; i < 6; i++){ 
        oss << std::hex 
            << std::setw(2) 
            << std::setfill('0')
            << static_cast<int>(mac.at(i));
        if ( i < 5 ) oss << ":"; 
    } 
    return oss.str(); 
    
} 

bool NetUtils::isBroadcastMac(const std::vector<uint8_t> src){
    for (int i = 0; i < 6; i++){ 
        if (src.at(i) != 0xFF){ 
            return false; 
        } 
    } 
    return true; 
}

