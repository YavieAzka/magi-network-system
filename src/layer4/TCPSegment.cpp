#include "../../include/layer4/TCPSegment.hpp"
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <iomanip>

TCPSegment::TCPSegment()
    : srcPort(0), dstPort(0), seqNum(0), ackNum(0),
      dataOffset(5),
      flags(0), windowSize(65535), checksum(0), urgentPointer(0)
{}

uint16_t onesComplementChecksum(const std::vector<uint8_t>& data) {
    uint32_t sum = 0;
    size_t len = data.size();
    size_t i = 0;
 
    while (i + 1 < len) {
        uint16_t word = (static_cast<uint16_t>(data[i]) << 8) | data[i + 1];
        sum += word;
        i += 2;
    }
 
    if (i < len) {
        uint16_t word = static_cast<uint16_t>(data[i]) << 8;
        sum += word;
    }
 
    while (sum >> 16) {
        sum = (sum >> 16) + (sum & 0xFFFF);
    }
 
    return static_cast<uint16_t>(~sum);
}
 
vector<uint8_t> TCPSegment::toBytes() const{
    vector<uint8_t> buf;
    buf.reserve(20+payload.size());

    // Macro helper: push uin16_t dalam big-endian
    auto pushU16 = [&](uint16_t val){
        buf.push_back((val >> 8) & 0xFF);
        buf.push_back(val & 0xFF);
    };

    // Macro helper: push uin32_t dalam big-endian
    auto pushU32 = [&](uint32_t val) {
        buf.push_back((val >> 24) & 0xFF);
        buf.push_back((val >> 16) & 0xFF);
        buf.push_back((val >> 8) & 0xFF);
        buf.push_back(val & 0xFF);
    };
     pushU16(srcPort);
    pushU16(dstPort);
    pushU32(seqNum);
    pushU32(ackNum);
 
    // Byte ke 12: bagian atas = dataOffset (ukuran header/4), bagian bawah = reserved (0)
    buf.push_back((dataOffset << 4) | 0x00);
    buf.push_back(flags);
 
    pushU16(windowSize);
    pushU16(checksum);
    pushU16(urgentPointer);
 
    // Append payload
    buf.insert(buf.end(), payload.begin(), payload.end());
 
    return buf;
}


TCPSegment TCPSegment::deserialize(const std::vector<uint8_t>& raw) {
    if (raw.size() < 20) {
        throw std::invalid_argument(
            "TCPSegment::deserialize: data terlalu pendek (" +
            std::to_string(raw.size()) + " byte, minimum 20)"
        );
    }
 
    TCPSegment seg;
 
    // Helper: baca uint16_t dari offset tertentu (big-endian)
    auto readU16 = [&](size_t offset) -> uint16_t {
        return (static_cast<uint16_t>(raw[offset]) << 8) | raw[offset + 1];
    };
 
    // Helper: baca uint32_t dari offset tertentu (big-endian)
    auto readU32 = [&](size_t offset) -> uint32_t {
        return (static_cast<uint32_t>(raw[offset])     << 24) |
               (static_cast<uint32_t>(raw[offset + 1]) << 16) |
               (static_cast<uint32_t>(raw[offset + 2]) << 8)  |
                static_cast<uint32_t>(raw[offset + 3]);
    };
 
    seg.srcPort       = readU16(0);
    seg.dstPort       = readU16(2);
    seg.seqNum        = readU32(4);
    seg.ackNum        = readU32(8);
 
    // Byte 12: nibble atas = dataOffset
    seg.dataOffset    = (raw[12] >> 4) & 0x0F;
    seg.flags         = raw[13];
 
    seg.windowSize    = readU16(14);
    seg.checksum      = readU16(16);
    seg.urgentPointer = readU16(18);
 
    // Hitung header size aktual: dataOffset * 4 byte
    size_t headerSize = seg.dataOffset * 4;
    if (headerSize > raw.size()) {
        throw std::invalid_argument("TCPSegment::deserialize: dataOffset melebihi ukuran data");
    }
 
    // Sisanya adalah payload
    if (headerSize < raw.size()) {
        seg.payload.assign(raw.begin() + headerSize, raw.end());
    }
 
    return seg;
}
 
TCPPseudoheader TCPSegment::buildPseudoHeader(
    const std::string& srcIP,
    const std::string& dstIP,
    uint16_t tcpLength
) {
    TCPPseudoheader ph;
    ph.srcIP      = NetUtils::stringToIp(srcIP);
    ph.dstIP      = NetUtils::stringToIp(dstIP);
    ph.zero       = 0;
    ph.protocol   = 6;          // Protocol number untuk TCP
    ph.tcplen  = tcpLength;  // Panjang TCP header + payload
    return ph;
}
 
void TCPSegment::calculateChecksum(const std::string& srcIP, const std::string& dstIP) {
    this->checksum = 0;
 
    vector<uint8_t> data;
 
    uint16_t tcpLength = static_cast<uint16_t>(20 + payload.size());
    TCPPseudoheader ph = buildPseudoHeader(srcIP, dstIP, tcpLength);
 
    auto pushU32 = [&](uint32_t val) {
        data.push_back((val >> 24) & 0xFF);
        data.push_back((val >> 16) & 0xFF);
        data.push_back((val >> 8)  & 0xFF);
        data.push_back(val & 0xFF);
    };
    auto pushU16 = [&](uint16_t val) {
        data.push_back((val >> 8) & 0xFF);
        data.push_back(val & 0xFF);
    };
 
    pushU32(ph.srcIP);
    pushU32(ph.dstIP);
    data.push_back(ph.zero);
    data.push_back(ph.protocol);
    pushU16(ph.tcplen);
 
    std::vector<uint8_t> segBytes = this->toBytes();
    data.insert(data.end(), segBytes.begin(), segBytes.end());
 
    this->checksum = onesComplementChecksum(data);
}
 
bool TCPSegment::verifyChecksum(const std::string& srcIP, const std::string& dstIP) const {
    std::vector<uint8_t> data;
 
    uint16_t tcpLength = static_cast<uint16_t>(20 + payload.size());
    TCPPseudoheader ph = buildPseudoHeader(srcIP, dstIP, tcpLength);
 
    auto pushU32 = [&](uint32_t val) {
        data.push_back((val >> 24) & 0xFF);
        data.push_back((val >> 16) & 0xFF);
        data.push_back((val >> 8)  & 0xFF);
        data.push_back(val & 0xFF);
    };
    auto pushU16 = [&](uint16_t val) {
        data.push_back((val >> 8) & 0xFF);
        data.push_back(val & 0xFF);
    };
 
    pushU32(ph.srcIP);
    pushU32(ph.dstIP);
    data.push_back(ph.zero);
    data.push_back(ph.protocol);
    pushU16(ph.tcplen);
 
    std::vector<uint8_t> segBytes = this->toBytes();
    data.insert(data.end(), segBytes.begin(), segBytes.end());
 
    uint16_t result = onesComplementChecksum(data);
    return result == 0x0000;
}
 
// Buat debugging aja
void TCPSegment::print() const {
    std::cout << "[TCPSegment] "
              << srcPort << " -> " << dstPort
              << " | seq=" << seqNum
              << " ack=" << ackNum
              << " flags=";
 
    if (flags & TCP_FLAG_SYN) std::cout << "SYN ";
    if (flags & TCP_FLAG_ACK) std::cout << "ACK ";
    if (flags & TCP_FLAG_FIN) std::cout << "FIN ";
    if (flags & TCP_FLAG_PSH) std::cout << "PSH ";
    if (flags & TCP_FLAG_RST) std::cout << "RST ";
 
    std::cout << "| win=" << windowSize
              << " | payload=" << payload.size() << "B"
              << " | checksum=0x" << std::hex << std::setw(4) << std::setfill('0') << checksum
              << std::dec << "\n";
}