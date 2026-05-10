#include "core/Router.hpp" 
#include "core/Interface.hpp"
#include "layer2/EthernetFrame.hpp"
#include "layer2/ARP.hpp"
#include "layer3/IPv4Packet.hpp"
#include "layer3/ICMPMessage.hpp"
#include "util/NetUtils.hpp"
 
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <stdexcept>

Router::Router(const string& name) : Node(name) {
    std::cout << "[" << name << "] Router initialized" << std::endl; 
} 


std::string Router::getType() const{
    return "router"; 
}

void Router::setInterfaceIp(int port, const std::string& ipCidr){ 
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

void Router::addRoute(const string& cidr, const string& nextHopIp, int outPort){ 
    auto slash = cidr.find('/'); 
    string netStr = cidr.substr(0, slash); 
    uint8_t prefix = static_cast<uint8_t>(stoi(cidr.substr(slash+1)));

    uint32_t netRaw = NetUtils::applyMask(NetUtils::stringToIp(netStr), prefix); 

    for (const auto& e : routingTable_){ 
        if (e.destNetwork == NetUtils::ipToString(netRaw) && 
                e.prefixLen == prefix &&
                e.outPort == outPort)
            return; 
    } 
    RoutingTableEntry entry;
    entry.destNetwork = NetUtils::ipToString(netRaw);
    entry.prefixLen   = prefix;
    entry.nextHopIp   = nextHopIp;
    entry.outPort     = outPort;
    routingTable_.push_back(entry);
} 

const RoutingTableEntry* Router::longestPrefixMatch(const string& destIp) const
{
    uint32_t dest = NetUtils::stringToIp(destIp);
 
    const RoutingTableEntry* best      = nullptr;
    int                      bestLen   = -1;
 
    for (const auto& entry : routingTable_) {
        uint32_t net = NetUtils::applyMask(
            NetUtils::stringToIp(entry.destNetwork), entry.prefixLen);
        uint32_t masked = NetUtils::applyMask(dest, entry.prefixLen);
 
        if (masked == net && (int)entry.prefixLen > bestLen) {
            best    = &entry;
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
         << setw(8)  << "Prefix"
         << setw(18) << "Next Hop"
         << setw(6)  << "Port" << "\n";
    cout << string(60, '-') << "\n";
 
    if (routingTable_.empty()) {
        cout << "empty\n";
    } else {
        for (const auto& e : routingTable_) {
            string nh = e.nextHopIp.empty() ? "directly" : e.nextHopIp;
            cout << left
                 << setw(20) << e.destNetwork
                 << setw(8)  << ("/" + to_string(e.prefixLen))
                 << setw(18) << nh
                 << setw(6)  << e.outPort << "\n";
        }
    }
    cout << string(60, '-') << "\n\n";
}

void Router::handleReceive(Interface* inInterface, const vector<uint8_t>& rawBytes){
    if (!isActive()) return; 

    EthernetFrame frame; 
    try{ 
        frame.deserialize(rawBytes);    
    } catch(const exception& e){ 
        std::cerr << "failed to parse ethernet" << e.what() << std::endl;
        return; 
    } 

    string myMac = inInterface->getMacAddress(); 
    bool toMe = (frame.destMac == myMac) || frame.isBroadcast();

    if (frame.etherType == 0x0806){ 
        if (!toMe) return; 

        ARPPacket arp; 
        try {
            arp.deserialize(const_cast<vector<uint8_t>&>(frame.payload));
        }catch(...){
            return; 
        }
        string senderIp = arp.getSenderProtAddr();
        string senderMac = NetUtils::macToString(arp.getSenderHwAddr());
        arpCache_.addEntry(senderIp, senderMac); 

        if (arp.getOperation() == ARP_OP_REQUEST){
            int port = inInterface->getPortNumber(); 
            std::string myIp = getInterfaceIp(port);    
            if (arp.getTargetProtAddr() != myIp) return; 

            vector<uint8_t> myMacBytes = NetUtils::stringToMac(myMac); 
            ARPPacket reply = ARPPacket::createReply(myMacBytes, myIp, arp.getSenderHwAddr(), senderIp); 

            EthernetFrame resp(senderMac, myMac, 0x0806, reply.toBytes()); 
            inInterface->send(resp.toBytes()); 

            cout << getName() << " ARP Reply " << senderIp << "\n"; 
        }else if (arp.getOperation() == ARP_OP_REPLY){ 
            auto queued = arpCache_.dequeueAll(senderIp)    ;
            for (const auto& pkt : queued) {
                EthernetFrame ef(senderMac, myMac, 0x0800, pkt); 
                inInterface->send(ef.toBytes()); 

            }

        }
        return; 
    } 

    if (frame.etherType == 0x0800){ 
        routePacket(inInterface, frame.payload); 
        return; 
    }


}


void Router::routePacket(Interface* inInterface, const vector<uint8_t>& ipBytes)
{
        IPv4Packet pkt; 
        try { 
            pkt.deserialize(ipBytes);
        }catch (const exception& e) { 
            cerr << getName() << " Failed to parse Ipv4: " << e.what() << std::endl;
            return; 
        } 
        if (!pkt.verifyChecksum()){ 
            std::cout << getName() << " IPv4 checksum invalid, drop " << pkt.srcIP << " -> " << pkt.dstIP << std::endl;
            return;
        } 

        std::cout << "[" << getName() << "] IPv4 " << pkt.srcIP
         << " -> " << pkt.dstIP
         << " TTL=" << (int)pkt.ttl
         << " proto=" << (int)pkt.protocol << std::endl;

        if (pkt.ttl <= 1){ 
            std::cout << getName() << " End of TTL, ICMP sent to " << pkt.srcIP << std::endl; 
            sendIcmpTimeExceeded(inInterface, ipBytes);
            return;
        }
        pkt.decrementTTL();

        const RoutingTableEntry* route = longestPrefixMatch(pkt.dstIP); 
        if (!route){ 
            std::cout << getName() << " No route to " << pkt.dstIP << ", sending ICMP unreachable to " << pkt.srcIP << std::endl; 
            sendIcmpUnreachable(inInterface, ipBytes, 0);
            return; 
        } 
        
        cout << getName() << " Route: " << pkt.dstIP << " via " << (route->nextHopIp.empty() ? "direct" : route->nextHopIp) << " port" << route->outPort << std::endl; 
    
        string nextHopIp = route->nextHopIp.empty() ? pkt.dstIP : route->nextHopIp; 

        forwardIpPacket(route->outPort, nextHopIp, pkt.toBytes(), pkt.srcIP); 
} 

void Router::forwardIpPacket(int outPort, const std::string& nextHopIp, const std::vector<uint8_t>& ipBytes, const std::string& srcForIcmp){ 
    Interface* outIfc = getInterface(outPort);
    if (!outIfc) {
        std::cerr  << getName() << " No interface port" << outPort << "\n";
        return;
    }
    if (!outIfc->isConnected()) {
        std::cerr << getName() << "] Port" << outPort << " not connected\n";
        return;
    }

    string myMac = outIfc->getMacAddress();

    if (arpCache_.hasIp(nextHopIp)) {
        string dstMac = arpCache_.getMac(nextHopIp);
        EthernetFrame frame(dstMac, myMac, 0x0800, ipBytes);
        outIfc->send(frame.toBytes());
    } else {
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

string routerSrcIp(Router* r, Interface* inIfc)
{
    string ip = r->getInterfaceIp(inIfc->getPortNumber());
    return ip.empty() ? "0.0.0.0" : ip;
}


void Router::sendIcmpTimeExceeded(Interface* inInterface, const vector<uint8_t>&     originalIpBytes){
    vector<uint8_t> orig(originalIpBytes.begin(), originalIpBytes.begin() + min((size_t)28, originalIpBytes.size()));
    ICMPMessage icmp = ICMPMessage::buildTimeExceeded(orig);

    IPv4Packet origPkt;
    try { origPkt.deserialize(originalIpBytes); } catch (...) { return; }

    string myIp = routerSrcIp(this, inInterface);
    IPv4Packet reply(myIp, origPkt.srcIP,
                     IPv4Packet::PROTOCOL_ICMP, 64, icmp.toBytes());
    reply.checksum = reply.calculateChecksum();

    const RoutingTableEntry* route = longestPrefixMatch(origPkt.srcIP);
    if (!route) {
        std::cerr << "[" << getName() << "] Error: no route\n";
        return;
    }
    string nextHop = route->nextHopIp.empty() ? origPkt.srcIP : route->nextHopIp;
    forwardIpPacket(route->outPort, nextHop, reply.toBytes(), myIp);
}

void Router::sendIcmpUnreachable(Interface* inInterface, const vector<uint8_t>& originalIpBytes, uint8_t code)
{
    vector<uint8_t> orig(originalIpBytes.begin(), originalIpBytes.begin() + min((size_t)28, originalIpBytes.size()));
    ICMPMessage icmp = ICMPMessage::buildUnreachable(orig, code);

    IPv4Packet origPkt;
    try { origPkt.deserialize(originalIpBytes); } catch (...) { return; }

    string myIp = routerSrcIp(this, inInterface);
    IPv4Packet reply(myIp, origPkt.srcIP, IPv4Packet::PROTOCOL_ICMP, 64, icmp.toBytes());
    reply.checksum = reply.calculateChecksum();

    const RoutingTableEntry* route = longestPrefixMatch(origPkt.srcIP);
    if (!route) {
        std::cerr << "[" << getName() << "] Error: no route\n";
        return;
    }
    string nextHop = route->nextHopIp.empty() ? origPkt.srcIP : route->nextHopIp;
    forwardIpPacket(route->outPort, nextHop, reply.toBytes(), myIp);
}
