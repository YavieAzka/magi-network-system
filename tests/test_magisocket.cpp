#include "core/Simulator.hpp"
#include "core/Host.hpp"
#include "layer7/MagiSocket.hpp"

#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <vector>
#include <string>

using namespace std;

int main() {
    cout << "\n========================================\n";
    cout << "  Layer 7 MagiSocket Integration Test\n";
    cout << "  Topologi: Server <---> Client\n";
    cout << "========================================\n";

    Simulator sim;
    Host* server = dynamic_cast<Host*>(sim.createNode("host", "Server"));
    Host* client = dynamic_cast<Host*>(sim.createNode("host", "Client"));

    sim.linkNodes("Server:1", "Client:1", 10);
    
    server->setIpAddress("192.168.1.10/24");
    client->setIpAddress("192.168.1.11/24");
    cout << "\n[Setup Server] Inisialisasi MagiSocket Server...\n";
    MagiSocket serverSock(server, MAGI_SOCK_STREAM);
    serverSock.bind("192.168.1.10", 80);
    serverSock.listen();

    cout << "\n[Setup Client] Menjalankan Client di thread terpisah...\n";
    thread clientThread([&]() {
        this_thread::sleep_for(chrono::milliseconds(500));

        MagiSocket clientSock(client, MAGI_SOCK_STREAM);
        clientSock.bind("192.168.1.11", 12345);
        
        cout << "[Client] Memulai koneksi (connect) ke Server...\n";
        clientSock.connect("192.168.1.10", 80);

        this_thread::sleep_for(chrono::milliseconds(1000)); 
        
        string msgStr = "Halo Server, ini Client!";
        vector<uint8_t> msgPayload(msgStr.begin(), msgStr.end());
        
        cout << "[Client] Mengirim pesan: \"" << msgStr << "\"\n";
        clientSock.send(msgPayload);

        this_thread::sleep_for(chrono::milliseconds(1500));
        
        cout << "\n[Teardown Client] Menutup koneksi Client...\n";
        clientSock.close();
    });

    cout << "\n[Server] Menunggu koneksi masuk (blocking accept)...\n";
    MagiSocket* acceptedSock = nullptr;
    
    while (!acceptedSock) {
        try {
            acceptedSock = serverSock.accept();
        } catch (const std::runtime_error&) {
            this_thread::sleep_for(chrono::milliseconds(100)); 
        }
    }
    cout << "[Server] Koneksi diterima! Socket baru berhasil dialokasikan.\n";

    cout << "\n[Server] Menunggu data dari Client...\n";
    this_thread::sleep_for(chrono::milliseconds(1500));
    
    vector<uint8_t> receivedBytes = acceptedSock->recv();
    string receivedStr(receivedBytes.begin(), receivedBytes.end());
    
    cout << "[Server] Berhasil melakukan recv(). Menerima pesan:\n";
    cout << ">> \"" << receivedStr << "\"\n";
    assert(receivedStr == "Halo Server, ini Client!" && "GAGAL: Data yang diterima cacat!");

    this_thread::sleep_for(chrono::milliseconds(500)); 
    
    cout << "\n[Teardown Server] Menutup koneksi dari sisi Server...\n";
    acceptedSock->close();
    serverSock.close(); 
    clientThread.join();
    delete acceptedSock;
    
    cout << "\n========================================\n";
    cout << "  Pengujian MagiSocket TCP SUKSES!\n";
    cout << "========================================\n";

    return 0;
}
