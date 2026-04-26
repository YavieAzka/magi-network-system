#include "../include/core/Node.hpp"
#include "../include/core/Interface.hpp"
#include "../include/core/Link.hpp"
#include "../include/core/Packet.hpp"

int main() {
    // 1. Buat dua node
    Node nodeA("H1");
    Node nodeB("H2");

    // 2. Tambah interface
    Interface* ifA = nodeA.addInterface(1, "CA:FE:00:00:00:01");
    Interface* ifB = nodeB.addInterface(1, "CA:FE:00:00:00:02");

    // 3. Hubungkan dengan link, delay 10ms
    Link link(ifA, ifB, 10);

    // 4. Buat packet dan kirim
    RawPacket pkt("Hello from H1!");
    ifA->send(pkt.toBytes());

    // Expected output:
    // [H1:1] sending 14 bytes.
    // [Link] transmitting 14 bytes from H1:1 to H2:1 | delay=10ms
    // [H2] received 14 bytes on port 1
    // [H2] raw bytes: 48 65 6C 6C 6F ...

    return 0;
}