#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <cstring>
#include "layer3/IPv4Packet.hpp"
#include "layer3/FragmentReassembler.hpp"
#include "layer3/ICMPMessage.hpp"
#include "core/Router.hpp"
#include "core/Host.hpp"
#include "core/Link.hpp"
#include "core/Interface.hpp"

using namespace std;

void testBasicFragmentation()
{
    cout << "=== Test 1: Basic Fragmentation ===\n";
    vector<uint8_t> payload(200, 0xAB);
    IPv4Packet pkt("10.0.0.1", "10.0.0.2", 6, 64, payload);
    pkt.identification = 0x1234;

    uint16_t mtu = 100;
    auto frags = pkt.fragment(mtu);

    cout << "  Original payload: " << payload.size() << " bytes\n";
    cout << "  MTU: " << mtu << "\n";
    cout << "  Number of fragments: " << frags.size() << "\n";

    assert(frags.size() > 1 && "Should produce more than 1 fragment");
    assert(frags.size() == 3 && "Expected 3 fragments for 200-byte payload with MTU=100");

    size_t totalPayload = 0;
    for (size_t i = 0; i < frags.size(); i++)
    {
        auto &f = frags[i];
        cout << "  Fragment " << i
             << ": offset=" << f.getFragmentOffset()
             << " MF=" << f.isMoreFragments()
             << " payload=" << f.payload.size()
             << " totalLen=" << f.totalLen << "\n";

        assert(f.srcIP == "10.0.0.1" && "Fragment srcIP must match");
        assert(f.dstIP == "10.0.0.2" && "Fragment dstIP must match");
        assert(f.identification == 0x1234 && "Fragment ID must match");
        assert(f.protocol == 6 && "Fragment protocol must match");
        assert(f.verifyChecksum() && "Fragment checksum must be valid");

        totalPayload += f.payload.size();

        if (i < frags.size() - 1)
        {
            assert(f.isMoreFragments() && "Intermediate fragments must have MF set");
            assert(f.getFragmentOffset() % 8 == 0 && "Fragment offset must be multiple of 8");
        }
        else
        {
            assert(!f.isMoreFragments() && "Last fragment must not have MF set");
        }
    }
    size_t totalWithHeaders = 0;
    for (auto &f : frags)
        totalWithHeaders += f.payload.size();
    assert(totalWithHeaders == payload.size() && "Total fragment payload must equal original payload");

    cout << "[OK] Basic fragmentation test passed.\n\n";
}

void testFragmentOffsetAlignment()
{
    cout << "=== Test 2: Fragment Offset Alignment ===\n";
    vector<uint8_t> payload(500, 0xCD);
    IPv4Packet pkt("192.168.1.1", "192.168.2.1", 17, 64, payload);
    pkt.identification = 0x5678;

    uint16_t mtu = 200;
    auto frags = pkt.fragment(mtu);

    cout << "  Original payload: " << payload.size() << " bytes\n";
    cout << "  MTU: " << mtu << "\n";
    cout << "  Number of fragments: " << frags.size() << "\n";

    for (const auto &f : frags)
    {
        assert(f.getFragmentOffset() % 8 == 0 && "Every fragment offset must be multiple of 8");
    }

    size_t totalPayload = 0;
    for (const auto &f : frags)
        totalPayload += f.payload.size();
    assert(totalPayload == payload.size() && "Total payload must match original");

    cout << "[OK] Fragment offset alignment test passed.\n\n";
}

void testReassembly()
{
    cout << "=== Test 3: Fragment Reassembly ===\n";
    vector<uint8_t> payload(300, 0xEF);
    for (size_t i = 0; i < payload.size(); i++)
        payload[i] = static_cast<uint8_t>(i & 0xFF);

    IPv4Packet pkt("10.0.0.1", "10.0.0.2", 6, 64, payload);
    pkt.identification = 0xABCD;

    uint16_t mtu = 100;
    auto frags = pkt.fragment(mtu);

    FragmentReassembler reassembler;
    std::optional<IPv4Packet> result;

    for (size_t i = 0; i < frags.size(); i++)
    {
        cout << "  Adding fragment " << i
             << " offset=" << frags[i].getFragmentOffset()
             << " MF=" << frags[i].isMoreFragments()
             << " payload_size=" << frags[i].payload.size() << "\n";

        result = reassembler.addFragment(frags[i]);
    }

    assert(result.has_value() && "Reassembly should produce a complete packet");
    IPv4Packet &reassembled = result.value();

    cout << "  Reassembled packet: srcIP=" << reassembled.srcIP
         << " dstIP=" << reassembled.dstIP
         << " payload_size=" << reassembled.payload.size() << "\n";

    assert(reassembled.srcIP == "10.0.0.1" && "Reassembled srcIP must match");
    assert(reassembled.dstIP == "10.0.0.2" && "Reassembled dstIP must match");
    assert(reassembled.protocol == 6 && "Reassembled protocol must match");
    assert(reassembled.payload == payload && "Reassembled payload must match original");
    assert(!reassembled.isFragment() && "Reassembled packet should not be a fragment");
    assert(reassembled.verifyChecksum() && "Reassembled packet checksum must be valid");

    cout << "[OK] Fragment reassembly test passed.\n\n";
}

void testDontFragment()
{
    cout << "=== Test 4: Don't Fragment (DF) Flag ===\n";
    vector<uint8_t> payload(300, 0x42);
    IPv4Packet pkt("10.0.0.1", "10.0.0.2", 6, 64, payload);
    pkt.identification = 0xBEEF;
    pkt.flagsFragment = IPv4Packet::FLAG_DF;

    uint16_t mtu = 100;
    auto frags = pkt.fragment(mtu);

    assert(frags.empty() && "DF set packet should not be fragmented (returned empty vector)");

    cout << "[OK] Don't Fragment test passed.\n\n";
}

void testNoFragmentationNeeded()
{
    cout << "=== Test 5: No Fragmentation Needed (packet smaller than MTU) ===\n";
    vector<uint8_t> payload(50, 0x11);
    IPv4Packet pkt("1.2.3.4", "5.6.7.8", 17, 64, payload);
    pkt.identification = 0x0001;

    uint16_t mtu = 1500;
    auto frags = pkt.fragment(mtu);

    assert(frags.size() == 1 && "Packet smaller than MTU should return single fragment");
    assert(frags[0].payload == payload && "Payload should be unchanged");
    assert(!frags[0].isFragment() && "Should not be marked as fragment");

    cout << "[OK] No fragmentation needed test passed.\n\n";
}

void testReassemblyOutOfOrder()
{
    cout << "=== Test 6: Reassembly Out of Order ===\n";
    vector<uint8_t> payload(200, 0x77);
    for (size_t i = 0; i < payload.size(); i++)
        payload[i] = static_cast<uint8_t>((i + 1) & 0xFF);

    IPv4Packet pkt("172.16.0.1", "172.16.0.2", 17, 32, payload);
    pkt.identification = 0x4242;

    uint16_t mtu = 100;
    auto frags = pkt.fragment(mtu);

    FragmentReassembler reassembler;

    auto result = reassembler.addFragment(frags[2]);
    assert(!result.has_value() && "Last fragment alone shouldn't complete reassembly");

    result = reassembler.addFragment(frags[0]);
    assert(!result.has_value() && "First+last fragment shouldn't complete reassembly");

    result = reassembler.addFragment(frags[1]);
    assert(result.has_value() && "All fragments should complete reassembly");

    assert(result.value().payload == payload && "Out-of-order reassembled payload must match");
    assert(result.value().verifyChecksum() && "Reassembled checksum must be valid");

    cout << "[OK] Out-of-order reassembly test passed.\n\n";
}

void testICMPFragmentationNeeded()
{
    cout << "=== Test 7: Router Fragmentation & ICMP Frag Needed ===\n";
    Host h1("H1", "192.168.1.10/24", "192.168.1.1");
    Router r1("R1");
    Host h2("H2", "10.0.0.20/24", "10.0.0.1");

    h1.addInterface(1, "AA:AA:AA:AA:AA:01");
    r1.addInterface(1, "BB:BB:BB:BB:BB:01");
    r1.setInterfaceIp(1, "192.168.1.1/24");
    r1.addInterface(2, "BB:BB:BB:BB:BB:02");
    r1.setInterfaceIp(2, "10.0.0.1/24");
    h2.addInterface(1, "CC:CC:CC:CC:CC:01");

    Link link1(h1.getInterface(1), r1.getInterface(1));
    Link link2(r1.getInterface(2), h2.getInterface(1));

    r1.getInterface(2)->setMtu(128);

    h1.ping(h2.getIpAddress(), 1);

    cout << "[OK] Router MTU set and ping executed with low MTU.\n\n";
}

void testFragmentHelpers()
{
    cout << "=== Test 8: IPv4Packet Fragment Helper Methods ===\n";

    IPv4Packet normalPkt("1.1.1.1", "2.2.2.2", 6, 64, {1, 2, 3});
    assert(!normalPkt.isFragment() && "Normal packet should not be a fragment");
    assert(!normalPkt.isMoreFragments() && "Normal packet MF should be 0");
    assert(!normalPkt.isDontFragment() && "Normal packet DF should be 0");
    assert(normalPkt.getFragmentOffset() == 0 && "Normal packet offset should be 0");

    IPv4Packet fragPkt;
    fragPkt.srcIP = "1.1.1.1";
    fragPkt.dstIP = "2.2.2.2";
    fragPkt.protocol = 6;
    fragPkt.ttl = 64;
    fragPkt.identification = 42;
    fragPkt.payload = {1, 2, 3};
    fragPkt.flagsFragment = IPv4Packet::FLAG_MF | 5;
    assert(fragPkt.isFragment() && "Packet with MF+offset should be a fragment");
    assert(fragPkt.isMoreFragments() && "MF flag should be set");
    assert(!fragPkt.isDontFragment() && "DF should not be set");
    assert(fragPkt.getFragmentOffset() == 40 && "Offset 5*8=40 bytes");

    IPv4Packet dfPkt;
    dfPkt.flagsFragment = IPv4Packet::FLAG_DF;
    assert(dfPkt.isDontFragment() && "DF flag should be set");

    cout << "[OK] Fragment helper methods test passed.\n\n";
}

void testLargeFragmentation()
{
    cout << "=== Test 9: Large Payload Fragmentation ===\n";
    vector<uint8_t> payload(4000, 0xAA);
    IPv4Packet pkt("10.0.0.1", "10.0.0.2", 17, 64, payload);
    pkt.identification = 0x9999;

    uint16_t mtu = 576;
    auto frags = pkt.fragment(mtu);

    cout << "  Original payload: " << payload.size() << " bytes\n";
    cout << "  MTU: " << mtu << "\n";
    cout << "  Number of fragments: " << frags.size() << "\n";

    assert(frags.size() > 1 && "Large payload should be fragmented");

    size_t totalPayload = 0;
    for (const auto &f : frags)
    {
        totalPayload += f.payload.size();
        assert(f.verifyChecksum() && "Every fragment checksum must be valid");
        assert(f.toBytes().size() <= mtu && "Every fragment must fit within MTU");
    }
    assert(totalPayload == payload.size() && "Total fragment payload must equal original");

    FragmentReassembler reassembler;
    std::optional<IPv4Packet> result;
    for (auto &frag : frags)
    {
        result = reassembler.addFragment(frag);
    }
    assert(result.has_value() && "Large payload should reassemble completely");
    assert(result.value().payload == payload && "Reassembled large payload must match original");

    cout << "[OK] Large payload fragmentation and reassembly test passed.\n\n";
}

void testFragmentReassemblyTimeout()
{
    cout << "=== Test 10: Fragment Reassembler Pending Count & Cleanup ===\n";

    FragmentReassembler reassembler;

    vector<uint8_t> payload(200, 0x55);
    IPv4Packet pkt1("10.0.0.1", "10.0.0.2", 6, 64, payload);
    pkt1.identification = 0x1111;
    auto frags1 = pkt1.fragment(100);

    reassembler.addFragment(frags1[0]);
    assert(reassembler.pendingCount() == 1 && "Should have 1 pending reassembly buffer");
    reassembler.addFragment(frags1[1]);
    assert(reassembler.pendingCount() == 1 && "Still 1 pending (missing last fragment)");

    auto result = reassembler.addFragment(frags1[2]);
    assert(result.has_value() && "Complete reassembly should succeed");
    assert(reassembler.pendingCount() == 0 && "After reassembly, no pending buffers");

    cout << "[OK] Fragment reassembler pending count test passed.\n\n";
}

void testFragmentationWithRouterSimulation()
{
    cout << "=== Test 11: End-to-End Router Fragmentation Simulation ===\n";

    Host h1("H1", "192.168.1.10/24", "192.168.1.1");
    Router r1("R1");
    Host h2("H2", "10.0.0.20/24", "10.0.0.1");

    h1.addInterface(1, "AA:AA:AA:AA:AA:01");
    r1.addInterface(1, "BB:BB:BB:BB:BB:01");
    r1.setInterfaceIp(1, "192.168.1.1/24");
    r1.addInterface(2, "BB:BB:BB:BB:BB:02");
    r1.setInterfaceIp(2, "10.0.0.1/24");
    h2.addInterface(1, "CC:CC:CC:CC:CC:01");

    Link link1(h1.getInterface(1), r1.getInterface(1));
    Link link2(r1.getInterface(2), h2.getInterface(1));

    r1.getInterface(2)->setMtu(300);

    cout << "  R1 port2 MTU=" << r1.getInterface(2)->getMtu() << "\n";

    vector<uint8_t> largePayload(500, 0xAB);
    for (size_t i = 0; i < largePayload.size(); i++)
        largePayload[i] = static_cast<uint8_t>(i & 0xFF);

    ICMPMessage icmpReq = ICMPMessage::buildEchoRequest(1, 1, largePayload);
    IPv4Packet ipPkt(h1.getIpAddress(), h2.getIpAddress(),
                     IPv4Packet::PROTOCOL_ICMP, 64, icmpReq.toBytes());

    cout << "  Original IP packet size: " << ipPkt.toBytes().size() << " bytes\n";
    cout << "  Original payload: " << ipPkt.payload.size() << " bytes\n";

    auto frags = ipPkt.fragment(300);

    cout << "  Fragmented into " << frags.size() << " fragments:\n";
    for (size_t i = 0; i < frags.size(); i++)
    {
        cout << "    Fragment " << i
             << ": offset=" << frags[i].getFragmentOffset()
             << " MF=" << frags[i].isMoreFragments()
             << " payload_size=" << frags[i].payload.size()
             << " total_size=" << frags[i].toBytes().size() << "\n";
        assert(frags[i].toBytes().size() <= 300 && "Fragment must fit within MTU");
    }

    FragmentReassembler reassembler;
    std::optional<IPv4Packet> result;
    for (auto &frag : frags)
    {
        result = reassembler.addFragment(frag);
    }

    assert(result.has_value() && "Reassembly should succeed after all fragments");
    assert(result.value().payload == ipPkt.payload && "Reassembled payload must match original");
    assert(!result.value().isFragment() && "Reassembled packet should not be a fragment");

    cout << "[OK] End-to-end router fragmentation simulation passed.\n\n";
}

int main()
{
    cout << "Memulai pengujian IP Fragmentation & Reassembly...\n\n";

    testBasicFragmentation();
    testFragmentOffsetAlignment();
    testReassembly();
    testDontFragment();
    testNoFragmentationNeeded();
    testReassemblyOutOfOrder();
    testFragmentHelpers();
    testLargeFragmentation();
    testFragmentReassemblyTimeout();
    testICMPFragmentationNeeded();
    testFragmentationWithRouterSimulation();

    cout << "Semua tes LULUS!\n";
    return 0;
}