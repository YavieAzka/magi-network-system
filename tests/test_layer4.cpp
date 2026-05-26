// TRICK: Mengubah private/protected menjadi public KHUSUS untuk keperluan Unit Testing
#define private public
#define protected public
#include "core/Simulator.hpp"
#include "core/Host.hpp"
#include "core/Router.hpp"
#include "layer4/TCPSocket.hpp"
#undef private
#undef protected

#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

using namespace std;

// PENGUJIAN 1: TCP FULL PIPELINE
// Fase 1: Handshake | Fase 2: Data | Fase 3: Teardown
void test_tcp_full_pipeline(Host* h1, Host* h2) {
    cout << " TEST: TCP Full Pipeline (Lintas Router)\n";


    // ---- FASE 1: Handshake ----
    cout << "\n[Fase 1] 3-Way Handshake: H1 -> R1 -> H2...\n";
    h2->tcpListen(80);
    thread clientThread([&]() {
        h1->tcpConnect("10.0.0.20", 80);
    });

    this_thread::sleep_for(chrono::milliseconds(2000));
    cout << "[OK] Handshake selesai.\n";

    // ---- FASE 2: Transfer Data (PSH) ----
    cout << "\n[Fase 2] Transfer Data & Buffer Assembly...\n";
    TCPSocket* h1Socket = nullptr;
    for (auto const& [port, sock] : h1->tcpSockets_) {
        if (sock->remotePort == 80) {
            h1Socket = sock;
            break;
        }
    }
    assert(h1Socket != nullptr && "FATAL: Socket pengirim tidak ditemukan di H1!");
    TCPSocket* h2Socket = nullptr;
    for (auto const& [port, sock] : h2->tcpSockets_) {
        if (sock->state == TCPState::ESTABLISHED) {
            h2Socket = sock;
            break;
        }
    }
    if (!h2Socket && h2->tcpSockets_.count(80)) {
        h2Socket = h2->tcpSockets_[80];
    }
    assert(h2Socket != nullptr && "FATAL: Socket penerima tidak ditemukan di H2!");
    string msgStr = "Halo P5 Jarkom! Data TCP via Router!";
    vector<uint8_t> msgPayload(msgStr.begin(), msgStr.end());

    h1Socket->sendData(msgPayload);

    this_thread::sleep_for(chrono::milliseconds(1000));
    vector<uint8_t> receivedData = h2Socket->recvData();
    assert(receivedData == msgPayload && "GAGAL: Data yang diterima H2 cacat atau tidak sesuai!");
    cout << "[OK] Data sukses dibaca dari recvData() dan divalidasi dengan assert!\n";

    // ---- FASE 3: Teardown (4-Way FIN) ----
    cout << "\n[Fase 3] 4-Way Teardown: Menutup koneksi TCP...\n";

    h1Socket->close();

    this_thread::sleep_for(chrono::milliseconds(1500));

    int waitCount = 0;
    while ((h1Socket->state != TCPState::CLOSED || h2Socket->state != TCPState::CLOSED) && waitCount < 20)
    {
        this_thread::sleep_for(chrono::milliseconds(100));
        waitCount++;
    }

    assert(h1Socket->state == TCPState::CLOSED && "GAGAL: State H1 belum CLOSED!");
    assert(h2Socket->state == TCPState::CLOSED && "GAGAL: State H2 belum CLOSED!");

    cout << "[OK] 4-Way Teardown sukses. Kedua status soket kembali CLOSED.\n";

    clientThread.detach();
    cout << "\n--- TEST TCP FULL PIPELINE SELESAI ---\n";
}

// PENGUJIAN 2: UDP TRANSMISSION
void test_udp_transmission(Host* h1, Host* h2) {
    cout << " TEST: UDP Transmission (Lintas Router)\n";

    string udpMsg = "Datagram UDP Cepat via Router!";
    vector<uint8_t> udpPayload(udpMsg.begin(), udpMsg.end());

    cout << "[UDP] H1 menembak datagram ke H2 (10.0.0.20:5000)...\n";
    
    h1->sendUDP("10.0.0.20", 8080, 5000, udpPayload);

    // Beri waktu paket melewati router
    this_thread::sleep_for(chrono::milliseconds(500));

    cout << "[OK] Datagram UDP berhasil dikirim!\n";
    cout << "\n--- TEST UDP SELESAI ---\n";
}

int main() {
    cout << "\n========================================\n";
    cout << "  Layer 4 Integration Test (P5)\n";
    cout << "  Topologi: H1 <-> R1 <-> H2\n";
    cout << "========================================\n";

    Simulator sim;

    Host* h1 = dynamic_cast<Host*>  (sim.createNode("host",   "H1"));
    Router* r1 = dynamic_cast<Router*>(sim.createNode("router", "R1"));
    Host* h2 = dynamic_cast<Host*>  (sim.createNode("host",   "H2"));

    sim.linkNodes("H1:1", "R1:1", 10);
    sim.linkNodes("R1:2", "H2:1", 10);

    h1->setIpAddress("192.168.1.10/24");
    h1->setDefaultGateway("192.168.1.1");

    h2->setIpAddress("10.0.0.20/8");
    h2->setDefaultGateway("10.0.0.1");

    r1->setInterfaceIp(1, "192.168.1.1/24");
    r1->setInterfaceIp(2, "10.0.0.1/8");

    r1->addRoute("192.168.1.0/24", "192.168.1.10", 1);
    r1->addRoute("10.0.0.0/8",     "10.0.0.20",    2);

    test_tcp_full_pipeline(h1, h2);
    test_udp_transmission(h1, h2);

    cout << " Semua Test Layer 4 Selesai & Lulus!\n";

    return 0;
}