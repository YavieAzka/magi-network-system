#ifndef ICMP_PACKET_HPP
#define ICMP_PACKET_HPP

#include "../../include/core/Packet.hpp"
#include <vector>
#include <cstdint>
#include <string>

constexpr uint8_t ICMP_TYPE_ECHO_REPLY       = 0;
constexpr uint8_t ICMP_TYPE_DEST_UNREACHABLE = 3;
constexpr uint8_t ICMP_TYPE_ECHO_REQUEST     = 8;
constexpr uint8_t ICMP_TYPE_TIME_EXCEEDED    = 11;

class ICMPPacket : public Packet {
public:
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t identifier;
    uint16_t sequence_number;

    ICMPPacket();
    ICMPPacket(uint8_t type, uint8_t code, uint16_t id, uint16_t seq, const std::vector<uint8_t>& payload_data);
    std::vector<uint8_t> toBytes() const override;
    void deserialize(const std::vector<uint8_t>& raw) override;
    std::string get_type() const override;
    void calculate_and_set_checksum();

private:
    uint16_t compute_checksum(const std::vector<uint8_t>& data) const;
};

#endif