#ifndef UDP_PACKET_HPP
#define UDP_PACKET_HPP

#include "../../include/core/Packet.hpp"
#include <vector>
#include <cstdint>
#include <string>

class UDPPacket : public Packet {
public:
    uint16_t src_port;  
    uint16_t dest_port; 
    uint16_t length;    
    uint16_t checksum;  

    UDPPacket();
    UDPPacket(uint16_t src, uint16_t dest, const std::vector<uint8_t>& payload_data);
    std::vector<uint8_t> toBytes() const override;
    void deserialize(const std::vector<uint8_t>& raw) override;
    std::string get_type() const override;
    void calculate_and_set_checksum(uint32_t src_ip, uint32_t dest_ip);
};

#endif