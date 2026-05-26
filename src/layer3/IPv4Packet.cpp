#include "../../include/layer3/IPv4Packet.hpp"
#include "../../include/util/NetUtils.hpp"

IPv4Packet::IPv4Packet(const std::string &_src, const std::string &_dst,
                       std::uint8_t _protocol, std::uint8_t _ttl,
                       const std::vector<std::uint8_t> &_payload)
{
    this->ttl = _ttl;
    this->protocol = _protocol;
    this->srcIP = _src;
    this->dstIP = _dst;
    this->payload = _payload;
    this->checksum = this->calculateChecksum();
}

std::vector<std::uint8_t> IPv4Packet::toBytes() const
{
    std::vector<uint8_t> buf(20, 0);
    buf[0] = (this->version << 4) | this->ihl;
    buf[1] = (this->tos);

    buf[2] = 0;
    buf[3] = 0;

    buf[4] = (this->identification >> 8) & 0xFF;
    buf[5] = this->identification & 0xFF;

    buf[6] = (this->flagsFragment >> 8) & 0xFF;
    buf[7] = (this->flagsFragment & 0xFF);

    buf[8] = this->ttl;
    buf[9] = this->protocol;
    buf[10] = (this->checksum >> 8) & 0xFF;
    buf[11] = this->checksum & 0xFF;

    uint32_t byteIPv4Src = NetUtils::stringToIp(srcIP);
    buf[12] = (byteIPv4Src >> 24) & 0xFF;
    buf[13] = (byteIPv4Src >> 16) & 0xFF;
    buf[14] = (byteIPv4Src >> 8) & 0xFF;
    buf[15] = (byteIPv4Src) & 0xFF;

    uint32_t byteIPv4Dst = NetUtils::stringToIp(dstIP);
    buf[16] = (byteIPv4Dst >> 24) & 0xFF;
    buf[17] = (byteIPv4Dst >> 16) & 0xFF;
    buf[18] = (byteIPv4Dst >> 8) & 0xFF;
    buf[19] = (byteIPv4Dst) & 0xFF;

    buf.insert(buf.end(), payload.begin(), payload.end());

    uint16_t len = static_cast<uint16_t>(buf.size());
    buf[2] = (len >> 8) & 0xFF;
    buf[3] = len & 0xFF;

    return buf;
}

void IPv4Packet::deserialize(const std::vector<std::uint8_t> &raw)
{
    if (raw.size() < 20)
        throw runtime_error("IPv4: packet too short");
    version = raw[0] >> 4 & 0x0F;
    ihl = raw[0] & 0x0F;
    tos = raw[1];
    totalLen = ((uint16_t)raw[2] << 8) | raw[3];
    identification = ((uint16_t)raw[4] << 8) | raw[5];
    flagsFragment = ((uint16_t)raw[6] << 8) | raw[7];
    ttl = raw[8];
    protocol = raw[9];
    checksum = ((uint16_t)raw[10] << 8) | raw[11];
    uint32_t srcRaw = ((uint32_t)raw[12] << 24) |
                      ((uint32_t)raw[13] << 16) |
                      ((uint32_t)raw[14] << 8) |
                      ((uint32_t)raw[15]);
    srcIP = NetUtils::ipToString(srcRaw);

    uint32_t dstRaw = ((uint32_t)raw[16] << 24) |
                      ((uint32_t)raw[17] << 16) |
                      ((uint32_t)raw[18] << 8) |
                      ((uint32_t)raw[19]);
    dstIP = NetUtils::ipToString(dstRaw);
    payload.assign(raw.begin() + 20, raw.end());
}

uint16_t IPv4Packet::calculateChecksum() const
{
    vector<uint8_t> buf = toBytes();
    buf[10] = 0;
    buf[11] = 0;

    uint32_t sum = 0;
    for (int i = 0; i < 20; i += 2)
    {
        uint16_t word = ((uint16_t)buf[i] << 8) | buf[i + 1];
        sum += word;
    }

    while (sum >> 16)
    {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return ~sum & 0xFFFF;
}

bool IPv4Packet::verifyChecksum() const
{
    vector<uint8_t> buf = toBytes();

    uint32_t sum = 0;
    for (int i = 0; i < 20; i += 2)
    {
        uint16_t word = ((uint16_t)buf[i] << 8) | buf[i + 1];
        sum += word;
    }

    while (sum >> 16)
    {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (sum & 0xFFFF) == 0xFFFF;
}

void IPv4Packet::decrementTTL()
{
    ttl--;
    checksum = calculateChecksum();
}

bool IPv4Packet::isFragment() const
{
    return isMoreFragments() || getFragmentOffset() > 0;
}

bool IPv4Packet::isMoreFragments() const
{
    return (flagsFragment & FLAG_MF) != 0;
}

bool IPv4Packet::isDontFragment() const
{
    return (flagsFragment & FLAG_DF) != 0;
}

uint16_t IPv4Packet::getFragmentOffset() const
{
    return (flagsFragment & FRAGMENT_OFFSET_MASK) * 8;
}

std::vector<IPv4Packet> IPv4Packet::fragment(uint16_t pathMTU) const
{
    std::vector<IPv4Packet> result;

    uint16_t headerLen = static_cast<uint16_t>(ihl) * 4;
    uint16_t maxData = pathMTU - headerLen;
    if (maxData < 8)
        return result;

    // round down to multiple of 8
    maxData &= 0xFFF8U;

    uint16_t ipPktSize = static_cast<uint16_t>(toBytes().size());
    if (ipPktSize <= pathMTU)
    {
        result.push_back(*this);
        return result;
    }

    if (isDontFragment())
        return result;

    const uint8_t *data = payload.data();
    size_t dataLen = payload.size();
    size_t offset = 0;

    while (offset < dataLen)
    {
        size_t chunkSize = std::min(static_cast<size_t>(maxData), dataLen - offset);
        if (offset + chunkSize < dataLen)
            chunkSize &= ~static_cast<size_t>(7);

        IPv4Packet frag;
        frag.version = version;
        frag.ihl = ihl;
        frag.tos = tos;
        frag.identification = identification;
        frag.ttl = ttl;
        frag.protocol = protocol;
        frag.srcIP = srcIP;
        frag.dstIP = dstIP;
        frag.payload.assign(data + offset, data + offset + chunkSize);

        uint16_t flags = 0;
        if (flagsFragment & FLAG_DF) flags |= FLAG_DF;
        if (offset + chunkSize < dataLen) flags |= FLAG_MF;
        uint16_t fragOff = static_cast<uint16_t>(offset / 8);
        frag.flagsFragment = flags | fragOff;

        frag.totalLen = static_cast<uint16_t>(headerLen + chunkSize);
        frag.checksum = frag.calculateChecksum();

        result.push_back(frag);
        offset += chunkSize;
    }

    return result;
}
