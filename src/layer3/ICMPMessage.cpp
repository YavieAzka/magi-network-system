#include "layer3/ICMPMessage.hpp"

ICMPMessage::ICMPMessage()
    : type(0), code(0), checksum(0) {}

ICMPMessage::ICMPMessage(uint8_t type, uint8_t code, std::vector<uint8_t> icmpPayload)
    : type(type), code(code), checksum(0), icmpPayload(std::move(icmpPayload)) {}

ICMPMessage::~ICMPMessage() = default;

uint16_t ICMPMessage::calculateChecksum() const
{
    std::vector<uint8_t> buf;
    buf.reserve(4 + icmpPayload.size());

    buf.push_back(type);
    buf.push_back(code);
    buf.push_back(0x00);
    buf.push_back(0x00);
    buf.insert(buf.end(), icmpPayload.begin(), icmpPayload.end());

    uint32_t sum = 0;

    for (size_t i = 0; i + 1 < buf.size(); i += 2)
    {
        uint16_t word = (static_cast<uint16_t>(buf[i]) << 8) | buf[i + 1];
        sum += word;
    }

    if (buf.size() % 2 != 0)
    {
        sum += static_cast<uint16_t>(buf.back()) << 8;
    }

    while (sum >> 16)
    {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return static_cast<uint16_t>(~sum);
}

bool ICMPMessage::verifyChecksum() const
{
    std::vector<uint8_t> buf;
    buf.reserve(4 + icmpPayload.size());

    buf.push_back(type);
    buf.push_back(code);
    ByteUtils::write_u16_be(buf, checksum); // masukkan checksum asli
    buf.insert(buf.end(), icmpPayload.begin(), icmpPayload.end());

    uint32_t sum = 0;

    for (size_t i = 0; i + 1 < buf.size(); i += 2)
    {
        uint16_t word = (static_cast<uint16_t>(buf[i]) << 8) | buf[i + 1];
        sum += word;
    }

    if (buf.size() % 2 != 0)
    {
        sum += static_cast<uint16_t>(buf.back()) << 8;
    }

    while (sum >> 16)
    {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return static_cast<uint16_t>(sum) == 0xFFFF;
}

bool ICMPMessage::isChecksumValid() const
{
    return verifyChecksum();
}

uint16_t ICMPMessage::getIdentifier() const
{
    if (icmpPayload.size() < 2)
        return 0;
    return ByteUtils::read_u16_be(icmpPayload, 0);
}

uint16_t ICMPMessage::getSequenceNumber() const
{
    if (icmpPayload.size() < 4)
        return 0;
    return ByteUtils::read_u16_be(icmpPayload, 2);
}

std::vector<uint8_t> ICMPMessage::getEchoData() const
{
    if (icmpPayload.size() <= 4)
        return {};
    return std::vector<uint8_t>(icmpPayload.begin() + 4, icmpPayload.end());
}

std::vector<uint8_t> ICMPMessage::toBytes() const
{
    std::vector<uint8_t> buf;
    buf.reserve(4 + icmpPayload.size());

    buf.push_back(type);
    buf.push_back(code);

    uint16_t cs = calculateChecksum();
    ByteUtils::write_u16_be(buf, cs);

    buf.insert(buf.end(), icmpPayload.begin(), icmpPayload.end());

    return buf;
}

void ICMPMessage::deserialize(const std::vector<uint8_t> &raw)
{
    if (raw.size() < 4)
    {
        throw std::invalid_argument(
            "ICMPMessage::deserialize: data terlalu pendek, minimal 4 byte, dapat " +
            std::to_string(raw.size()) + " byte");
    }

    type = raw[0];
    code = raw[1];
    checksum = ByteUtils::read_u16_be(raw, 2);

    icmpPayload.assign(raw.begin() + 4, raw.end());
}

ICMPMessage ICMPMessage::buildEchoRequest(uint16_t identifier,
                                          uint16_t sequenceNum,
                                          const std::vector<uint8_t> &data)
{
    std::vector<uint8_t> pl;
    pl.reserve(4 + data.size());

    ByteUtils::write_u16_be(pl, identifier);
    ByteUtils::write_u16_be(pl, sequenceNum);
    pl.insert(pl.end(), data.begin(), data.end());

    return ICMPMessage(ICMP::TYPE_ECHO_REQUEST, ICMP::CODE_DEFAULT, std::move(pl));
}

ICMPMessage ICMPMessage::buildEchoReply(const ICMPMessage &request)
{
    if (request.getICMPType() != ICMP::TYPE_ECHO_REQUEST)
    {
        throw std::invalid_argument(
            "ICMPMessage::buildEchoReply: input bukan Echo Request (type=" +
            std::to_string(request.getICMPType()) + ")");
    }

    return ICMPMessage(ICMP::TYPE_ECHO_REPLY, ICMP::CODE_DEFAULT, request.getICMPPayload());
}

ICMPMessage ICMPMessage::buildTimeExceeded(const std::vector<uint8_t> &originalHeader)
{
    std::vector<uint8_t> pl;
    pl.reserve(4 + originalHeader.size());

    pl.push_back(0x00);
    pl.push_back(0x00);
    pl.push_back(0x00);
    pl.push_back(0x00);

    pl.insert(pl.end(), originalHeader.begin(), originalHeader.end());

    return ICMPMessage(ICMP::TYPE_TIME_EXCEEDED, ICMP::CODE_TTL_EXCEEDED, std::move(pl));
}

ICMPMessage ICMPMessage::buildUnreachable(const std::vector<uint8_t> &originalHeader,
                                          uint8_t code)
{
    std::vector<uint8_t> pl;
    pl.reserve(4 + originalHeader.size());

    pl.push_back(0x00);
    pl.push_back(0x00);
    pl.push_back(0x00);
    pl.push_back(0x00);

    pl.insert(pl.end(), originalHeader.begin(), originalHeader.end());

    return ICMPMessage(ICMP::TYPE_UNREACHABLE, code, std::move(pl));
}