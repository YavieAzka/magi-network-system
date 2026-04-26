#include "../../include/core/ICMPPacket.hpp"
#include <stdexcept>

ICMPPacket::ICMPPacket() 
    : type(0), code(0), checksum(0), identifier(0), sequence_number(0) {
}

ICMPPacket::ICMPPacket(uint8_t type, uint8_t code, uint16_t id, uint16_t seq, const std::vector<uint8_t>& payload_data)
    : type(type), code(code), checksum(0), identifier(id), sequence_number(seq) {
    set_payload(payload_data);
    calculate_and_set_checksum();
}

std::vector<uint8_t> ICMPPacket::toBytes() const {
    std::vector<uint8_t> packet;
    packet.reserve(8 + payload.size());
    packet.push_back(type);
    packet.push_back(code);
    ByteUtils::write_u16_be(packet, checksum);
    ByteUtils::write_u16_be(packet, identifier);
    ByteUtils::write_u16_be(packet, sequence_number);
    ByteUtils::append_bytes(packet, payload);

    return packet;
}

void ICMPPacket::deserialize(const std::vector<uint8_t>& raw) {
    if (raw.size() < 8) {
        throw std::out_of_range("Data terlalu pendek untuk ICMP Packet (min. 8 byte)");
    }

    type            = raw[0];
    code            = raw[1];
    checksum        = ByteUtils::read_u16_be(raw, 2);
    identifier      = ByteUtils::read_u16_be(raw, 4);
    sequence_number = ByteUtils::read_u16_be(raw, 6);
    payload.assign(raw.begin() + 8, raw.end());
}

std::string ICMPPacket::get_type() const {
    return "ICMP";
}

void ICMPPacket::calculate_and_set_checksum() {
    checksum = 0; 
    std::vector<uint8_t> full_packet;
    full_packet.push_back(type);
    full_packet.push_back(code);
    ByteUtils::write_u16_be(full_packet, checksum);
    ByteUtils::write_u16_be(full_packet, identifier);
    ByteUtils::write_u16_be(full_packet, sequence_number);
    ByteUtils::append_bytes(full_packet, payload);

    checksum = compute_checksum(full_packet);
}

uint16_t ICMPPacket::compute_checksum(const std::vector<uint8_t>& data) const {
    uint32_t sum = 0;

    for (size_t i = 0; i < data.size() - 1; i += 2) {
        uint16_t word = (data[i] << 8) | data[i + 1];
        sum += word;
    }
    if (data.size() % 2 != 0) {
        sum += (data.back() << 8);
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return static_cast<uint16_t>(~sum);
}