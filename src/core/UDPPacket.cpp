#include "../../include/core/UDPPacket.hpp"
#include <stdexcept>

UDPPacket::UDPPacket() 
    : src_port(0), dest_port(0), length(8), checksum(0) {
}

UDPPacket::UDPPacket(uint16_t src, uint16_t dest, const std::vector<uint8_t>& payload_data)
    : src_port(src), dest_port(dest), checksum(0) {
    
    set_payload(payload_data);
    length = 8 + static_cast<uint16_t>(payload.size());
}

std::vector<uint8_t> UDPPacket::toBytes() const {
    std::vector<uint8_t> packet;
    packet.reserve(length);

    ByteUtils::write_u16_be(packet, src_port);
    ByteUtils::write_u16_be(packet, dest_port);
    ByteUtils::write_u16_be(packet, length);
    ByteUtils::write_u16_be(packet, checksum);
    ByteUtils::append_bytes(packet, payload);

    return packet;
}

void UDPPacket::deserialize(const std::vector<uint8_t>& raw) {
    if (raw.size() < 8) {
        throw std::out_of_range("Data terlalu pendek untuk UDP Packet (min. 8 byte)");
    }

    src_port  = ByteUtils::read_u16_be(raw, 0);
    dest_port = ByteUtils::read_u16_be(raw, 2);
    length    = ByteUtils::read_u16_be(raw, 4);
    checksum  = ByteUtils::read_u16_be(raw, 6);

    if (raw.size() < length) {
        throw std::out_of_range("Data corrupt: panjang payload UDP tidak sesuai dengan field length");
    }

    size_t actual_payload_size = length - 8;
    payload.assign(raw.begin() + 8, raw.begin() + 8 + actual_payload_size);
}

std::string UDPPacket::get_type() const {
    return "UDP";
}

void UDPPacket::calculate_and_set_checksum(uint32_t src_ip, uint32_t dest_ip) {
    checksum = 0; 
    std::vector<uint8_t> udp_data = toBytes();

    uint32_t sum = 0;
    sum += (src_ip >> 16) & 0xFFFF;
    sum += src_ip & 0xFFFF;
    sum += (dest_ip >> 16) & 0xFFFF;
    sum += dest_ip & 0xFFFF;
    sum += 0x0011;
    sum += length;
    
    for (size_t i = 0; i < udp_data.size() - 1; i += 2) {
        uint16_t word = (udp_data[i] << 8) | udp_data[i + 1];
        sum += word;
    }
    
    if (udp_data.size() % 2 != 0) {
        sum += (udp_data.back() << 8);
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    uint16_t final_checksum = static_cast<uint16_t>(~sum);
    checksum = (final_checksum == 0) ? 0xFFFF : final_checksum;
}