#include "core/Simulator.hpp"
#include "core/Host.hpp"
#include "layer7/MagiSocket.hpp"
#include "layer7/DHCPMessage.hpp"
#include "layer7/DHCPServer.hpp"
#include "layer7/DHCPClient.hpp"

#include <iostream>
#include <cassert>
#include <string>
#include <vector>

using namespace std;

// ============================================================
//  Helper — cetak header tiap test
// ============================================================
static void printHeader(const string& title) {
    cout << "\n========================================\n";
    cout << "  " << title << "\n";
    cout << "========================================\n";
}

static void printPass(const string& msg) {
    cout << "\033[32m[PASS]\033[0m " << msg << "\n";
}

// ============================================================
//  Test 1 — DHCPMessage serialize & deserialize
//
//  Memastikan DHCPMessage::toBytes() menghasilkan byte array
//  yang kalau di-deserialize kembali menghasilkan objek identik.
//  Ini penting karena seluruh komunikasi DHCP bergantung pada
//  kebenaran serialisasi ini.
// ============================================================
void testDHCPMessageSerializeDeserialize() {
    printHeader("Test 1: DHCPMessage Serialize & Deserialize");

    DHCPMessage msg;
    msg.op     = DHCP_OP_REQUEST;
    msg.htype  = DHCP_HTYPE_ETHERNET;
    msg.hlen   = DHCP_HLEN_ETHERNET;
    msg.hops   = 0;
    msg.xid    = 0xDEADBEEF;
    msg.secs   = 0;
    msg.flags  = 0;
    msg.ciaddr = "0.0.0.0";
    msg.yiaddr = "192.168.1.50";
    msg.siaddr = "192.168.1.1";
    msg.giaddr = "0.0.0.0";
    msg.chaddr = "AA:BB:CC:DD:EE:FF";

    msg.setMessageType(DHCP_MSG_DISCOVER);
    msg.setSubnetMask(24);
    msg.setRouter("192.168.1.1");
    msg.setLeaseTime(86400);
    msg.setServerId("192.168.1.1");

    // Serialize lalu deserialize
    vector<uint8_t> bytes = msg.toBytes();
    assert(!bytes.empty() && "toBytes() tidak boleh menghasilkan array kosong");
    printPass("toBytes() menghasilkan byte array non-empty");

    DHCPMessage parsed = DHCPMessage::deserialize(bytes);

    // Verifikasi field-field utama
    assert(parsed.op    == DHCP_OP_REQUEST      && "op tidak cocok");
    assert(parsed.htype == DHCP_HTYPE_ETHERNET  && "htype tidak cocok");
    assert(parsed.hlen  == DHCP_HLEN_ETHERNET   && "hlen tidak cocok");
    assert(parsed.xid   == 0xDEADBEEF           && "xid tidak cocok");
    assert(parsed.yiaddr == "192.168.1.50"       && "yiaddr tidak cocok");
    assert(parsed.siaddr == "192.168.1.1"        && "siaddr tidak cocok");
    assert(parsed.chaddr == "AA:BB:CC:DD:EE:FF"  && "chaddr tidak cocok");
    printPass("Semua field header ter-deserialize dengan benar");

    // Verifikasi options
    assert(parsed.getMessageType() == DHCP_MSG_DISCOVER && "Message type tidak cocok");
    assert(parsed.getRouter()      == "192.168.1.1"     && "Router option tidak cocok");
    assert(parsed.getServerId()    == "192.168.1.1"     && "Server ID option tidak cocok");
    assert(parsed.getSubnetMask()  == "255.255.255.0"   && "Subnet mask tidak cocok");
    printPass("Semua options ter-deserialize dengan benar");

    cout << "[Test 1] SUKSES\n";
}

// ============================================================
//  Test 2 — DHCPMessage option helpers
//
//  Memastikan setSubnetMask(), setRouter(), setLeaseTime(),
//  setServerId() dan getter-nya bekerja dengan benar untuk
//  berbagai nilai input.
// ============================================================
void testDHCPMessageOptions() {
    printHeader("Test 2: DHCPMessage Option Helpers");

    DHCPMessage msg;

    // Test berbagai prefix length
    msg.setSubnetMask(24);
    assert(msg.getSubnetMask() == "255.255.255.0" && "/24 salah");
    printPass("Subnet mask /24 = 255.255.255.0");

    msg.setSubnetMask(16);
    assert(msg.getSubnetMask() == "255.255.0.0" && "/16 salah");
    printPass("Subnet mask /16 = 255.255.0.0");

    msg.setSubnetMask(8);
    assert(msg.getSubnetMask() == "255.0.0.0" && "/8 salah");
    printPass("Subnet mask /8 = 255.0.0.0");

    // Test message type untuk semua tipe DORA
    msg.setMessageType(DHCP_MSG_DISCOVER);
    assert(msg.getMessageType() == DHCP_MSG_DISCOVER && "Discover salah");
    printPass("Message type DISCOVER benar");

    msg.setMessageType(DHCP_MSG_OFFER);
    assert(msg.getMessageType() == DHCP_MSG_OFFER && "Offer salah");
    printPass("Message type OFFER benar");

    msg.setMessageType(DHCP_MSG_REQUEST);
    assert(msg.getMessageType() == DHCP_MSG_REQUEST && "Request salah");
    printPass("Message type REQUEST benar");

    msg.setMessageType(DHCP_MSG_ACK);
    assert(msg.getMessageType() == DHCP_MSG_ACK && "ACK salah");
    printPass("Message type ACK benar");

    // Test getter saat option tidak ada
    DHCPMessage empty;
    assert(empty.getMessageType() == 0   && "Default message type harus 0");
    assert(empty.getRouter()      == ""  && "Default router harus kosong");
    assert(empty.getServerId()    == ""  && "Default server ID harus kosong");
    assert(empty.getSubnetMask()  == ""  && "Default subnet mask harus kosong");
    printPass("Getter mengembalikan nilai default saat option tidak ada");

    cout << "[Test 2] SUKSES\n";
}

// ============================================================
//  Test 3 — Magic cookie validation
//
//  Memastikan deserialize() melempar exception jika magic
//  cookie tidak valid — ini adalah guard terhadap paket korup
//  atau paket BOOTP yang bukan DHCP.
// ============================================================
void testMagicCookieValidation() {
    printHeader("Test 3: Magic Cookie Validation");

    // Buat paket valid dulu
    DHCPMessage msg;
    msg.op     = DHCP_OP_REQUEST;
    msg.htype  = DHCP_HTYPE_ETHERNET;
    msg.hlen   = DHCP_HLEN_ETHERNET;
    msg.hops   = 0;
    msg.xid    = 0x12345678;
    msg.ciaddr = msg.yiaddr = msg.siaddr = msg.giaddr = "0.0.0.0";
    msg.chaddr = "00:11:22:33:44:55";
    msg.setMessageType(DHCP_MSG_DISCOVER);

    vector<uint8_t> bytes = msg.toBytes();

    // Korupsi magic cookie (byte 236-239)
    // Posisi: 4(op+htype+hlen+hops) + 4(xid) + 2(secs) + 2(flags)
    //       + 16(4 IP fields) + 16(chaddr) + 64(sname) + 128(file) = 236
    bytes[236] = 0x00;
    bytes[237] = 0x00;
    bytes[238] = 0x00;
    bytes[239] = 0x00;

    bool exceptionThrown = false;
    try {
        DHCPMessage::deserialize(bytes);
    } catch (const invalid_argument&) {
        exceptionThrown = true;
    }

    assert(exceptionThrown && "Harus throw exception untuk magic cookie invalid");
    printPass("Magic cookie invalid dideteksi dan exception dilempar");

    // Pastikan paket asli (tidak dikorupsi) tetap valid
    vector<uint8_t> validBytes = msg.toBytes();
    bool noException = false;
    try {
        DHCPMessage::deserialize(validBytes);
        noException = true;
    } catch (...) {
        noException = false;
    }
    assert(noException && "Paket valid tidak boleh throw exception");
    printPass("Paket dengan magic cookie valid berhasil di-deserialize");

    cout << "[Test 3] SUKSES\n";
}

// ============================================================
//  Test 4 — DORA flow lengkap (DHCPServer + DHCPClient)
//
//  Ini adalah integration test utama — memastikan seluruh
//  alur DORA bekerja end-to-end:
//  Client mulai dengan IP "0.0.0.0", setelah discover()
//  selesai IP-nya harus sudah berubah ke IP dari pool server.
// ============================================================
void testDORAFlow() {
    printHeader("Test 4: DORA Flow (DHCPServer + DHCPClient)");

    // Setup topologi: Client -- Server
    Simulator sim;
    Host* server = dynamic_cast<Host*>(sim.createNode("host", "DHCPServer"));
    Host* client = dynamic_cast<Host*>(sim.createNode("host", "DHCPClient"));

    sim.linkNodes("DHCPServer:1", "DHCPClient:1", 0);

    // Set IP server secara statis (server harus punya IP tetap)
    server->setIpAddress("192.168.1.1/24");
    server->setDefaultGateway("192.168.1.1");

    // Client mulai tanpa IP yang valid
    client->setIpAddress("0.0.0.0/0");

    cout << "\n[Setup] Server IP: " << server->getIpAddress() << "\n";
    cout << "[Setup] Client IP sebelum DHCP: " << client->getIpAddress() << "\n";

    // Inisialisasi dan jalankan DHCP Server
    // Pool: 192.168.1.100 - 192.168.1.200, gateway 192.168.1.1, prefix /24
    DHCPServer dhcpServer(server,
                          "192.168.1.100",
                          "192.168.1.200",
                          "192.168.1.1",
                          24);
    dhcpServer.start();

    // Jalankan DHCP Client
    DHCPClient dhcpClient(client);
    bool success = dhcpClient.discover();

    assert(success && "DORA flow harus berhasil");
    printPass("DORA flow selesai tanpa error");

    // Verifikasi IP client sudah berubah ke IP dari pool
    string clientIp = client->getIpAddress();
    cout << "\n[Hasil] Client IP setelah DHCP: " << clientIp << "\n";

    assert(clientIp != "0.0.0.0" && "IP client tidak boleh tetap 0.0.0.0");
    printPass("IP client berhasil diubah dari 0.0.0.0");

    // Verifikasi IP yang didapat ada dalam range pool (192.168.1.100-200)
    // Ambil octet terakhir dari IP
    int lastOctet = stoi(clientIp.substr(clientIp.rfind('.') + 1));
    assert(lastOctet >= 100 && lastOctet <= 200 &&
           "IP yang didapat harus dalam range pool");
    printPass("IP " + clientIp + " berada dalam range pool 192.168.1.100-200");

    // Verifikasi gateway juga sudah diset
    string gw = client->getDefaultGateway();
    assert(gw == "192.168.1.1" && "Gateway harus diset ke 192.168.1.1");
    printPass("Default gateway berhasil diset ke " + gw);

    cout << "[Test 4] SUKSES\n";
}

// ============================================================
//  Test 5 — Dua client mendapat IP berbeda
//
//  Memastikan server tidak memberikan IP yang sama ke dua
//  client berbeda (tidak terjadi IP conflict).
// ============================================================
void testMultipleClients() {
    printHeader("Test 5: Multiple Clients — IP Unik per Client");

    Simulator sim;
    Host* server  = dynamic_cast<Host*>(sim.createNode("host", "Server"));
    Host* clientA = dynamic_cast<Host*>(sim.createNode("host", "ClientA"));
    Host* clientB = dynamic_cast<Host*>(sim.createNode("host", "ClientB"));

    // Topologi: ClientA -- Server -- ClientB
    sim.linkNodes("Server:1", "ClientA:1", 0);
    sim.linkNodes("Server:2", "ClientB:1", 0);

    server->setIpAddress("192.168.1.1/24");
    clientA->setIpAddress("0.0.0.0/0");
    clientB->setIpAddress("0.0.0.0/0");

    DHCPServer dhcpServer(server,
                          "192.168.1.100",
                          "192.168.1.200",
                          "192.168.1.1",
                          24);
    dhcpServer.start();

    // Client A discover dulu
    DHCPClient dhcpClientA(clientA);
    bool successA = dhcpClientA.discover();
    assert(successA && "Client A harus berhasil mendapat IP");
    printPass("Client A berhasil mendapat IP: " + clientA->getIpAddress());

    // Client B discover setelahnya
    DHCPClient dhcpClientB(clientB);
    bool successB = dhcpClientB.discover();
    assert(successB && "Client B harus berhasil mendapat IP");
    printPass("Client B berhasil mendapat IP: " + clientB->getIpAddress());

    // IP keduanya harus berbeda
    assert(clientA->getIpAddress() != clientB->getIpAddress() &&
           "Dua client tidak boleh mendapat IP yang sama");
    printPass("IP Client A (" + clientA->getIpAddress() +
              ") berbeda dari Client B (" + clientB->getIpAddress() + ")");

    cout << "[Test 5] SUKSES\n";
}

// ============================================================
//  Test 6 — Client yang sama selalu dapat IP yang sama
//
//  Memastikan server mengembalikan IP yang sama jika client
//  dengan MAC address yang sama melakukan discover ulang
//  (simulasi DHCP renewal / reconnect).
// ============================================================
void testConsistentIpForSameClient() {
    printHeader("Test 6: Client yang Sama Selalu Dapat IP yang Sama");

    Simulator sim;
    Host* server = dynamic_cast<Host*>(sim.createNode("host", "Server"));
    Host* client = dynamic_cast<Host*>(sim.createNode("host", "Client"));

    sim.linkNodes("Server:1", "Client:1", 0);
    server->setIpAddress("192.168.1.1/24");
    client->setIpAddress("0.0.0.0/0");

    DHCPServer dhcpServer(server,
                          "192.168.1.100",
                          "192.168.1.200",
                          "192.168.1.1",
                          24);
    dhcpServer.start();

    // Discover pertama
    DHCPClient dhcpClient1(client);
    bool success1 = dhcpClient1.discover();
    assert(success1 && "Discover pertama harus berhasil");
    string firstIp = client->getIpAddress();
    printPass("Discover pertama mendapat IP: " + firstIp);

    // Reset IP client lalu discover lagi (simulasi reconnect)
    client->setIpAddress("0.0.0.0/0");

    DHCPClient dhcpClient2(client);
    bool success2 = dhcpClient2.discover();
    assert(success2 && "Discover kedua harus berhasil");
    string secondIp = client->getIpAddress();
    printPass("Discover kedua mendapat IP: " + secondIp);

    assert(firstIp == secondIp &&
           "Client yang sama harus mendapat IP yang sama");
    printPass("IP konsisten untuk client yang sama: " + firstIp);

    cout << "[Test 6] SUKSES\n";
}

// ============================================================
//  Test 7 — UDP routing patch (handleUDPSegment)
//
//  Memastikan paket UDP yang masuk ke host diteruskan ke
//  socket yang tepat berdasarkan dstPort, dan paket ke port
//  yang tidak ada socket-nya di-drop dengan benar.
// ============================================================
void testUDPRouting() {
    printHeader("Test 7: UDP Routing Patch (handleUDPSegment)");

    Simulator sim;
    Host* hostA = dynamic_cast<Host*>(sim.createNode("host", "HostA"));
    Host* hostB = dynamic_cast<Host*>(sim.createNode("host", "HostB"));

    sim.linkNodes("HostA:1", "HostB:1", 0);
    hostA->setIpAddress("10.0.0.1/24");
    hostB->setIpAddress("10.0.0.2/24");

    // Buat UDP socket di HostB port 5000
    MagiSocket* sockB = new MagiSocket(hostB, MAGI_SOCK_DGRAM);
    sockB->bind("10.0.0.2", 5000);

    // Kirim UDP dari HostA ke HostB:5000
    string msg = "halo UDP";
    vector<uint8_t> payload(msg.begin(), msg.end());
    hostA->sendUDP("10.0.0.2", 9999, 5000, payload);

    // Baca dari socket HostB
    vector<uint8_t> received = sockB->recv(1024);
    string receivedStr(received.begin(), received.end());

    assert(receivedStr == msg && "Data UDP harus sampai ke socket yang tepat");
    printPass("Data UDP sampai ke socket port 5000: \"" + receivedStr + "\"");

    // Kirim ke port yang tidak ada socket-nya — harus di-drop (tidak crash)
    hostA->sendUDP("10.0.0.2", 9999, 9999, payload);
    printPass("Paket ke port tanpa socket di-drop tanpa crash");

    sockB->close();
    delete sockB;

    cout << "[Test 7] SUKSES\n";
}

// ============================================================
//  main
// ============================================================
int main() {
    cout << "\n========================================\n";
    cout << "  Unit Test — DHCP (P2)\n";
    cout << "  DHCPMessage, DHCPServer, DHCPClient\n";
    cout << "  UDP Routing Patch\n";
    cout << "========================================\n";

    try {
        testDHCPMessageSerializeDeserialize();
        testDHCPMessageOptions();
        testMagicCookieValidation();
        testDORAFlow();
        testMultipleClients();
        testConsistentIpForSameClient();
        testUDPRouting();
    }
    catch (const exception& e) {
        throw std::runtime_error("Unhandled exception: " + std::string(e.what()));
    }
    catch (...)
    {
        throw std::runtime_error("Unknown exception");
    }

    cout << "\n========================================\n";
    cout << "\033[32m  Semua test DHCP LULUS!\033[0m\n";
    cout << "========================================\n\n";

    return 0;
}