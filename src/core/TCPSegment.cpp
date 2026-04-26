#include "../../include/core/TCPSegment.hpp"
#include <stdexcept>

TCPSegment::TCPSegment() 
    : src_port(0), dest_port(0), seq_num(0), ack_num(0), 
      data_offset(5), flags(0), window_size(8192), checksum(0), urgent_pointer(0) {
}

TCPSegment::TCPSegment(uint16_t src, uint16_t dest, uint32_t seq, uint32_t ack, 
                       uint8_t flg, uint16_t win, 
                       const std::vector<uint8_t>& payload_data,
                       const std::vector<uint8_t>& tcp_options)
    : src_port(src), dest_port(dest), seq_num(seq), ack_num(ack),
      flags(flg), window_size(win), checksum(0), urgent_pointer(0), options(tcp_options) {
    while (options.size() % 4 != 0) {
        options.push_back(0x00);
    }
    data_offset = 5 + static_cast<uint8_t>(options.size() / 4);
    if (data_offset > 15) {
        throw std::runtime_error("TCP Options terlalu besar (Max data_offset 15)");
    }

    set_payload(payload_data);
}

std::vector<uint8_t> TCPSegment::toBytes() const {
    std::vector<uint8_t> segment;
    segment.reserve((data_offset * 4) + payload.size());
    ByteUtils::write_u16_be(segment, src_port);
    ByteUtils::write_u16_be(segment, dest_port);
    ByteUtils::write_u32_be(segment, seq_num);
    ByteUtils::write_u32_be(segment, ack_num);
    uint8_t offset_res = (data_offset << 4) & 0xF0; 
    segment.push_back(offset_res);
    segment.push_back(flags);

    ByteUtils::write_u16_be(segment, window_size);
    ByteUtils::write_u16_be(segment, checksum);
    ByteUtils::write_u16_be(segment, urgent_pointer);
    if (!options.empty()) {
        ByteUtils::append_bytes(segment, options);
    }
    ByteUtils::append_bytes(segment, payload);

    return segment;
}

void TCPSegment::deserialize(const std::vector<uint8_t>& raw) {
    if (raw.size() < 20) {
        throw std::out_of_range("Data terlalu pendek untuk TCP Segment (min. 20 byte)");
    }

    src_port       = ByteUtils::read_u16_be(raw, 0);
    dest_port      = ByteUtils::read_u16_be(raw, 2);
    seq_num        = ByteUtils::read_u32_be(raw, 4);
    ack_num        = ByteUtils::read_u32_be(raw, 8);
    data_offset    = (raw[12] >> 4) & 0x0F;
    flags          = raw[13];
    window_size    = ByteUtils::read_u16_be(raw, 14);
    checksum       = ByteUtils::read_u16_be(raw, 16);
    urgent_pointer = ByteUtils::read_u16_be(raw, 18);

    size_t header_bytes = data_offset * 4;
    
    if (raw.size() < header_bytes) {
        throw std::out_of_range("Data corrupt: panjang header TCP (Data Offset) melebihi ukuran raw data");
    }
    options.clear();
    if (header_bytes > 20) {
        options.assign(raw.begin() + 20, raw.begin() + header_bytes);
    }
    size_t actual_payload_size = raw.size() - header_bytes;
    payload.assign(raw.begin() + header_bytes, raw.begin() + header_bytes + actual_payload_size);
}

std::string TCPSegment::get_type() const {
    return "TCP";
}

void TCPSegment::calculate_and_set_checksum(uint32_t src_ip, uint32_t dest_ip) {
    checksum = 0; 
    std::vector<uint8_t> tcp_data = toBytes(); 

    uint32_t sum = 0;

    sum += (src_ip >> 16) & 0xFFFF;
    sum += src_ip & 0xFFFF;
    sum += (dest_ip >> 16) & 0xFFFF;
    sum += dest_ip & 0xFFFF;
    sum += 0x0006;
    sum += static_cast<uint16_t>(tcp_data.size());

    for (size_t i = 0; i < tcp_data.size() - 1; i += 2) {
        uint16_t word = (tcp_data[i] << 8) | tcp_data[i + 1];
        sum += word;
    }
    if (tcp_data.size() % 2 != 0) {
        sum += (tcp_data.back() << 8);
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    checksum = static_cast<uint16_t>(~sum);
}