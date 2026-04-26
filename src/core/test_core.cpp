// #include "../include/core/Packet.hpp"
// #include "../include/core/Node.hpp"
// #include "../include/core/Interface.hpp"
// #include "../include/core/Link.hpp"

// #include <cassert>
// #include <cstdint>
// #include <iostream>
// #include <stdexcept>
// #include <string>
// #include <vector>

// using namespace std;

// /*
//  * DummyPacket hanya untuk testing.
//  * Karena Packet adalah abstract class, kita butuh class turunan konkret
//  * untuk membuktikan bahwa polymorphism Packet::toBytes() berjalan.
//  */
// class DummyPacket : public Packet
// {
// private:
//     vector<uint8_t> payload;

// public:
//     explicit DummyPacket(const string &text)
//         : payload(text.begin(), text.end())
//     {
//     }

//     explicit DummyPacket(const vector<uint8_t> &payload)
//         : payload(payload)
//     {
//     }

//     vector<uint8_t> toBytes() const override
//     {
//         return payload;
//     }

//     string getPayloadAsString() const
//     {
//         return string(payload.begin(), payload.end());
//     }
// };

// static void printDivider(const string &title)
// {
//     cout << "\n========== " << title << " ==========" << endl;
// }

// static void testPacketPolymorphism()
// {
//     printDivider("TEST 1: Packet polymorphism");

//     DummyPacket dummy("HELLO");

//     Packet *packet = &dummy;
//     vector<uint8_t> raw = packet->toBytes();

//     cout << "Payload as text      : " << dummy.getPayloadAsString() << endl;
//     cout << "Serialized byte size : " << raw.size() << endl;
//     cout << "Serialized bytes     : ";

//     for (uint8_t byte : raw)
//     {
//         cout << static_cast<char>(byte);
//     }

//     cout << endl;

//     assert(raw.size() == 5);
//     assert(raw[0] == 'H');
//     assert(raw[1] == 'E');
//     assert(raw[2] == 'L');
//     assert(raw[3] == 'L');
//     assert(raw[4] == 'O');

//     cout << "[PASS] Packet abstract base + toBytes() override works" << endl;
// }

// static void testNodeCreationAndInterface()
// {
//     printDivider("TEST 2: Node and Interface creation");

//     Node h1("H1");

//     assert(h1.getName() == "H1");
//     assert(h1.getType() == "node");
//     assert(h1.isActive());

//     Interface *h1p1 = h1.addInterface(1, "AA:BB:CC:00:00:01");

//     assert(h1p1 != nullptr);
//     assert(h1.getInterface(1) == h1p1);
//     assert(h1p1->getOwner() == &h1);
//     assert(h1p1->getPortNumber() == 1);
//     assert(h1p1->getMacAddress() == "AA:BB:CC:00:00:01");
//     assert(h1p1->isUp());
//     assert(!h1p1->isConnected());
//     assert(h1p1->getMtu() == 1500);
//     assert(h1p1->getSentBytes() == 0);
//     assert(h1p1->getReceivedBytes() == 0);

//     cout << "Node name       : " << h1.getName() << endl;
//     cout << "Node type       : " << h1.getType() << endl;
//     cout << "Node active     : " << (h1.isActive() ? "true" : "false") << endl;
//     cout << "Interface port  : " << h1p1->getPortNumber() << endl;
//     cout << "Interface MAC   : " << h1p1->getMacAddress() << endl;
//     cout << "Interface MTU   : " << h1p1->getMtu() << endl;

//     cout << "[PASS] Node and Interface creation works" << endl;
// }

// static void testLinkTransmission()
// {
//     printDivider("TEST 3: Link transmission");

//     Node h1("H1");
//     Node h2("H2");

//     Interface *h1p1 = h1.addInterface(1, "AA:BB:CC:00:00:01");
//     Interface *h2p1 = h2.addInterface(1, "AA:BB:CC:00:00:02");

//     Link link(h1p1, h2p1, 0);

//     assert(h1p1->isConnected());
//     assert(h2p1->isConnected());
//     assert(link.isActive());
//     assert(link.getDelayMs() == 0);
//     assert(link.getInterfaceA() == h1p1);
//     assert(link.getInterfaceB() == h2p1);
//     assert(link.getOtherInterface(h1p1) == h2p1);
//     assert(link.getOtherInterface(h2p1) == h1p1);

//     DummyPacket packet("HELLO");
//     vector<uint8_t> raw = packet.toBytes();

//     cout << "Sending payload from H1 to H2..." << endl;
//     h1p1->send(raw);

//     assert(h1p1->getSentBytes() == raw.size());
//     assert(h2p1->getReceivedBytes() == raw.size());
//     assert(link.getTransmittedBytes() == raw.size());

//     cout << "H1 sent bytes        : " << h1p1->getSentBytes() << endl;
//     cout << "H2 received bytes    : " << h2p1->getReceivedBytes() << endl;
//     cout << "Link transmitted     : " << link.getTransmittedBytes() << endl;

//     cout << "[PASS] Interface -> Link -> Interface -> Node transmission works" << endl;
// }

// static void testInterfaceDown()
// {
//     printDivider("TEST 4: Interface down blocks sending");

//     Node h1("H1");
//     Node h2("H2");

//     Interface *h1p1 = h1.addInterface(1, "AA:BB:CC:00:00:01");
//     Interface *h2p1 = h2.addInterface(1, "AA:BB:CC:00:00:02");

//     Link link(h1p1, h2p1, 0);

//     h1p1->setUp(false);

//     DummyPacket packet("HELLO");
//     vector<uint8_t> raw = packet.toBytes();

//     cout << "Trying to send while H1 interface is down..." << endl;
//     h1p1->send(raw);

//     assert(h1p1->getSentBytes() == 0);
//     assert(h2p1->getReceivedBytes() == 0);
//     assert(link.getTransmittedBytes() == 0);

//     cout << "H1 sent bytes        : " << h1p1->getSentBytes() << endl;
//     cout << "H2 received bytes    : " << h2p1->getReceivedBytes() << endl;
//     cout << "Link transmitted     : " << link.getTransmittedBytes() << endl;

//     cout << "[PASS] Interface down blocks sending" << endl;
// }

// static void testMtuBlocking()
// {
//     printDivider("TEST 5: MTU blocks oversized packet");

//     Node h1("H1");
//     Node h2("H2");

//     Interface *h1p1 = h1.addInterface(1, "AA:BB:CC:00:00:01");
//     Interface *h2p1 = h2.addInterface(1, "AA:BB:CC:00:00:02");

//     Link link(h1p1, h2p1, 0);

//     h1p1->setMtu(3);

//     DummyPacket packet("HELLO");
//     vector<uint8_t> raw = packet.toBytes();

//     cout << "MTU                : " << h1p1->getMtu() << endl;
//     cout << "Packet byte size   : " << raw.size() << endl;
//     cout << "Trying to send packet larger than MTU..." << endl;

//     h1p1->send(raw);

//     assert(h1p1->getSentBytes() == 0);
//     assert(h2p1->getReceivedBytes() == 0);
//     assert(link.getTransmittedBytes() == 0);

//     cout << "H1 sent bytes        : " << h1p1->getSentBytes() << endl;
//     cout << "H2 received bytes    : " << h2p1->getReceivedBytes() << endl;
//     cout << "Link transmitted     : " << link.getTransmittedBytes() << endl;

//     cout << "[PASS] MTU check works" << endl;
// }

// static void testInactiveLink()
// {
//     printDivider("TEST 6: Inactive link blocks transmission");

//     Node h1("H1");
//     Node h2("H2");

//     Interface *h1p1 = h1.addInterface(1, "AA:BB:CC:00:00:01");
//     Interface *h2p1 = h2.addInterface(1, "AA:BB:CC:00:00:02");

//     Link link(h1p1, h2p1, 0);
//     link.setActive(false);

//     DummyPacket packet("HELLO");
//     vector<uint8_t> raw = packet.toBytes();

//     cout << "Trying to send through inactive link..." << endl;
//     h1p1->send(raw);

//     /*
//      * Interface sudah berhasil memanggil send(), jadi sentBytes naik.
//      * Tetapi Link inactive, sehingga transmittedBytes dan receivedBytes tidak naik.
//      */
//     assert(h1p1->getSentBytes() == raw.size());
//     assert(link.getTransmittedBytes() == 0);
//     assert(h2p1->getReceivedBytes() == 0);

//     cout << "H1 sent bytes        : " << h1p1->getSentBytes() << endl;
//     cout << "H2 received bytes    : " << h2p1->getReceivedBytes() << endl;
//     cout << "Link transmitted     : " << link.getTransmittedBytes() << endl;

//     cout << "[PASS] Inactive link blocks transmission" << endl;
// }

// static void testRemoveConnectedInterface()
// {
//     printDivider("TEST 7: Removing connected interface is rejected");

//     Node h1("H1");
//     Node h2("H2");

//     Interface *h1p1 = h1.addInterface(1, "AA:BB:CC:00:00:01");
//     Interface *h2p1 = h2.addInterface(1, "AA:BB:CC:00:00:02");

//     Link link(h1p1, h2p1, 0);

//     bool thrown = false;

//     try
//     {
//         h1.removeInterface(1);
//     }
//     catch (const runtime_error &e)
//     {
//         thrown = true;
//         cout << "Caught expected error: " << e.what() << endl;
//     }

//     assert(thrown);

//     cout << "[PASS] Removing connected interface is rejected" << endl;
// }

// int main()
// {
//     try
//     {
//         cout << "=== MILESTONE 0 CORE TEST START ===" << endl;

//         testPacketPolymorphism();
//         testNodeCreationAndInterface();
//         testLinkTransmission();
//         testInterfaceDown();
//         testMtuBlocking();
//         testInactiveLink();
//         testRemoveConnectedInterface();

//         cout << "\n=== ALL TESTS PASSED ===" << endl;
//         return 0;
//     }
//     catch (const exception &e)
//     {
//         cerr << "\n[FAILED] Unexpected exception: " << e.what() << endl;
//         return 1;
//     }
// }