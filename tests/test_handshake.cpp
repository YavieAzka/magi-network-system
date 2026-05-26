// test_handshake.cpp
#include <iostream>
#include <cassert>
#include "layer4/TCPSocket.hpp"

TCPSegment makeSegment(
    uint16_t srcPort, uint16_t dstPort,
    uint32_t seq,     uint32_t ack,
    uint8_t  flags
) {
    TCPSegment seg;
    seg.srcPort    = srcPort;
    seg.dstPort    = dstPort;
    seg.seqNum     = seq;
    seg.ackNum     = ack;
    seg.flags      = flags;
    seg.windowSize = 65535;
    seg.dataOffset = 5;
    return seg;
}
// Test 1 — Alur normal: client activeOpen, server passiveOpen
void test_handshake_normal() {
    std::cout << "\n===== Test: 3-Way Handshake Normal =====\n";

    TCPSegment synFromClient;
    TCPSegment synAckFromServer;
    TCPSegment ackFromClient;

    TCPSocket client(
        "192.168.1.10", 54321,
        [&](const TCPSegment& seg, const std::string&, uint16_t) {
            // Simpan ke variabel yang sesuai berdasarkan flags
            if (seg.flags == TCP_FLAG_SYN)
                synFromClient = seg;
            else if (seg.flags == TCP_FLAG_ACK)
                ackFromClient = seg;
        }
    );

    TCPSocket server(
        "192.168.1.20", 80,
        [&](const TCPSegment& seg, const std::string&, uint16_t) {
            synAckFromServer = seg;
        }
    );
    server.remoteIP = "192.168.1.10";

    // Langkah 1: server mulai mendengarkan
    server.passiveOpen();
    assert(server.state == TCPState::LISTEN);

    // Langkah 2: client kirim SYN
    client.activeOpen("192.168.1.20", 80);
    assert(client.state == TCPState::SYN_SENT);

    // Langkah 3: server terima SYN, kirim SYN-ACK
    server.handleIncomingSegment(synFromClient);
    assert(server.state == TCPState::SYN_RCVD); // ← harus SYN_RCVD dulu

    // Langkah 4: client terima SYN-ACK, kirim ACK
    client.handleIncomingSegment(synAckFromServer);
    assert(client.state == TCPState::ESTABLISHED);

    // Langkah 5: server terima ACK, handshake selesai
    server.handleIncomingSegment(ackFromClient);
    assert(server.state == TCPState::ESTABLISHED);

    std::cout << "[PASS] Kedua sisi mencapai ESTABLISHED\n";
}
// Test 2 — activeOpen saat state bukan CLOSED harus throw
void test_activeOpen_invalid_state() {
    std::cout << "\n===== Test: activeOpen pada State Tidak Valid =====\n";

    TCPSocket client("192.168.1.10", 54321,
        [](const TCPSegment&, const std::string&, uint16_t){}
    );

    client.activeOpen("192.168.1.20", 80); // state → SYN_SENT

    bool exceptionThrown = false;
    try {
        client.activeOpen("192.168.1.20", 80); // harus throw
    } catch (const std::runtime_error& e) {
        exceptionThrown = true;
        std::cout << "Exception tertangkap (benar): " << e.what() << "\n";
    }
    assert(exceptionThrown);

    std::cout << "[PASS] Exception dilempar dengan benar\n";
}
// Test 3 — SYN-ACK dengan ackNum salah harus diabaikan
void test_invalid_ackNum() {
    std::cout << "\n===== Test: SYN-ACK dengan ackNum Tidak Valid =====\n";

    TCPSocket client("192.168.1.10", 54321,
        [](const TCPSegment&, const std::string&, uint16_t){}
    );
    client.activeOpen("192.168.1.20", 80);

    // Buat SYN-ACK palsu dengan ackNum yang salah (9999 bukan 1001)
    TCPSegment fakeSynAck = makeSegment(
        80, 54321,
        2000,  // seqNum server
        9999,  // ackNum salah — seharusnya 1001
        TCP_FLAG_SYN | TCP_FLAG_ACK
    );

    client.handleIncomingSegment(fakeSynAck);

    // State harus tetap SYN_SENT karena segmen diabaikan
    assert(client.state == TCPState::SYN_SENT);

    std::cout << "[PASS] Segmen tidak valid diabaikan, state tetap SYN_SENT\n";
}
// main
int main() {
    test_handshake_normal();
    test_activeOpen_invalid_state();
    test_invalid_ackNum();

    std::cout << "\n===== Semua test passed =====\n";
    return 0;
}