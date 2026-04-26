#include "../../include/core/EthernetFrame.hpp"
#include <stdexcept>

EthernetFrame::EthernetFrame() : ethertype(0x0800), fcs(0) {
    dest_mac.assign(6, 0);
    src_mac.assign(6, 0);
}

EthernetFrame::EthernetFrame(const std::vector<uint8_t>& d_mac, const std::vector<uint8_t>& s_mac, 
                             uint16_t type, const std::vector<uint8_t>& payload_data)
    : dest_mac(d_mac), src_mac(s_mac), ethertype(type), fcs(0) {
    if (dest_mac.size() != 6 || src_mac.size() != 6) {
        throw std::invalid_argument("MAC Address harus 6 byte");
    }
    set_payload(payload_data);
    calculate_and_set_fcs();
}

std::vector<uint8_t> EthernetFrame::toBytes() const {
    std::vector<uint8_t> frame;
    frame.reserve(14 + payload.size() + 4);

    ByteUtils::append_bytes(frame, dest_mac);
    ByteUtils::append_bytes(frame, src_mac);
    ByteUtils::write_u16_be(frame, ethertype);
    ByteUtils::append_bytes(frame, payload);
    ByteUtils::write_u32_be(frame, fcs);

    return frame;
}

void EthernetFrame::deserialize(const std::vector<uint8_t>& raw) {
    if (raw.size() < 18) {
        throw std::out_of_range("Data terlalu pendek untuk Ethernet Frame ber-FCS");
    }
    dest_mac.assign(raw.begin(), raw.begin() + 6);
    src_mac.assign(raw.begin() + 6, raw.begin() + 12);
    ethertype = ByteUtils::read_u16_be(raw, 12);

    size_t payload_size = raw.size() - 14 - 4;
    payload.assign(raw.begin() + 14, raw.begin() + 14 + payload_size);
    fcs = ByteUtils::read_u32_be(raw, raw.size() - 4);
}

std::string EthernetFrame::get_type() const { 
    return "ETHERNET"; 
}

void EthernetFrame::calculate_and_set_fcs() {
    std::vector<uint8_t> data_to_hash;
    ByteUtils::append_bytes(data_to_hash, dest_mac);
    ByteUtils::append_bytes(data_to_hash, src_mac);
    ByteUtils::write_u16_be(data_to_hash, ethertype);
    ByteUtils::append_bytes(data_to_hash, payload);
    fcs = compute_crc32(data_to_hash);
}

bool EthernetFrame::verify_fcs() const {
    std::vector<uint8_t> data_to_hash;
    ByteUtils::append_bytes(data_to_hash, dest_mac);
    ByteUtils::append_bytes(data_to_hash, src_mac);
    ByteUtils::write_u16_be(data_to_hash, ethertype);
    ByteUtils::append_bytes(data_to_hash, payload);
    return fcs == compute_crc32(data_to_hash);
}
uint32_t EthernetFrame::compute_crc32(const std::vector<uint8_t>& data) const {
    uint32_t crc = 0xFFFFFFFF;
    for (uint8_t byte : data) {
        crc ^= byte;
        for (int i = 0; i < 8; i++) {
            crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320 : (crc >> 1);
        }
    }
    return ~crc;
}