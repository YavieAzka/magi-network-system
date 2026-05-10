#include "layer2/ARP.hpp"
#include "util/NetUtils.hpp"

ARPPacket::ARPPacket()
    : hardware_type(1), protocol_type(0x0800),
      hardware_len(6), protocol_len(4), operation(0)
{
    sender_hw_addr.assign(6, 0);
    target_hw_addr.assign(6, 0);
}

std::vector<uint8_t> ARPPacket::toBytes() const
{
    std::vector<uint8_t> bytes;
    bytes.reserve(28);

    ByteUtils::write_u16_be(bytes, hardware_type);
    ByteUtils::write_u16_be(bytes, protocol_type);
    bytes.push_back(hardware_len);
    bytes.push_back(protocol_len);
    ByteUtils::write_u16_be(bytes, operation);

    ByteUtils::append_bytes(bytes, sender_hw_addr);

    uint32_t sip = NetUtils::stringToIp(sender_prot_addr);
    ByteUtils::write_u32_be(bytes, sip);

    ByteUtils::append_bytes(bytes, target_hw_addr);

    uint32_t tip = NetUtils::stringToIp(target_prot_addr);
    ByteUtils::write_u32_be(bytes, tip);

    return bytes;
}

void ARPPacket::deserialize(const std::vector<uint8_t> &raw)
{
    if (raw.size() < 28)
        throw std::out_of_range("Data too short for ARP Packet (min. 28 byte)");

    hardware_type = ByteUtils::read_u16_be(raw, 0);
    protocol_type = ByteUtils::read_u16_be(raw, 2);
    hardware_len = raw[4];
    protocol_len = raw[5];
    operation = ByteUtils::read_u16_be(raw, 6);

    sender_hw_addr.assign(raw.begin() + 8, raw.begin() + 14);
    sender_prot_addr = NetUtils::ipToString(ByteUtils::read_u32_be(raw, 14));

    target_hw_addr.assign(raw.begin() + 18, raw.begin() + 24);
    target_prot_addr = NetUtils::ipToString(ByteUtils::read_u32_be(raw, 24));
}

ARPPacket ARPPacket::createRequest(const std::vector<uint8_t> &senderMac,
                                   const std::string &senderIp,
                                   const std::string &targetIp)
{
    ARPPacket pkt;
    pkt.operation = ARP_OP_REQUEST;
    pkt.sender_hw_addr = senderMac;
    pkt.sender_prot_addr = senderIp;
    pkt.target_hw_addr.assign(6, 0);
    pkt.target_prot_addr = targetIp;
    return pkt;
}

ARPPacket ARPPacket::createReply(const std::vector<uint8_t> &senderMac,
                                 const std::string &senderIp,
                                 const std::vector<uint8_t> &targetMac,
                                 const std::string &targetIp)
{
    ARPPacket pkt;
    pkt.operation = ARP_OP_REPLY;
    pkt.sender_hw_addr = senderMac;
    pkt.sender_prot_addr = senderIp;
    pkt.target_hw_addr = targetMac;
    pkt.target_prot_addr = targetIp;
    return pkt;
}

bool ARPCache::hasIp(const std::string &ip) const
{
    return arpcache.find(ip) != arpcache.end();
}

void ARPCache::addEntry(const std::string &ip, const std::string &mac)
{
    arpcache[ip] = mac;
}

void ARPCache::clearCache()
{
    arpcache.clear();
}

std::string ARPCache::getMac(const std::string &ip) const
{
    auto it = arpcache.find(ip);
    if (it != arpcache.end())
        return it->second;
    return "";
}

void ARPCache::enqueuePacket(const std::string &ip, const std::vector<uint8_t> &packet)
{
    ipPacketQueue[ip].push_back(packet);
}

void ARPCache::clearQueue()
{
    ipPacketQueue.clear();
}

std::vector<std::vector<uint8_t>> ARPCache::dequeueAll(const std::string &ip)
{
    std::vector<std::vector<uint8_t>> result;
    auto it = ipPacketQueue.find(ip);
    if (it != ipPacketQueue.end())
    {
        result = std::move(it->second);
        ipPacketQueue.erase(it);
    }
    return result;
}

const std::map<std::string, std::string> &ARPCache::getTable() const
{
    return arpcache;
}