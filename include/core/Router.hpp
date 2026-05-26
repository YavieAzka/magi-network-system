#pragma once
#include "core/Node.hpp"
#include "layer2/ARP.hpp"
#include "middleboxes/ACL.hpp"

#include <string>
#include <vector>
#include <cstdint>

struct RoutingTableEntry
{
    std::string destNetwork;
    uint8_t prefixLen;
    std::string nextHopIp;
    int outPort;
};

class Router : public Node
{
public:
    Router(const std::string &name);
    ~Router() override = default;

    std::string getType() const override;
    void handleReceive(Interface *inInterface, const std::vector<uint8_t> &rawBytes) override;

    void addRoute(const std::string &cidr, const std::string &nextHopIp, int outport);

    const RoutingTableEntry *longestPrefixMatch(const std::string &destIp) const;

    void printRoutingTable() const;

    ARPCache &getArpCache() { return arpCache_; }
    void printArpCache() const;

    void setInterfaceIp(int port, const std::string &ipCidr);

    std::string getInterfaceIp(int port) const;

    void addACLRule(const ACLRule &rule);
    void removeACLRule(int index);
    void printACL() const;
    ACLList &getACL() { return acl_; }

private:
    std::vector<RoutingTableEntry> routingTable_;
    ARPCache arpCache_;

    std::map<int, std::string> ifaceIp_;
    std::map<int, uint8_t> ifacePrefix_;

    ACLList acl_;

    void routePacket(Interface *inInterface, const std::vector<uint8_t> &ipBytes);
    void sendIcmpTimeExceeded(Interface *inInterface, const std::vector<uint8_t> &originalIpBytes);

    void sendIcmpUnreachable(Interface *inInterface, const std::vector<uint8_t> &originalIpBytes, uint8_t code = 0);

    void sendIcmpFragNeeded(Interface *inInterface, const std::vector<uint8_t> &originalIpBytes, uint16_t nextHopMTU);

    void forwardIpPacket(int outPort, const std::string &nextHopOrDest, const std::vector<uint8_t> &ipBytes, const std::string &srcFromIcmp);
};
