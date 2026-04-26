#include "../../include/core/ARPPacket.hpp"
#include <stdexcept>

ARPPacket::ARPPacket() 
    : hardware_type(1), protocol_type(0x0800), 
      hardware_len(6), protocol_len(4), opcode(0),
      sender_ip(0), target_ip(0) {
    sender_mac.assign(6, 0);
    target_mac.assign(6, 0);
}

ARPPacket::ARPPacket(uint16_t op, 
                     const std::vector<uint8_t>& smac, uint32_t sip, 
                     const std::vector<uint8_t>& tmac, uint32_t tip)
    : hardware_type(1), protocol_type(0x0800), 
      hardware_len(6), protocol_len(4), opcode(op),
      sender_mac(smac), sender_ip(sip), 
      target_mac(tmac), target_ip(tip) {
    
    if (sender_mac.size() != 6 || target_mac.size() != 6) {
        throw std::invalid_argument("MAC Address harus 6 byte");
    }
}

std::vector<uint8_t> ARPPacket::toBytes() const {
    std::vector<uint8_t> packet;
    packet.reserve(28);

    ByteUtils::write_u16_be(packet, hardware_type);
    ByteUtils::write_u16_be(packet, protocol_type);
    packet.push_back(hardware_len);
    packet.push_back(protocol_len);
    ByteUtils::write_u16_be(packet, opcode);
    ByteUtils::append_bytes(packet, sender_mac);
    ByteUtils::write_u32_be(packet, sender_ip);
    ByteUtils::append_bytes(packet, target_mac);
    ByteUtils::write_u32_be(packet, target_ip);

    return packet;
}

void ARPPacket::deserialize(const std::vector<uint8_t>& raw) {
    if (raw.size() < 28) {
        throw std::out_of_range("Data terlalu pendek untuk ARP Packet (min. 28 byte)");
    }

    hardware_type = ByteUtils::read_u16_be(raw, 0);
    protocol_type = ByteUtils::read_u16_be(raw, 2);
    hardware_len  = raw[4];
    protocol_len  = raw[5];
    opcode        = ByteUtils::read_u16_be(raw, 6);

    sender_mac.assign(raw.begin() + 8, raw.begin() + 14);
    sender_ip     = ByteUtils::read_u32_be(raw, 14);

    target_mac.assign(raw.begin() + 18, raw.begin() + 24);
    target_ip     = ByteUtils::read_u32_be(raw, 24);
}

std::string ARPPacket::get_type() const {
    return "ARP";
}