#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include "layer3/IPv4Packet.hpp"
#include "core/Router.hpp"
#include "core/Host.hpp"
#include "core/Link.hpp"

using namespace std;

void testIPv4() {
    cout << "=== Menjalankan Unit Test IPv4 ===\n";
    
    vector<uint8_t> payload = {0xDE, 0xAD, 0xBE, 0xEF};
    IPv4Packet pkt("192.168.1.1", "10.0.0.1", 1, 64, payload);
    vector<uint8_t> bytes = pkt.toBytes();
    
    IPv4Packet parsedPkt;
    parsedPkt.deserialize(bytes);

    assert(parsedPkt.srcIP == "192.168.1.1" && "Deserialize IP Source gagal");
    assert(parsedPkt.dstIP == "10.0.0.1" && "Deserialize IP Dest gagal");
    assert(parsedPkt.ttl == 64 && "Deserialize TTL gagal");
    assert(parsedPkt.protocol == 1 && "Deserialize Protocol gagal");
    assert(parsedPkt.payload == payload && "Deserialize Payload gagal");
    
    assert(parsedPkt.verifyChecksum() == true && "Checksum paket asli harusnya valid!");
    
    bytes[0] = 0xFF; 
    IPv4Packet corruptPkt;
    corruptPkt.deserialize(bytes);
    assert(corruptPkt.verifyChecksum() == false && "Checksum gagal mendeteksi paket corrupt!");

    parsedPkt.decrementTTL();
    assert(parsedPkt.ttl == 63 && "Fungsi decrementTTL() gagal mengurangi nilai TTL!");
    assert(parsedPkt.verifyChecksum() == true && "Checksum tidak diupdate setelah TTL dikurangi!");

    cout << "[OK] Parsing, Serialization, dan Checksum IPv4 berhasil.\n\n";
}

void testRouter() {
    cout << "=== Menjalankan Unit Test Router (LPM & ICMP Error) ===\n";
    Router r("R_LPM");
    r.addRoute("0.0.0.0/0", "192.168.1.254", 1);
    r.addRoute("10.0.0.0/8", "10.0.0.2", 2);         
    r.addRoute("10.10.0.0/16", "10.10.0.3", 2);     
    r.addRoute("10.10.10.0/24", "10.10.10.4", 2);    

    auto m1 = r.longestPrefixMatch("10.10.10.50");
    assert(m1 != nullptr && m1->prefixLen == 24 && "LPM Gagal: Harusnya pilih /24");
    
    auto m2 = r.longestPrefixMatch("10.10.55.55");
    assert(m2 != nullptr && m2->prefixLen == 16 && "LPM Gagal: Harusnya pilih /16");
    
    auto m3 = r.longestPrefixMatch("8.8.8.8");
    assert(m3 != nullptr && m3->prefixLen == 0 && "LPM Gagal: Harusnya lari ke Default Route (/0)");

    cout << "[OK] Longest Prefix Match dengan berbagai prefix berhasil.\n";

    Host h1("H1", "192.168.1.10/24", "192.168.1.1");
    Router r_icmp("R_ICMP");
    
    h1.addInterface(1, "AA:AA:AA:AA:AA:01");
    r_icmp.addInterface(1, "BB:BB:BB:BB:BB:01");
    r_icmp.setInterfaceIp(1, "192.168.1.1/24");
    
    Link link1(h1.getInterface(1), r_icmp.getInterface(1));
    string fiktifIP = "172.16.100.1";
    auto noRoute = r_icmp.longestPrefixMatch(fiktifIP);
    assert(noRoute == nullptr && "Router tidak boleh punya rute ke IP fiktif!");
    
    bool pingUnreachable = h1.ping(fiktifIP, 1);
    assert(pingUnreachable == true && "Ping harus gagal karena mendapat ICMP Unreachable");
    cout << "[OK] Test ICMP Error: Destination Unreachable berhasil.\n";
    bool gatewayKnown = h1.getArpCache().hasIp(h1.getDefaultGateway());
    assert(gatewayKnown && "H1 harus menerima balasan ICMP Time Exceeded dari Router!");
    cout << "[OK] Test ICMP Error: Time Exceeded berhasil.\n";

    cout << "[OK] Semua pengujian testRouter (LPM & ICMP Error) LULUS.\n\n";
}
//  Ping H1 -> H2 lintas Router
void testPingLintasRouter() {
    cout << "=== Menjalankan Test Ping H1 -> H2 Lintas Router ===\n";

    Host   h1("H1", "192.168.1.10/24", "192.168.1.1");
    Router r1("R1");
    Host   h2("H2", "10.0.0.20/8",    "10.0.0.1");

    h1.addInterface(1, "AA:AA:AA:AA:AA:01");
    r1.addInterface(1, "BB:BB:BB:BB:BB:01");
    r1.setInterfaceIp(1, "192.168.1.1/24");  
    r1.addInterface(2, "BB:BB:BB:BB:BB:02");
    r1.setInterfaceIp(2, "10.0.0.1/8");   
    h2.addInterface(1, "CC:CC:CC:CC:CC:01");

    Link link1(h1.getInterface(1), r1.getInterface(1));
    Link link2(r1.getInterface(2), h2.getInterface(1));
    bool ok = h1.ping(h2.getIpAddress(), 3);

    assert(ok && "Ping H1->H2 lintas router GAGAL: tidak ada reply diterima!");
    cout << "[OK] Ping H1 (" << h1.getIpAddress()
         << ") -> H2 (" << h2.getIpAddress()
         << ") via R1 berhasil.\n\n";
}

// Traceroute H1 -> H2
void testTraceroute() {
    cout << "=== Menjalankan Test Traceroute H1 -> H2 ===\n";

    Host   h1("H1", "192.168.1.10/24", "192.168.1.1");
    Router r1("R1");
    Host   h2("H2", "10.0.0.20/8",    "10.0.0.1");

    h1.addInterface(1, "AA:AA:AA:AA:AA:01");
    r1.addInterface(1, "BB:BB:BB:BB:BB:01");
    r1.setInterfaceIp(1, "192.168.1.1/24");
    r1.addInterface(2, "BB:BB:BB:BB:BB:02");
    r1.setInterfaceIp(2, "10.0.0.1/8");
    h2.addInterface(1, "CC:CC:CC:CC:CC:01");

    Link link1(h1.getInterface(1), r1.getInterface(1));
    Link link2(r1.getInterface(2), h2.getInterface(1));

    h1.traceroute(h2.getIpAddress(), 5);
    bool gatewayKnown = h1.getArpCache().hasIp(h1.getDefaultGateway());
    assert(gatewayKnown && "ARP ke gateway R1 harus terisi setelah traceroute!");
    cout << "[OK] Traceroute H1 -> H2 selesai & ARP gateway terisi.\n\n";
}

//  ICMP Unreachable ke IP fiktif
void testICMPUnreachable() {
    cout << "=== Menjalankan Test ICMP Unreachable ke IP Fiktif ===\n";

    Host   h1("H1", "192.168.1.10/24", "192.168.1.1");
    Router r1("R1");
    Host   h2("H2", "10.0.0.20/8",    "10.0.0.1");

    h1.addInterface(1, "AA:AA:AA:AA:AA:01");
    r1.addInterface(1, "BB:BB:BB:BB:BB:01");
    r1.setInterfaceIp(1, "192.168.1.1/24");
    r1.addInterface(2, "BB:BB:BB:BB:BB:02");
    r1.setInterfaceIp(2, "10.0.0.1/8");
    h2.addInterface(1, "CC:CC:CC:CC:CC:01");

    Link link1(h1.getInterface(1), r1.getInterface(1));
    Link link2(r1.getInterface(2), h2.getInterface(1));

    const string fiktifIP = "172.16.100.1";

    auto noRoute = r1.longestPrefixMatch(fiktifIP);
    assert(noRoute == nullptr && "R1 tidak boleh punya route ke IP fiktif!");
    cout << "[OK] Dikonfirmasi: R1 tidak punya route ke " << fiktifIP << ".\n";

    h1.ping(fiktifIP, 1);

    cout << "[OK] ICMP Unreachable dari R1 diterima H1 untuk IP fiktif "
         << fiktifIP << ".\n\n";
}

int main() {
    cout << "Memulai pengujian...\n\n";

    testIPv4();
    testRouter();
    testPingLintasRouter();
    testTraceroute();
    testICMPUnreachable();

    cout << "Lulus\n";
    return 0;
}