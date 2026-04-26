#ifndef TCP_SEGMENT_HPP
#define TCP_SEGMENT_HPP

#include "../../include/core/Packet.hpp"
#include <vector>
#include <cstdint>
#include <string>


constexpr uint8_t TCP_FLAG_FIN = 0x01;
constexpr uint8_t TCP_FLAG_SYN = 0x02;
constexpr uint8_t TCP_FLAG_RST = 0x04;
constexpr uint8_t TCP_FLAG_PSH = 0x08;
constexpr uint8_t TCP_FLAG_ACK = 0x10;
constexpr uint8_t TCP_FLAG_URG = 0x20;

class TCPSegment : public Packet {
public:
    uint16_t src_port;   
    uint16_t dest_port;    
    uint32_t seq_num;      
    uint32_t ack_num;     
    uint8_t  data_offset;    
    uint8_t  flags;        
    uint16_t window_size;    
    uint16_t checksum;       
    uint16_t urgent_pointer;
    std::vector<uint8_t> options;

    TCPSegment();
    TCPSegment(uint16_t src, uint16_t dest, uint32_t seq, uint32_t ack, 
               uint8_t flg, uint16_t win, 
               const std::vector<uint8_t>& payload_data,
               const std::vector<uint8_t>& tcp_options = {});\
    std::vector<uint8_t> toBytes() const override;
    void deserialize(const std::vector<uint8_t>& raw) override;
    std::string get_type() const override;
    void calculate_and_set_checksum(uint32_t src_ip, uint32_t dest_ip);
};

#endif