#include "core/Host.hpp"
#include "core/Interface.hpp"
#include "layer2/EthernetFrame.hpp"
#include "layer2/ARP.hpp"
#include "layer3/IPv4Packet.hpp"
#include "layer3/ICMPMessage.hpp"
#include "layer4/TCPSegment.hpp"
#include "layer4/TCPSocket.hpp"
#include "util/NetUtils.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
using namespace std;

Host::Host(const string &name, const string &ipCidr, const string &defaultGateway)
    : Node(name)
{
    NetUtils util;
    auto [ip, prefix] = util.parseCIDR(ipCidr);

    ipRaw_ = ip;
    prefix_ = prefix;
    gatewayRaw_ = util.stringToIp(defaultGateway);

    cout << "[" << name << "] Host dibuat"
         << "  IP=" << getIpAddress() << "/" << (int)prefix_
         << "  GW=" << defaultGateway << "\n";
}

string Host::getIpAddress() const { return NetUtils::ipToString(ipRaw_); }
uint8_t Host::getPrefixLength() const { return prefix_; }
string Host::getNetworkAddress() const
{
    return NetUtils::ipToString(NetUtils::applyMask(ipRaw_, prefix_));
}
string Host::getDefaultGateway() const { return NetUtils::ipToString(gatewayRaw_); }
string Host::getType() const { return "host"; }

ARPCache &Host::getArpCache() { return arpCache_; }
const ARPCache &Host::getArpCache() const { return arpCache_; }

void Host::printArpCache() const
{
    cout << "\n[" << getName() << "] ARP Cache:\n";
    cout << string(44, '-') << "\n";
    cout << left << setw(18) << "IP Address" << setw(20) << "MAC Address" << "\n";
    cout << string(44, '-') << "\n";
    const auto &tbl = arpCache_.getTable();
    if (tbl.empty())
        cout << "(kosong)\n";
    else
        for (const auto &[ip, mac] : tbl)
            cout << left << setw(18) << ip << setw(20) << mac << "\n";
    cout << string(44, '-') << "\n\n";
}

bool Host::isLocalNetwork(const string &destIp) const
{
    uint32_t dest = NetUtils::stringToIp(destIp);
    return NetUtils::applyMask(dest, prefix_) ==
           NetUtils::applyMask(ipRaw_, prefix_);
}

void Host::sendEthernet(const string &destMac, uint16_t etherType,
                        const vector<uint8_t> &payload)
{
    Interface *ifc = getInterface(1);
    if (!ifc)
    {
        cerr << "[" << getName() << "] Tidak ada interface port 1\n";
        return;
    }
    EthernetFrame frame(destMac, ifc->getMacAddress(), etherType, payload);
    ifc->send(frame.toBytes());
}

void Host::sendArpRequest(const string &targetIp)
{
    Interface *ifc = getInterface(1);
    if (!ifc)
        return;
    vector<uint8_t> myMac = NetUtils::stringToMac(ifc->getMacAddress());
    ARPPacket req = ARPPacket::createRequest(myMac, getIpAddress(), targetIp);
    cout << "[" << getName() << "] ARP Request: siapa punya " << targetIp << "?\n";
    sendEthernet("FF:FF:FF:FF:FF:FF", 0x0806, req.toBytes());
}

void Host::sendLayer3Packet(const string &destIp,
                            const vector<uint8_t> &l3Payload,
                            uint8_t protocol)
{
    IPv4Packet ipPkt(getIpAddress(), destIp, protocol, 64, l3Payload);
    vector<uint8_t> ipBytes = ipPkt.toBytes();

    string nextHopIp = isLocalNetwork(destIp)
                           ? destIp
                           : NetUtils::ipToString(gatewayRaw_);

    cout << "[" << getName() << "] Kirim L3 ke " << destIp
         << " via next-hop " << nextHopIp << "\n";

    if (arpCache_.hasIp(nextHopIp))
    {
        sendEthernet(arpCache_.getMac(nextHopIp), 0x0800, ipBytes);
    }
    else
    {
        cout << "[" << getName() << "] MAC " << nextHopIp
             << " belum diketahui, mengantrekan IP packet\n";
        arpCache_.enqueuePacket(nextHopIp, ipBytes);
        sendArpRequest(nextHopIp);
    }
}

bool Host::ping(const string &targetIp, int count)
{
    cout << "\n[" << getName() << "] PING " << targetIp
         << " (" << count << " packets)\n";

    int received = 0;
    static uint16_t globalId = 0x1234;
    uint16_t id = ++globalId;
    vector<uint8_t> data(32, 0xAB);

    for (int seq = 1; seq <= count; seq++)
    {
        icmpReplyReceived_ = false;
        pendingIdentifier_ = id;
        pendingSeq_ = static_cast<uint16_t>(seq);

        ICMPMessage req = ICMPMessage::buildEchoRequest(id, seq, data);
        auto t0 = chrono::steady_clock::now();
        sendLayer3Packet(targetIp, req.toBytes(), IPv4Packet::PROTOCOL_ICMP);
        auto t1 = chrono::steady_clock::now();
        double rttMs = chrono::duration<double, milli>(t1 - t0).count();

        if (icmpReplyReceived_)
        {
            cout << "[" << getName() << "] Reply from " << icmpReplySrcIp_
                 << ": seq=" << seq
                 << " time=" << fixed << setprecision(2) << rttMs << "ms\n";
            ++received;
        }
        else
        {
            cout << "[" << getName() << "] Request timeout seq=" << seq << "\n";
        }
    }
    cout << "[" << getName() << "] Ping stats: "
         << received << "/" << count << " received\n\n";
    return received > 0;
}

void Host::traceroute(const string &targetIp, int maxHops)
{
    cout << "\n[" << getName() << "] TRACEROUTE ke " << targetIp << "\n";

    static uint16_t traceId = 0x5678;
    uint16_t id = ++traceId;
    vector<uint8_t> data(32, 0xAA);

    for (int ttl = 1; ttl <= maxHops; ttl++)
    {
        icmpReplyReceived_ = false;
        icmpReplySrcIp_ = "";
        pendingIdentifier_ = id;
        pendingSeq_ = static_cast<uint16_t>(ttl);

        ICMPMessage req = ICMPMessage::buildEchoRequest(id, ttl, data);
        IPv4Packet ipPkt(getIpAddress(), targetIp,
                         IPv4Packet::PROTOCOL_ICMP,
                         static_cast<uint8_t>(ttl),
                         req.toBytes());

        string nextHopIp = isLocalNetwork(targetIp)
                               ? targetIp
                               : NetUtils::ipToString(gatewayRaw_);

        auto t0 = chrono::steady_clock::now();
        if (arpCache_.hasIp(nextHopIp))
        {
            sendEthernet(arpCache_.getMac(nextHopIp), 0x0800, ipPkt.toBytes());
        }
        else
        {
            arpCache_.enqueuePacket(nextHopIp, ipPkt.toBytes());
            sendArpRequest(nextHopIp);
        }
        auto t1 = chrono::steady_clock::now();
        double rttMs = chrono::duration<double, milli>(t1 - t0).count();

        if (icmpReplyReceived_)
        {
            cout << "  " << ttl << "  " << icmpReplySrcIp_
                 << "  " << fixed << setprecision(2) << rttMs << "ms\n";
            if (icmpReplySrcIp_ == targetIp)
            {
                cout << "[" << getName() << "] Traceroute selesai.\n\n";
                return;
            }
        }
        else
        {
            cout << "  " << ttl << "  * * * (timeout)\n";
        }
    }
    cout << "[" << getName() << "] Max hops tercapai.\n\n";
}

void Host::handleReceive(Interface *inInterface, const vector<uint8_t> &rawBytes)
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
        cerr << "[" << getName() << "] Gagal parse Ethernet: " << e.what() << "\n";
        return;
    }

    string myMac = inInterface->getMacAddress();
    if (frame.destMac != myMac && !frame.isBroadcast())
        return;

    if (frame.etherType == 0x0806)
        handleArpPacket(frame.payload);
    else if (frame.etherType == 0x0800)
        handleIPv4Packet(inInterface, frame.payload);
}

void Host::handleArpPacket(const vector<uint8_t> &arpBytes)
{
    ARPPacket arp;
    try
    {
        arp.deserialize(arpBytes);
    }
    catch (const exception &e)
    {
        return;
    }

    string senderIp = arp.getSenderProtAddr();
    string senderMac = NetUtils::macToString(arp.getSenderHwAddr());
    arpCache_.addEntry(senderIp, senderMac);
    cout << "[" << getName() << "] ARP learn: " << senderIp
         << " -> " << senderMac << "\n";

    if (arp.getOperation() == ARP_OP_REQUEST)
    {
        if (arp.getTargetProtAddr() != getIpAddress())
            return;
        Interface *ifc = getInterface(1);
        if (!ifc)
            return;
        vector<uint8_t> myMacBytes = NetUtils::stringToMac(ifc->getMacAddress());
        ARPPacket reply = ARPPacket::createReply(
            myMacBytes, getIpAddress(), arp.getSenderHwAddr(), senderIp);
        cout << "[" << getName() << "] ARP Reply -> " << senderIp << "\n";
        sendEthernet(senderMac, 0x0806, reply.toBytes());
    }
    else if (arp.getOperation() == ARP_OP_REPLY)
    {
        auto queued = arpCache_.dequeueAll(senderIp);
        for (const auto &pkt : queued)
        {
            cout << "[" << getName() << "] Kirim queued packet ke " << senderIp << "\n";
            sendEthernet(senderMac, 0x0800, pkt);
        }
    }
}

void Host::handleIPv4Packet(Interface *, const vector<uint8_t> &ipBytes)
{
    IPv4Packet ipPkt;
    try
    {
        ipPkt.deserialize(ipBytes);
    }
    catch (const exception &e)
    {
        return;
    }

    if (!ipPkt.verifyChecksum())
    {
        cout << "[" << getName() << "] IPv4 checksum invalid, drop\n";
        return;
    }
    if (ipPkt.dstIP != getIpAddress())
        return;

    cout << "[" << getName() << "] IPv4 diterima dari " << ipPkt.srcIP
         << " proto=" << (int)ipPkt.protocol << "\n";

    if (ipPkt.protocol == IPv4Packet::PROTOCOL_ICMP)
        handleIcmpPacket(ipPkt.srcIP, ipPkt.payload);

    // added by yavie after implementing TCPSocket
    else if (ipPkt.protocol == 6) // 6 = nomor protokol TCP
        handleTCPSegment(ipPkt.srcIP, ipPkt.payload);
}

void Host::handleIcmpPacket(const string &srcIp, const vector<uint8_t> &icmpBytes)
{
    ICMPMessage msg;
    try
    {
        msg.deserialize(icmpBytes);
    }
    catch (const exception &e)
    {
        return;
    }

    if (!msg.isChecksumValid())
    {
        cout << "[" << getName() << "] ICMP checksum invalid, drop\n";
        return;
    }

    cout << "[" << getName() << "] ICMP type=" << (int)msg.getICMPType()
         << " dari " << srcIp << "\n";

    if (msg.getICMPType() == ICMP::TYPE_ECHO_REQUEST)
    {
        ICMPMessage reply = ICMPMessage::buildEchoReply(msg);
        cout << "[" << getName() << "] Kirim ICMP Echo Reply ke " << srcIp << "\n";
        sendLayer3Packet(srcIp, reply.toBytes(), IPv4Packet::PROTOCOL_ICMP);
    }
    else if (msg.getICMPType() == ICMP::TYPE_ECHO_REPLY)
    {
        if (msg.getIdentifier() == pendingIdentifier_)
        {
            icmpReplyReceived_ = true;
            icmpReplySrcIp_ = srcIp;
        }
    }
    else if (msg.getICMPType() == ICMP::TYPE_TIME_EXCEEDED)
    {
        icmpReplyReceived_ = true;
        icmpReplySrcIp_ = srcIp;
        cout << "[" << getName() << "] ICMP Time Exceeded dari " << srcIp << "\n";
    }
    else if (msg.getICMPType() == ICMP::TYPE_UNREACHABLE)
    {
        icmpReplyReceived_ = true;
        icmpReplySrcIp_ = srcIp;
        cout << "[" << getName() << "] ICMP Unreachable dari " << srcIp
             << " code=" << (int)msg.getCode() << "\n";
    }
}

void Host::handleTCPSegment(const string &srcIp, const vector<uint8_t> &tcpBytes)
{
    TCPSegment seg;
    try { seg = TCPSegment::deserialize(tcpBytes); }
    catch (const exception &e) {
        cerr << "[" << getName() << "] Gagal parse TCPSegment: " << e.what() << "\n";
        return;
    }

    // Cari socket yang mendengarkan di destination port ini
    auto it = tcpSockets_.find(seg.dstPort);
    if (it == tcpSockets_.end()) {
        cout << "[" << getName() << "] Tidak ada socket di port "
             << seg.dstPort << ", drop\n";
        return;
    }

    TCPSocket* socket = it->second;

    // Beri tahu socket siapa pengirimnya — inilah yang dimaksud sebelumnya
    socket->remoteIP = srcIp;

    socket->handleIncomingSegment(seg);
}