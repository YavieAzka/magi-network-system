#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include "../include/layer4/TCPSocket.hpp"
#include "../include/layer4/TCPSegment.hpp"

using namespace std;

static int testsPassed = 0;
static int testsFailed = 0;

static void check(const string &testName, bool condition)
{
    if (condition)
    {
        cout << "[PASS] " << testName << "\n" << flush;
        testsPassed++;
    }
    else
    {
        cout << "[FAIL] " << testName << "\n" << flush;
        testsFailed++;
    }
}

struct CapturedSegment
{
    TCPSegment seg;
    string dstIP;
    uint16_t dstPort;
};

static vector<CapturedSegment> clientSent;
static vector<CapturedSegment> serverSent;

static void clientSendCb(const TCPSegment &seg, const string &ip, uint16_t port)
{
    clientSent.push_back({seg, ip, port});
}

static void serverSendCb(const TCPSegment &seg, const string &ip, uint16_t port)
{
    serverSent.push_back({seg, ip, port});
}

static void deliverTo(TCPSocket &target, vector<CapturedSegment> &src)
{
    vector<CapturedSegment> copy = src;
    src.clear();
    for (auto &cs : copy)
        target.handleIncomingSegment(cs.seg);
}

static void doHandshake(TCPSocket &client, TCPSocket &server)
{
    client.activeOpen(server.localIP, server.localPort);
    deliverTo(server, clientSent);
    deliverTo(client, serverSent);
    deliverTo(server, clientSent);
}

void testInitialState()
{
    cout << "\n== TEST: Initial state ==\n" << flush;
    TCPSocket client("10.0.0.1", 12345, clientSendCb);
    check("Initial state is CLOSED", client.state == TCPState::CLOSED);
    check("Initial seqNum is 1000", client.seqNum == 1000);
    check("Initial ackNum is 0", client.ackNum == 0);
    clientSent.clear();
}

void testActiveOpen()
{
    cout << "\n== TEST: activeOpen sends SYN, transitions to SYN_SENT ==\n" << flush;
    clientSent.clear();
    TCPSocket client("10.0.0.1", 12345, clientSendCb);

    client.activeOpen("10.0.0.2", 80);
    check("State is SYN_SENT after activeOpen", client.state == TCPState::SYN_SENT);
    check("remoteIP set", client.remoteIP == "10.0.0.2");
    check("remotePort set", client.remotePort == 80);
    check("seqNum incremented (1001)", client.seqNum == 1001);
    check("One segment sent", clientSent.size() == 1);
    if (!clientSent.empty())
    {
        check("Segment has SYN flag", (clientSent[0].seg.flags & TCP_FLAG_SYN) != 0);
        check("SYN seqNum is 1000", clientSent[0].seg.seqNum == 1000);
    }
    clientSent.clear();
}

void testPassiveOpen()
{
    cout << "\n== TEST: passiveOpen transitions to LISTEN ==\n" << flush;
    TCPSocket server("10.0.0.2", 80, serverSendCb);
    server.passiveOpen();
    check("State is LISTEN after passiveOpen", server.state == TCPState::LISTEN);
}

void testThreeWayHandshake()
{
    cout << "\n== TEST: Full 3-way handshake ==\n" << flush;
    clientSent.clear();
    serverSent.clear();

    TCPSocket client("10.0.0.1", 12345, clientSendCb);
    TCPSocket server("10.0.0.2", 80, serverSendCb);

    server.passiveOpen();

    // Step 1: Client sends SYN
    client.activeOpen("10.0.0.2", 80);
    check("Client state SYN_SENT", client.state == TCPState::SYN_SENT);

    // Step 2: Server receives SYN, sends SYN+ACK
    deliverTo(server, clientSent);
    check("Server state SYN_RCVD after receiving SYN", server.state == TCPState::SYN_RCVD);

    bool serverSentSynAck = (serverSent.size() == 1) &&
                             (serverSent[0].seg.flags & TCP_FLAG_SYN) &&
                             (serverSent[0].seg.flags & TCP_FLAG_ACK);
    check("Server sent SYN+ACK with correct flags", serverSentSynAck);

    // Step 3: Client receives SYN+ACK, sends ACK
    deliverTo(client, serverSent);
    check("Client state ESTABLISHED after receiving SYN+ACK", client.state == TCPState::ESTABLISHED);

    bool clientSentAck = (clientSent.size() == 1) &&
                          (clientSent[0].seg.flags & TCP_FLAG_ACK) &&
                          !(clientSent[0].seg.flags & TCP_FLAG_SYN);
    check("Client sent ACK (no SYN)", clientSentAck);

    // Step 4: Server receives ACK
    deliverTo(server, clientSent);
    check("Server state ESTABLISHED after receiving ACK", server.state == TCPState::ESTABLISHED);

    check("Client seqNum correct after handshake", client.seqNum == 1001);
    check("Server seqNum correct after handshake", server.seqNum == 1001);
    check("Client ackNum correct after handshake", client.ackNum == 1001);
    check("Server ackNum correct after handshake", server.ackNum == 1001);
}

void testDataTransfer()
{
    cout << "\n== TEST: Data transfer after ESTABLISHED ==\n" << flush;
    clientSent.clear();
    serverSent.clear();

    TCPSocket client("10.0.0.1", 12345, clientSendCb);
    TCPSocket server("10.0.0.2", 80, serverSendCb);

    server.passiveOpen();
    doHandshake(client, server);

    check("Both ESTABLISHED before data", client.state == TCPState::ESTABLISHED && server.state == TCPState::ESTABLISHED);

    // Client sends data
    vector<uint8_t> hello = {'H', 'e', 'l', 'l', 'o'};
    client.sendData(hello);

    bool dataOk = (clientSent.size() == 1) &&
                  (clientSent[0].seg.flags & TCP_FLAG_PSH) &&
                  (clientSent[0].seg.flags & TCP_FLAG_ACK) &&
                  (clientSent[0].seg.payload.size() == 5);
    check("Client sent PSH+ACK data segment", dataOk);

    // Server receives data
    deliverTo(server, clientSent);
    check("Server received data in buffer", server.receiveBuffer.size() == 5);
    check("Server buffer contains 'Hello'", server.receiveBuffer[0] == 'H' && server.receiveBuffer[4] == 'o');
    check("Server sent ACK for data", !serverSent.empty() && (serverSent[0].seg.flags & TCP_FLAG_ACK));

    auto received = server.recvData();
    check("recvData returns correct data", received == hello);
    check("recvData clears buffer", server.receiveBuffer.empty());
    serverSent.clear();
}

void testSendDataNotEstablished()
{
    cout << "\n== TEST: sendData in CLOSED state does nothing ==\n" << flush;
    clientSent.clear();
    TCPSocket client("10.0.0.1", 12345, clientSendCb);

    client.sendData({'t', 'e', 's', 't'});
    check("No segment sent when not ESTABLISHED", clientSent.empty());
    check("State remains CLOSED", client.state == TCPState::CLOSED);
}

void testActiveClose()
{
    cout << "\n== TEST: Active close (full 4-way teardown) ==\n" << flush;
    clientSent.clear();
    serverSent.clear();

    TCPSocket client("10.0.0.1", 12345, clientSendCb);
    TCPSocket server("10.0.0.2", 80, serverSendCb);

    server.passiveOpen();
    doHandshake(client, server);
    check("Both ESTABLISHED", client.state == TCPState::ESTABLISHED && server.state == TCPState::ESTABLISHED);

    // Client initiates close
    client.close();
    check("Client state is FIN_WAIT_1 after close()", client.state == TCPState::FIN_WAIT_1);
    check("Client sent FIN segment", !clientSent.empty() && (clientSent[0].seg.flags & TCP_FLAG_FIN));
    check("Client FIN has ACK flag", !clientSent.empty() && (clientSent[0].seg.flags & TCP_FLAG_ACK));

    // Server receives FIN
    deliverTo(server, clientSent);
    check("Server state is CLOSE_WAIT after FIN", server.state == TCPState::CLOSE_WAIT);

    // Server also closes
    server.close();
    check("Server state is LAST_ACK after close()", server.state == TCPState::LAST_ACK);

    // Client receives server's FIN+ACK
    deliverTo(client, serverSent);
    check("Client state is TIME_WAIT", client.state == TCPState::TIME_WAIT);

    // Server receives final ACK
    deliverTo(server, clientSent);
    check("Server state is CLOSED", server.state == TCPState::CLOSED);
}

void testPassiveClose()
{
    cout << "\n== TEST: Passive close (server receives FIN first) ==\n" << flush;
    clientSent.clear();
    serverSent.clear();

    TCPSocket client("10.0.0.1", 12345, clientSendCb);
    TCPSocket server("10.0.0.2", 80, serverSendCb);

    server.passiveOpen();
    doHandshake(client, server);
    check("Both ESTABLISHED", client.state == TCPState::ESTABLISHED && server.state == TCPState::ESTABLISHED);

    // Client initiates close
    client.close();
    deliverTo(server, clientSent);
    check("Server in CLOSE_WAIT", server.state == TCPState::CLOSE_WAIT);

    server.close();
    check("Server in LAST_ACK after close()", server.state == TCPState::LAST_ACK);
    serverSent.clear();
}

void testSimultaneousClose()
{
    cout << "\n== TEST: Simultaneous close ==\n" << flush;
    clientSent.clear();
    serverSent.clear();

    TCPSocket client("10.0.0.1", 12345, clientSendCb);
    TCPSocket server("10.0.0.2", 80, serverSendCb);

    server.passiveOpen();
    doHandshake(client, server);

    // Both close simultaneously from ESTABLISHED
    // In simultaneous close, both send FIN and go to FIN_WAIT_1
    client.close();
    server.close();

    check("Client in FIN_WAIT_1", client.state == TCPState::FIN_WAIT_1);
    check("Server also in FIN_WAIT_1 (simultaneous close)", server.state == TCPState::FIN_WAIT_1);

    // Both receive each other's FIN
    deliverTo(server, clientSent);
    deliverTo(client, serverSent);

    // Simultaneous FIN: receiving FIN while in FIN_WAIT_1 -> TIME_WAIT
    check("Client in TIME_WAIT after simultaneous FIN", client.state == TCPState::TIME_WAIT);
    check("Server in TIME_WAIT after simultaneous FIN", server.state == TCPState::TIME_WAIT);
}

void testInvalidStateTransitions()
{
    cout << "\n== TEST: Invalid state transitions ==\n" << flush;
    clientSent.clear();
    serverSent.clear();

    TCPSocket client("10.0.0.1", 12345, clientSendCb);

    bool threw = false;
    try
    {
        client.activeOpen("10.0.0.2", 80);
        client.activeOpen("10.0.0.2", 80);
    }
    catch (const runtime_error &)
    {
        threw = true;
    }
    check("activeOpen() in non-CLOSED throws", threw);

    TCPSocket server("10.0.0.2", 80, serverSendCb);
    server.passiveOpen();
    threw = false;
    try
    {
        server.passiveOpen();
    }
    catch (const runtime_error &)
    {
        threw = true;
    }
    check("passiveOpen() in non-CLOSED throws", threw);
}

void testHandshakeWithBadAck()
{
    cout << "\n== TEST: Handshake rejects bad ACK/ackNum ==\n" << flush;
    clientSent.clear();
    serverSent.clear();

    TCPSocket client("10.0.0.1", 12345, clientSendCb);
    TCPSocket server("10.0.0.2", 80, serverSendCb);

    server.passiveOpen();
    client.activeOpen("10.0.0.2", 80);
    deliverTo(server, clientSent);

    check("Server in SYN_RCVD after receiving SYN", server.state == TCPState::SYN_RCVD);

    // Send bad ACK to server (wrong ackNum)
    TCPSegment badAck;
    badAck.srcPort = 12345;
    badAck.dstPort = 80;
    badAck.seqNum = 9999;
    badAck.ackNum = 0;
    badAck.flags = TCP_FLAG_ACK;
    badAck.dataOffset = 5;
    badAck.windowSize = 65535;

    server.handleIncomingSegment(badAck);
    check("Server stays SYN_RCVD with bad ACK", server.state == TCPState::SYN_RCVD);

    // Send bad SYN+ACK to client (wrong ackNum)
    TCPSegment badSynAck;
    badSynAck.srcPort = 80;
    badSynAck.dstPort = 12345;
    badSynAck.seqNum = 1000;
    badSynAck.ackNum = 999;
    badSynAck.flags = TCP_FLAG_SYN | TCP_FLAG_ACK;
    badSynAck.dataOffset = 5;
    badSynAck.windowSize = 65535;

    client.handleIncomingSegment(badSynAck);
    check("Client stays SYN_SENT with bad ackNum", client.state == TCPState::SYN_SENT);
}

void testStateLogging()
{
    cout << "\n== TEST: State transition logging ==\n" << flush;
    clientSent.clear();
    serverSent.clear();

    TCPSocket client("10.0.0.1", 12345, clientSendCb);
    TCPSocket server("10.0.0.2", 80, serverSendCb);

    cout << "(Expect: CLOSED --> SYN_SENT)\n";
    client.activeOpen("10.0.0.2", 80);
    check("State logged for activeOpen", true);

    cout << "(Expect: CLOSED --> LISTEN)\n";
    server.passiveOpen();
    check("State logged for passiveOpen", true);

    cout << "(Expect: LISTEN --> SYN_RCVD, SYN_RCVD --> ESTABLISHED, SYN_SENT --> ESTABLISHED)\n";
    deliverTo(server, clientSent);
    deliverTo(client, serverSent);
    deliverTo(server, clientSent);
    check("Full handshake logged", client.state == TCPState::ESTABLISHED && server.state == TCPState::ESTABLISHED);
}

int main()
{
    testInitialState();
    testActiveOpen();
    testPassiveOpen();
    testThreeWayHandshake();
    testDataTransfer();
    testSendDataNotEstablished();
    testActiveClose();
    testPassiveClose();
    testSimultaneousClose();
    testInvalidStateTransitions();
    testHandshakeWithBadAck();
    testStateLogging();

    cout << "\n===========================\n";
    cout << "TCP State Machine Tests: PASSED " << testsPassed
         << " / FAILED " << testsFailed << "\n";
    return testsFailed > 0 ? 1 : 0;
}