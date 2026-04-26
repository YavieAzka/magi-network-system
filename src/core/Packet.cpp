#include "../../include/core/Packet.hpp"

Packet::Packet(vector<uint8_t> data) : payload(std::move(data)) {}


void Packet::set_payload(const vector<uint8_t>& data){
    payload = data;
}
void Packet::set_payload_from_packet(const Packet& inner_packet){
    payload = inner_packet.toBytes();
}

const vector<uint8_t>& Packet::get_payload_bytes() const{
    return payload;
}


size_t Packet::total_size() const {
    return payload.size();
}

RawPacket::RawPacket(std::vector<uint8_t> data) : Packet(std::move(data)) {}
RawPacket::RawPacket(const std::string& text) {
    payload = std::vector<uint8_t>(text.begin(), text.end());
}

vector<uint8_t> RawPacket::toBytes() const {
    // RawPacket tidak punya header, langsung return payload
    return payload;
}

void RawPacket::deserialize(const std::vector<uint8_t>& raw)  {
    payload = raw;
}

string RawPacket::get_type() const  {
    return "RAW";
}

// Utility: ambil payload sebagai string (untuk display)
string RawPacket::as_string() const {
    return std::string(payload.begin(), payload.end());
}

