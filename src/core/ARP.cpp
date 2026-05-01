#include "core/ARP.hpp"

std::vector<uint8_t> ARPPacket::toBytes() const {
    std::vector<uint8_t> bytes;
    bytes.reserve(28); 

    ByteUtils::write_u16_be(bytes, hardware_type);
    ByteUtils::write_u16_be(bytes, protocol_type);
    bytes.push_back(hardware_len);
    bytes.push_back(protocol_len);
    ByteUtils::write_u16_be(bytes, operation);

    bytes.insert(bytes.end(), sender_hw_addr.begin(), sender_hw_addr.end());
    ByteUtils::write_u32_be(bytes, sender_prot_addr);
    bytes.insert(bytes.end(), target_hw_addr.begin(), target_hw_addr.end());
    ByteUtils::write_u32_be(bytes, target_prot_addr);

    return bytes;
}

bool ARPPacket::parseToARP(std::vector<uint8_t>& data){
    if (data.size() < 28) {
        return false;
    }

    hardware_type = ByteUtils::read_u16_be(data, 0); 
    protocol_type = ByteUtils::read_u16_be(data, 2); 
    hardware_len = data[4]; 
    protocol_len = data[5]; 
    operation = ByteUtils::read_u16_be(data, 6);

    for (size_t i = 8; i < 14; ++i) {
        sender_hw_addr[i - 8] = data[i];
    }

    sender_prot_addr = ByteUtils::read_u32_be(data, 14);

    for (size_t i = 18; i < 24; ++i) {
        target_hw_addr[i - 18] = data[i];
    }

    target_prot_addr = ByteUtils::read_u32_be(data, 24);
    return true;
}

void ARPPacket::buildRequest(){
    operation = 1;
}

void ARPPacket::buildReply(){
    operation = 2;
}

bool ARPCache::checkIpInCache(std::string ip) const{
    auto it = arpcache.find(ip);
    if(it != arpcache.end()){
        return true;
    }
    else{
        return false;
    }
}

void ARPCache::addIpToCache(std::string ip, std::string mac){
    arpcache[ip] = mac;
}

void ARPCache::clearCache(){
    arpcache.clear();
}

std::string ARPCache::getMac(string ip){
    return arpcache[ip];
}

void ARPCache::clearQueue(){
    ipPacketQueue.clear();
}

void ARPCache::addPacketToCache(std::string ip, std::vector<uint8_t> packet){
    ipPacketQueue[ip].push_back(packet);
}

std::vector<std::vector<uint8_t>> ARPCache::getAndClearQueue(std::string ip){
    vector<std::vector<uint8_t>> ret = ipPacketQueue[ip];
    ipPacketQueue.erase(ip);
    return ret;
}