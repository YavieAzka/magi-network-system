#ifndef ARP_PACKET_HPP
#define ARP_PACKET_HPP

#include "../../include/core/Packet.hpp"
#include <vector>
#include <cstdint>

constexpr uint16_t ARP_OP_REQUEST = 1;
constexpr uint16_t ARP_OP_REPLY = 2;

class ARPPacket : public Packet {
public:
    uint16_t hardware_type; 
    uint16_t protocol_type;
    uint8_t hardware_len;   
    uint8_t protocol_len;   
    uint16_t opcode;       

    std::vector<uint8_t> sender_mac; 
    uint32_t sender_ip;              
    std::vector<uint8_t> target_mac; 
    uint32_t target_ip;             

    ARPPacket();
    ARPPacket(uint16_t op, 
              const std::vector<uint8_t>& smac, uint32_t sip, 
              const std::vector<uint8_t>& tmac, uint32_t tip);

    std::vector<uint8_t> toBytes() const override;
    void deserialize(const std::vector<uint8_t>& raw) override;
    std::string get_type() const override;
};

#endif