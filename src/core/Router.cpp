#include "core/Router.hpp"
#include "util/Exceptions.hpp"
#include "core/Interface.hpp"
#include "layer2/EthernetFrame.hpp"
#include "layer2/ARP.hpp"
#include "layer3/IPv4Packet.hpp"
#include "layer3/ICMPMessage.hpp"
#include "util/NetUtils.hpp"
#include "middleboxes/NAT.hpp"
#include "layer4/TCPSegment.hpp"
#include "layer4/UDPSegment.hpp"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <stdexcept>

Router::Router(const string &name) : Node(name)
{
    std::cout << "[" << name << "] Router initialized" << std::endl;
}

std::string Router::getType() const
{
    return "router";
}

void Router::setInterfaceIp(int port, const std::string &ipCidr)
{
    auto [ip, prefix] = NetUtils::parseCIDR(ipCidr);

    ifaceIp_[port] = NetUtils::ipToString(ip);
    ifacePrefix_[port] = prefix;

    std::string network = NetUtils::ipToString(NetUtils::applyMask(ip, prefix));
    std::string cidr = network + "/" + to_string(prefix);
    addRoute(cidr, "", port);

    std::cout << "[" << getName() << "] Interface port" << port
              << " IP=" << ifaceIp_[port] << "/" << (int)prefix << "\n";
};

string Router::getInterfaceIp(int port) const
{
    auto it = ifaceIp_.find(port);
    return (it != ifaceIp_.end()) ? it->second : "";
}

void Router::addRoute(const string &cidr, const string &nextHopIp, int outPort)
{
    auto slash = cidr.find('/');
    string netStr = cidr.substr(0, slash);
    uint8_t prefix = static_cast<uint8_t>(stoi(cidr.substr(slash + 1)));

    uint32_t netRaw = NetUtils::applyMask(NetUtils::stringToIp(netStr), prefix);

    for (const auto &e : routingTable_)
    {
        if (e.destNetwork == NetUtils::ipToString(netRaw) &&
            e.prefixLen == prefix &&
            e.outPort == outPort)
            return;
    }
    RoutingTableEntry entry;
    entry.destNetwork = NetUtils::ipToString(netRaw);
    entry.prefixLen = prefix;
    entry.nextHopIp = nextHopIp;
    entry.outPort = outPort;
    routingTable_.push_back(entry);
}

const RoutingTableEntry *Router::longestPrefixMatch(const string &destIp) const
{
    uint32_t dest = NetUtils::stringToIp(destIp);

    const RoutingTableEntry *best = nullptr;
    int bestLen = -1;

    for (const auto &entry : routingTable_)
    {
        uint32_t net = NetUtils::applyMask(
            NetUtils::stringToIp(entry.destNetwork), entry.prefixLen);
        uint32_t masked = NetUtils::applyMask(dest, entry.prefixLen);

        if (masked == net && (int)entry.prefixLen > bestLen)
        {
            best = &entry;
            bestLen = entry.prefixLen;
        }
    }
    return best;
}

void Router::printRoutingTable() const
{
    cout << "\n[" << getName() << "] Routing Table:\n";
    cout << string(60, '-') << "\n";
    cout << left
         << setw(20) << "Destination"
         << setw(8) << "Prefix"
         << setw(18) << "Next Hop"
         << setw(6) << "Port" << "\n";
    cout << string(60, '-') << "\n";

    if (routingTable_.empty())
    {
        cout << "empty\n";
    }
    else
    {
        for (const auto &e : routingTable_)
        {
            string nh = e.nextHopIp.empty() ? "directly" : e.nextHopIp;
            cout << left
                 << setw(20) << e.destNetwork
                 << setw(8) << ("/" + to_string(e.prefixLen))
                 << setw(18) << nh
                 << setw(6) << e.outPort << "\n";
        }
    }
    cout << string(60, '-') << "\n\n";
}
void Router::printArpCache() const
{
    cout << "\n[" << getName() << "] ARP Cache:\n";
    cout << string(44, '-') << "\n";
    cout << dec << setfill(' '); // tambahkan ini
    cout << left << setw(18) << "IP Address" << setw(20) << "MAC Address" << "\n";
    cout << string(44, '-') << "\n";
    const auto &tbl = arpCache_.getTable();
    if (tbl.empty())
        cout << "(kosong)\n";
    else
        for (const auto &[ip, mac] : tbl)
            cout << dec << setfill(' ') << left << setw(18) << ip << setw(20) << mac << "\n";
    cout << string(44, '-') << "\n\n";
}

void Router::handleReceive(Interface *inInterface, const vector<uint8_t> &rawBytes)
{
    if (!isActive())
        return;

    EthernetFrame frame;
    try
    {
        frame.deserialize(rawBytes);
    }
    catch (const exception &e)
    {
        throw ParseException("Failed to parse ethernet: " + std::string(e.what()));
    }

    string myMac = inInterface->getMacAddress();
    bool toMe = (frame.destMac == myMac) || frame.isBroadcast();

    if (frame.etherType == 0x0806)
    {
        if (!toMe)
            return;

        ARPPacket arp;
        try
        {
            arp.deserialize(const_cast<vector<uint8_t> &>(frame.payload));
        }
        catch (...)
        {
            return;
        }
        string senderIp = arp.getSenderProtAddr();
        string senderMac = NetUtils::macToString(arp.getSenderHwAddr());
        arpCache_.addEntry(senderIp, senderMac);

        if (arp.getOperation() == ARP_OP_REQUEST)
        {
            int port = inInterface->getPortNumber();
            std::string myIp = getInterfaceIp(port);
            if (arp.getTargetProtAddr() != myIp)
                return;

            vector<uint8_t> myMacBytes = NetUtils::stringToMac(myMac);
            ARPPacket reply = ARPPacket::createReply(myMacBytes, myIp, arp.getSenderHwAddr(), senderIp);

            EthernetFrame resp(senderMac, myMac, 0x0806, reply.toBytes());
            inInterface->send(resp.toBytes());

            cout << getName() << " ARP Reply " << senderIp << "\n";
        }
        else if (arp.getOperation() == ARP_OP_REPLY)
        {
            auto queued = arpCache_.dequeueAll(senderIp);
            for (const auto &pkt : queued)
            {
                EthernetFrame ef(senderMac, myMac, 0x0800, pkt);
                inInterface->send(ef.toBytes());
            }
        }
        return;
    }

    if (frame.etherType == 0x0800)
    {
        routePacket(inInterface, frame.payload);
        return;
    }
}

void Router::routePacket(Interface *inInterface, const vector<uint8_t> &ipBytes)
{
    IPv4Packet pkt;
    try
    {
        pkt.deserialize(ipBytes);
    }
    catch (const exception &e)
    {
        throw ParseException(getName() + " failed to parse IPv4: " + std::string(e.what()));
    }
    if (pkt.dstIP == "255.255.255.255")
    {
        for (auto &[port, ip] : ifaceIp_)
        {
            Interface *outIfc = getInterface(port);
            if (outIfc && outIfc != inInterface && outIfc->isConnected())
            {
                int pathMTU = outIfc->getMtu();
                if (static_cast<int>(ipBytes.size()) > pathMTU && !pkt.isDontFragment())
                {
                    vector<IPv4Packet> frags = pkt.fragment(static_cast<uint16_t>(pathMTU));
                    string myMac = outIfc->getMacAddress();
                    string dstMac = "FF:FF:FF:FF:FF:FF";
                    for (const auto &f : frags)
                    {
                        EthernetFrame frame(dstMac, myMac, 0x0800, f.toBytes());
                        outIfc->send(frame.toBytes());
                    }
                }
                else
                {
                    outIfc->send(ipBytes);
                }
            }
        }
        return;
    }
    if (!pkt.verifyChecksum())
    {
        std::cout << getName() << " IPv4 checksum invalid, drop " << pkt.srcIP << " -> " << pkt.dstIP << std::endl;
        return;
    }

    std::cout << "[" << getName() << "] IPv4 " << pkt.srcIP
              << " -> " << pkt.dstIP
              << " TTL=" << (int)pkt.ttl
              << " proto=" << (int)pkt.protocol << std::endl;

    // === NAT INBOUND ===
    if (nat_) {
        int inPort = inInterface->getPortNumber();
        if (inPort == natPublicPort_) {
            if (pkt.protocol == 6) {
                TCPSegment seg = TCPSegment::deserialize(pkt.payload);
                std::string privateIp; uint16_t privatePort;
                if (!nat_->translateInbound(seg.dstPort, privateIp, privatePort)) {
                    std::cout << "[NAT] Inbound: tidak ada mapping untuk port "
                              << seg.dstPort << ", drop.\n";
                    return;
                }
                seg.dstPort = privatePort;
                pkt.dstIP   = privateIp;
                seg.calculateChecksum(pkt.srcIP, pkt.dstIP);
                pkt.payload = seg.toBytes();
            } else if (pkt.protocol == 17) {
                UDPSegment seg = UDPSegment::deserialize(pkt.payload);
                std::string privateIp; uint16_t privatePort;
                if (!nat_->translateInbound(seg.dstPort, privateIp, privatePort)) {
                    std::cout << "[NAT] Inbound: tidak ada mapping untuk port "
                              << seg.dstPort << ", drop.\n";
                    return;
                }
                seg.dstPort  = privatePort;
                pkt.dstIP    = privateIp;
                seg.checksum = UDPSegment::calculateChecksum(pkt.srcIP, pkt.dstIP, seg);
                pkt.payload  = seg.toBytes();
            } else if (pkt.protocol == 1) {
                ICMPMessage icmp;
                icmp.deserialize(pkt.payload);
                uint16_t identifier = icmp.getIdentifier();
                std::string privateIp; uint16_t privatePort;
                if (!nat_->translateInboundICMP(identifier, privateIp, privatePort)) {
                    std::cout << "[NAT] Inbound ICMP: tidak ada mapping untuk id "
                              << identifier << ", drop.\n";
                    return;
                }
                pkt.dstIP = privateIp;
            }
            pkt.checksum = 0;
            pkt.checksum = pkt.calculateChecksum();
            std::cout << "[NAT] Inbound: → " << pkt.dstIP << "\n";
        }
    }
    // === END NAT INBOUND ===

    // Cek apakah paket ditujukan ke salah satu IP interface router ini
    for (const auto &[port, ip] : ifaceIp_)
    {
        if (ip == pkt.dstIP)
        {
            // Paket untuk router sendiri — balas ICMP Echo Reply
            if (pkt.protocol == IPv4Packet::PROTOCOL_ICMP)
            {
                ICMPMessage icmp;
                try
                {
                    icmp.deserialize(pkt.payload);
                }
                catch (...)
                {
                    return;
                }

                if (icmp.getICMPType() == ICMP::TYPE_ECHO_REQUEST)
                {
                    ICMPMessage reply = ICMPMessage::buildEchoReply(icmp);
                    IPv4Packet replyPkt(ip, pkt.srcIP,
                                        IPv4Packet::PROTOCOL_ICMP, 64,
                                        reply.toBytes());
                    replyPkt.checksum = replyPkt.calculateChecksum();

                    string nextHop = pkt.srcIP;
                    forwardIpPacket(port, nextHop, replyPkt.toBytes(), ip);
                    cout << "[" << getName() << "] ICMP Echo Reply -> "
                         << pkt.srcIP << "\n";
                }
            }
            return; // paket untuk router, tidak perlu di-forward
        }
    }

    if (pkt.ttl <= 1)
    {
        std::cout << getName() << " End of TTL, ICMP sent to " << pkt.srcIP << std::endl;
        sendIcmpTimeExceeded(inInterface, ipBytes);
        return;
    }
    pkt.decrementTTL();
    std::cout << "[" << getName() << "] TTL decremented -> " << (int)pkt.ttl
              << " | checksum recalculated\n";

    const RoutingTableEntry *route = longestPrefixMatch(pkt.dstIP);
    if (!route)
    {
        std::cout << getName() << " No route to " << pkt.dstIP << ", sending ICMP unreachable to " << pkt.srcIP << std::endl;
        sendIcmpUnreachable(inInterface, ipBytes, 0);
        return;
    }

    cout << getName() << " Route: " << pkt.dstIP << " via " << (route->nextHopIp.empty() ? "direct" : route->nextHopIp) << " port" << route->outPort << std::endl;

    string nextHopIp = route->nextHopIp.empty() ? pkt.dstIP : route->nextHopIp;

    if (!acl_.empty())
    {
        int srcPort = -1, dstPort = -1;
        if ((pkt.protocol == 6 || pkt.protocol == 17) && pkt.payload.size() >= 4)
        {
            srcPort = (pkt.payload[0] << 8) | pkt.payload[1];
            dstPort = (pkt.payload[2] << 8) | pkt.payload[3];
        }
        if (!acl_.isPermitted(pkt.srcIP, pkt.dstIP, pkt.protocol,
                              srcPort, dstPort,
                              inInterface->getPortNumber(), route->outPort))
        {
            return;
        }
    }

    // === NAT OUTBOUND ===
    if (nat_) {
        int inPort = inInterface->getPortNumber();
        if (inPort == natPrivatePort_) {
            std::string originalSrcIp = pkt.srcIP;

            if (pkt.protocol == 6) {
                TCPSegment seg = TCPSegment::deserialize(pkt.payload);
                uint16_t mappedPort = nat_->translateOutbound(pkt.srcIP, seg.srcPort);
                if (mappedPort == 0) return;
                seg.srcPort = mappedPort;
                pkt.srcIP   = nat_->getPublicIp();
                seg.calculateChecksum(pkt.srcIP, pkt.dstIP);
                pkt.payload = seg.toBytes();
            } else if (pkt.protocol == 17) {
                UDPSegment seg = UDPSegment::deserialize(pkt.payload);
                uint16_t mappedPort = nat_->translateOutbound(pkt.srcIP, seg.srcPort);
                if (mappedPort == 0) return;
                seg.srcPort  = mappedPort;
                pkt.srcIP    = nat_->getPublicIp();
                seg.checksum = UDPSegment::calculateChecksum(pkt.srcIP, pkt.dstIP, seg);
                pkt.payload  = seg.toBytes();
            } else if (pkt.protocol == 1) {
                ICMPMessage icmp;
                icmp.deserialize(pkt.payload);
                uint16_t identifier = icmp.getIdentifier();
                uint16_t mappedPort = nat_->translateOutbound(pkt.srcIP, identifier);
                if (mappedPort == 0) return;
                pkt.srcIP = nat_->getPublicIp();
            }

            pkt.checksum = 0;
            pkt.checksum = pkt.calculateChecksum();
            std::cout << "[NAT] Outbound: " << originalSrcIp
                      << " → " << pkt.srcIP << "\n";
        }
    }
    // === END NAT OUTBOUND ===

    forwardIpPacket(route->outPort, nextHopIp, pkt.toBytes(), pkt.srcIP);
}

void Router::forwardIpPacket(int outPort, const std::string &nextHopIp, const std::vector<uint8_t> &ipBytes, const std::string &srcForIcmp)
{
    Interface *outIfc = getInterface(outPort);
    if (!outIfc)
    {
        throw NetworkException(getName() + " has no interface on port " + std::to_string(outPort));
    }
    if (!outIfc->isConnected())
    {
        throw NetworkException(getName() + " port " + std::to_string(outPort) + " is not connected");
    }

    int pathMTU = outIfc->getMtu();
    string myMac = outIfc->getMacAddress();

    IPv4Packet pkt;
    try
    {
        pkt.deserialize(ipBytes);
    }
    catch (const exception &e)
    {
        throw ParseException(getName() + " forwardIpPacket failed to parse IP: " + std::string(e.what()));
    }

    uint16_t ipTotalLen = static_cast<uint16_t>(ipBytes.size());

    if (ipTotalLen > static_cast<uint16_t>(pathMTU))
    {
        if (pkt.isDontFragment())
        {
            cout << "[" << getName() << "] Packet too big (" << ipTotalLen
                 << " > MTU " << pathMTU << ") and DF set, ICMP Frag Needed\n";
            Interface *inIfc = nullptr;
            for (auto &[port, ip] : ifaceIp_)
            {
                if (ip == srcForIcmp)
                {
                    inIfc = getInterface(port);
                    break;
                }
            }
            if (!inIfc)
            {
                for (int i = 1; i <= 48; ++i)
                {
                    Interface *tmp = getInterface(i);
                    if (tmp && tmp->isConnected())
                    {
                        inIfc = tmp;
                        break;
                    }
                }
            }
            if (inIfc)
                sendIcmpFragNeeded(inIfc, ipBytes, static_cast<uint16_t>(pathMTU));
            return;
        }

        vector<IPv4Packet> fragments = pkt.fragment(static_cast<uint16_t>(pathMTU));
        if (fragments.empty())
        {
            throw NetworkException(getName() + " fragmentation failed, dropping packet");
        }
        if (fragments.size() == 1 && !pkt.isDontFragment())
        {
            // doesn't actually need fragmentation
        }
        else
        {
            cout << "[" << getName() << "] Fragmenting packet " << pkt.srcIP
                 << " -> " << pkt.dstIP
                 << " (" << ipTotalLen << " bytes) into "
                 << fragments.size() << " fragments (MTU=" << pathMTU << ")\n";
        }

        for (const auto &frag : fragments)
        {
            vector<uint8_t> fragBytes = frag.toBytes();
            if (arpCache_.hasIp(nextHopIp))
            {
                string dstMac = arpCache_.getMac(nextHopIp);
                EthernetFrame frame(dstMac, myMac, 0x0800, fragBytes);
                outIfc->send(frame.toBytes());
            }
            else
            {
                cout << "[" << getName() << "] MAC " << nextHopIp
                     << " unknown, enqueued fragment\n";
                arpCache_.enqueuePacket(nextHopIp, fragBytes);
                string myIp = getInterfaceIp(outPort);
                vector<uint8_t> myMacBytes = NetUtils::stringToMac(myMac);
                ARPPacket req = ARPPacket::createRequest(myMacBytes, myIp, nextHopIp);
                EthernetFrame arpFrame("FF:FF:FF:FF:FF:FF", myMac, 0x0806, req.toBytes());
                outIfc->send(arpFrame.toBytes());
            }
        }
        return;
    }

    if (arpCache_.hasIp(nextHopIp))
    {
        string dstMac = arpCache_.getMac(nextHopIp);
        EthernetFrame frame(dstMac, myMac, 0x0800, ipBytes);
        outIfc->send(frame.toBytes());
    }
    else
    {
        cout << "[" << getName() << "] MAC " << nextHopIp
             << " unknown, enqueued \n";
        arpCache_.enqueuePacket(nextHopIp, ipBytes);

        string myIp = getInterfaceIp(outPort);
        vector<uint8_t> myMacBytes = NetUtils::stringToMac(myMac);
        ARPPacket req = ARPPacket::createRequest(myMacBytes, myIp, nextHopIp);
        EthernetFrame arpFrame("FF:FF:FF:FF:FF:FF", myMac, 0x0806, req.toBytes());
        outIfc->send(arpFrame.toBytes());
    }
}

string routerSrcIp(Router *r, Interface *inIfc)
{
    string ip = r->getInterfaceIp(inIfc->getPortNumber());
    return ip.empty() ? "0.0.0.0" : ip;
}

void Router::sendIcmpTimeExceeded(Interface *inInterface, const vector<uint8_t> &originalIpBytes)
{
    vector<uint8_t> orig(originalIpBytes.begin(), originalIpBytes.begin() + min((size_t)28, originalIpBytes.size()));
    ICMPMessage icmp = ICMPMessage::buildTimeExceeded(orig);

    IPv4Packet origPkt;
    try
    {
        origPkt.deserialize(originalIpBytes);
    }
    catch (...)
    {
        return;
    }

    string myIp = routerSrcIp(this, inInterface);
    IPv4Packet reply(myIp, origPkt.srcIP,
                     IPv4Packet::PROTOCOL_ICMP, 64, icmp.toBytes());
    reply.checksum = reply.calculateChecksum();

    const RoutingTableEntry *route = longestPrefixMatch(origPkt.srcIP);
    if (!route)
    {
        throw NetworkException(getName() + " error: no route");
    }
    string nextHop = route->nextHopIp.empty() ? origPkt.srcIP : route->nextHopIp;
    forwardIpPacket(route->outPort, nextHop, reply.toBytes(), myIp);
}

void Router::sendIcmpUnreachable(Interface *inInterface, const vector<uint8_t> &originalIpBytes, uint8_t code)
{
    vector<uint8_t> orig(originalIpBytes.begin(), originalIpBytes.begin() + min((size_t)28, originalIpBytes.size()));
    ICMPMessage icmp = ICMPMessage::buildUnreachable(orig, code);

    IPv4Packet origPkt;
    try
    {
        origPkt.deserialize(originalIpBytes);
    }
    catch (...)
    {
        return;
    }

    string myIp = routerSrcIp(this, inInterface);
    IPv4Packet reply(myIp, origPkt.srcIP, IPv4Packet::PROTOCOL_ICMP, 64, icmp.toBytes());
    reply.checksum = reply.calculateChecksum();

    const RoutingTableEntry *route = longestPrefixMatch(origPkt.srcIP);
    if (!route)
    {
        throw NetworkException(getName() + " error: no route");
    }
    string nextHop = route->nextHopIp.empty() ? origPkt.srcIP : route->nextHopIp;
    forwardIpPacket(route->outPort, nextHop, reply.toBytes(), myIp);
}

void Router::sendIcmpFragNeeded(Interface *inInterface, const vector<uint8_t> &originalIpBytes, uint16_t nextHopMTU)
{
    vector<uint8_t> orig(originalIpBytes.begin(), originalIpBytes.begin() + min((size_t)28, originalIpBytes.size()));
    ICMPMessage icmp = ICMPMessage::buildFragmentationNeeded(nextHopMTU, orig);

    IPv4Packet origPkt;
    try
    {
        origPkt.deserialize(originalIpBytes);
    }
    catch (...)
    {
        return;
    }

    string myIp = routerSrcIp(this, inInterface);
    IPv4Packet reply(myIp, origPkt.srcIP, IPv4Packet::PROTOCOL_ICMP, 64, icmp.toBytes());
    reply.checksum = reply.calculateChecksum();

    const RoutingTableEntry *route = longestPrefixMatch(origPkt.srcIP);
    if (!route)
    {
        throw NetworkException(getName() + " error: no route for Frag Needed ICMP");
    }
    string nextHop = route->nextHopIp.empty() ? origPkt.srcIP : route->nextHopIp;
    forwardIpPacket(route->outPort, nextHop, reply.toBytes(), myIp);
}

void Router::addACLRule(const ACLRule &rule)
{
    acl_.addRule(rule);
}
void Router::removeACLRule(int index)
{
    acl_.removeRule(index);
}
void Router::printACL() const
{
    cout << "\n[" << getName() << "] ACL Rules:\n";
    acl_.printRules();
    cout << "\n";
}

void Router::enableNAT(const std::string& publicIp,
                       int privatePort, int publicPort) {
    nat_ = std::make_unique<NAT>(publicIp);
    natPrivatePort_ = privatePort;
    natPublicPort_  = publicPort;
    std::cout << "[" << getName() << "] NAT enabled: "
              << "private=port" << privatePort
              << " public=port" << publicPort
              << " (" << publicIp << ")\n";
}

void Router::printNATTable() const {
    if (!nat_) {
        std::cout << "[" << getName() << "] NAT tidak aktif.\n";
        return;
    }
    nat_->printTable();
}