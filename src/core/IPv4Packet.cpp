#include "IPv4Packet.hpp"
#include <stdexcept>

IPv4Packet::IPv4Packet() 
    : version_ihl(0x45), tos(0), total_length(20), identification(0),
      flags_fragment(0), ttl(64), protocol(0), header_checksum(0),
      src_ip(0), dest_ip(0) {
}

IPv4Packet::IPv4Packet(uint8_t prot, uint32_t src, uint32_t dest, 
                       const std::vector<uint8_t>& payload_data,
                       const std::vector<uint8_t>& ip_options)
    : tos(0), identification(0), flags_fragment(0), ttl(64), 
      protocol(prot), header_checksum(0), src_ip(src), dest_ip(dest), options(ip_options) {
    
    while (options.size() % 4 != 0) {
        options.push_back(0x00); 
    }

    uint8_t ihl = 5 + static_cast<uint8_t>(options.size() / 4);
    if (ihl > 15) throw std::runtime_error("IP Options terlalu besar (IHL max 15)");
    
    version_ihl = (0x4 << 4) | (ihl & 0x0F);
    set_payload(payload_data);
    total_length = (ihl * 4) + static_cast<uint16_t>(payload.size());

    calculate_and_set_checksum();
}

std::vector<uint8_t> IPv4Packet::toBytes() const {
    std::vector<uint8_t> packet;
    packet.reserve(total_length);

    packet.push_back(version_ihl);
    packet.push_back(tos);
    ByteUtils::write_u16_be(packet, total_length);
    ByteUtils::write_u16_be(packet, identification);
    ByteUtils::write_u16_be(packet, flags_fragment);
    packet.push_back(ttl);
    packet.push_back(protocol);
    ByteUtils::write_u16_be(packet, header_checksum);
    ByteUtils::write_u32_be(packet, src_ip);
    ByteUtils::write_u32_be(packet, dest_ip);

    if (!options.empty()) {
        ByteUtils::append_bytes(packet, options);
    }

    ByteUtils::append_bytes(packet, payload);

    return packet;
}

void IPv4Packet::deserialize(const std::vector<uint8_t>& raw) {
    if (raw.size() < 20) {
        throw std::out_of_range("Data terlalu pendek untuk IPv4 Header");
    }

    version_ihl = raw[0];
    uint8_t ihl = version_ihl & 0x0F;
    size_t header_bytes = ihl * 4;

    if (raw.size() < header_bytes) {
        throw std::out_of_range("Data corrupt: IHL lebih besar dari ukuran data mentah");
    }

    tos            = raw[1];
    total_length   = ByteUtils::read_u16_be(raw, 2);
    identification = ByteUtils::read_u16_be(raw, 4);
    flags_fragment = ByteUtils::read_u16_be(raw, 6);
    ttl            = raw[8];
    protocol       = raw[9];
    header_checksum = ByteUtils::read_u16_be(raw, 10);
    src_ip         = ByteUtils::read_u32_be(raw, 12);
    dest_ip        = ByteUtils::read_u32_be(raw, 16);

    options.clear();
    if (header_bytes > 20) {
        options.assign(raw.begin() + 20, raw.begin() + header_bytes);
    }

    if (raw.size() < total_length) {
         payload.assign(raw.begin() + header_bytes, raw.end());
    } else {
         payload.assign(raw.begin() + header_bytes, raw.begin() + total_length);
    }
}

std::string IPv4Packet::get_type() const {
    return "IPv4";
}

void IPv4Packet::calculate_and_set_checksum() {
    header_checksum = 0;
    
    std::vector<uint8_t> header_only;
    header_only.push_back(version_ihl);
    header_only.push_back(tos);
    ByteUtils::write_u16_be(header_only, total_length);
    ByteUtils::write_u16_be(header_only, identification);
    ByteUtils::write_u16_be(header_only, flags_fragment);
    header_only.push_back(ttl);
    header_only.push_back(protocol);
    ByteUtils::write_u16_be(header_only, header_checksum);
    ByteUtils::write_u32_be(header_only, src_ip);
    ByteUtils::write_u32_be(header_only, dest_ip);
    
    if (!options.empty()) {
        ByteUtils::append_bytes(header_only, options);
    }

    header_checksum = compute_checksum(header_only);
}

uint16_t IPv4Packet::compute_checksum(const std::vector<uint8_t>& data) const {
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