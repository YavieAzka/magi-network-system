#ifndef IPV4_PACKET_HPP
#define IPV4_PACKET_HPP

#include "../../include/core/Packet.hpp"
#include <vector>
#include <cstdint>
#include <string>

class IPv4Packet : public Packet {
public:
    uint8_t  version_ihl;     
    uint8_t  tos;              
    uint16_t total_length;     
    uint16_t identification;
    uint16_t flags_fragment;  
    uint8_t  ttl;          
    uint8_t  protocol;       
    uint16_t header_checksum;
    uint32_t src_ip;
    uint32_t dest_ip;
    std::vector<uint8_t> options;

    IPv4Packet();
    IPv4Packet(uint8_t prot, uint32_t src, uint32_t dest, 
               const std::vector<uint8_t>& payload_data,
               const std::vector<uint8_t>& ip_options = {});
    std::vector<uint8_t> toBytes() const override;
    void deserialize(const std::vector<uint8_t>& raw) override;
    std::string get_type() const override;
    void calculate_and_set_checksum();

private:
    uint16_t compute_checksum(const std::vector<uint8_t>& header_data) const;
};

#endif