#pragma once
#include "core/Node.hpp"
#include "layer2/ARP.hpp"
#include <string>
#include <cstdint>

class Host : public Node
{
public:
    Host(const std::string &name,
         const std::string &ipCidr,
         const std::string &defaultGateway);

    std::string getIpAddress() const;
    uint8_t getPrefixLength() const;
    std::string getNetworkAddress() const;
    std::string getDefaultGateway() const;
    std::string getType() const override;

    ARPCache &getArpCache();
    const ARPCache &getArpCache() const;
    void printArpCache() const;

    void sendLayer3Packet(const std::string &destIp,
                          const std::vector<uint8_t> &l3Payload,
                          uint8_t protocol);

    bool ping(const std::string &targetIp, int count = 4);
    void traceroute(const std::string &targetIp, int maxHops = 30);

    void handleReceive(Interface *inInterface,
                       const std::vector<uint8_t> &rawBytes) override;
    void setIpAddress(const std::string &ipCidr);
    void setDefaultGateway(const std::string &gw);

private:
    uint32_t ipRaw_;
    uint8_t prefix_;
    uint32_t gatewayRaw_;
    ARPCache arpCache_;

    bool icmpReplyReceived_ = false;
    std::string icmpReplySrcIp_;
    uint16_t pendingIdentifier_ = 0;
    uint16_t pendingSeq_ = 0;
    uint8_t icmpReplyType_ = 0;

    bool isLocalNetwork(const std::string &destIp) const;
    void sendEthernet(const std::string &destMac,
                      uint16_t etherType,
                      const std::vector<uint8_t> &payload);
    void sendArpRequest(const std::string &targetIp);
    void handleArpPacket(const std::vector<uint8_t> &arpBytes);
    void handleIPv4Packet(Interface *inInterface,
                          const std::vector<uint8_t> &ipBytes);
    void handleIcmpPacket(const std::string &srcIp,
                          const std::vector<uint8_t> &icmpBytes);
};