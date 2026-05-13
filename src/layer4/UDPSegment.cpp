#include "layer4/UDPSegment.hpp"
#include <stdexcept>
#include <sstream>

static std::vector<uint8_t> ipToBytes(const std::string &ip)
{
    std::vector<uint8_t> result;
    std::stringstream ss(ip);
    std::string octet;
    while (std::getline(ss, octet, '.'))
    {
        result.push_back(static_cast<uint8_t>(std::stoi(octet)));
    }
    if (result.size() != 4)
        throw std::invalid_argument("Invalid IP: " + ip);
    return result;
}

static uint16_t onesComplementSum(const std::vector<uint8_t> &data)
{
    uint32_t sum = 0;
    size_t i = 0;

    for (; i + 1 < data.size(); i += 2)
    {
        uint16_t word = (static_cast<uint16_t>(data[i]) << 8) | data[i + 1];
        sum += word;
    }

    if (i < data.size())
    {
        uint16_t word = static_cast<uint16_t>(data[i]) << 8;
        sum += word;
    }

    while (sum >> 16)
    {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return static_cast<uint16_t>(~sum);
}

std::vector<uint8_t> UDPSegment::toBytes() const
{
    std::vector<uint8_t> buf;
    buf.reserve(8 + payload.size());

    buf.push_back((srcPort >> 8) & 0xFF);
    buf.push_back(srcPort & 0xFF);

    buf.push_back((dstPort >> 8) & 0xFF);
    buf.push_back(dstPort & 0xFF);

    buf.push_back((length >> 8) & 0xFF);
    buf.push_back(length & 0xFF);

    buf.push_back((checksum >> 8) & 0xFF);
    buf.push_back(checksum & 0xFF);

    buf.insert(buf.end(), payload.begin(), payload.end());

    return buf;
}

UDPSegment UDPSegment::deserialize(const std::vector<uint8_t> &raw)
{
    if (raw.size() < 8)
    {
        throw std::invalid_argument("UDP segment terlalu pendek");
    }

    UDPSegment seg;
    seg.srcPort = (static_cast<uint16_t>(raw[0]) << 8) | raw[1];
    seg.dstPort = (static_cast<uint16_t>(raw[2]) << 8) | raw[3];
    seg.length = (static_cast<uint16_t>(raw[4]) << 8) | raw[5];
    seg.checksum = (static_cast<uint16_t>(raw[6]) << 8) | raw[7];

    if (raw.size() > 8)
    {
        seg.payload.assign(raw.begin() + 8, raw.end());
    }

    return seg;
}

std::vector<uint8_t> UDPSegment::buildPseudoHeader(
    const std::string &srcIP,
    const std::string &dstIP,
    uint16_t udpLength)
{
    std::vector<uint8_t> pseudo;
    pseudo.reserve(12);

    auto src = ipToBytes(srcIP);
    pseudo.insert(pseudo.end(), src.begin(), src.end());

    auto dst = ipToBytes(dstIP);
    pseudo.insert(pseudo.end(), dst.begin(), dst.end());

    pseudo.push_back(0x00);

    pseudo.push_back(0x11);

    pseudo.push_back((udpLength >> 8) & 0xFF);
    pseudo.push_back(udpLength & 0xFF);

    return pseudo;
}

uint16_t UDPSegment::calculateChecksum(
    const std::string &srcIP,
    const std::string &dstIP,
    const UDPSegment &seg)
{
    uint16_t udpLength = static_cast<uint16_t>(8 + seg.payload.size());

    UDPSegment copy = seg;
    copy.checksum = 0;
    copy.length = udpLength;
    std::vector<uint8_t> data = buildPseudoHeader(srcIP, dstIP, udpLength);
    std::vector<uint8_t> segBytes = copy.toBytes();
    data.insert(data.end(), segBytes.begin(), segBytes.end());

    return onesComplementSum(data);
}

bool UDPSegment::verifyChecksum(
    const std::string &srcIP,
    const std::string &dstIP) const
{
    if (checksum == 0x0000)
        return true;

    uint16_t expected = calculateChecksum(srcIP, dstIP, *this);
    return expected == checksum;
}