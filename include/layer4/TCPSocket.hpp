// Bagian 1 — Guard, Includes
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include "TCPSegment.hpp"
enum class TCPState {
    CLOSED,
    LISTEN,
    SYN_SENT,
    SYN_RCVD,
    ESTABLISHED,
    FIN_WAIT_1,
    FIN_WAIT_2,
    TIME_WAIT,
    CLOSE_WAIT,
    LAST_ACK
};
using SendCallback = std::function<
    void(const TCPSegment&, const std::string&, uint16_t)
>;
class TCPSocket {
public:

    std::string  localIP;
    uint16_t     localPort;
    std::string  remoteIP;   // diisi saat koneksi terbentuk
    uint16_t     remotePort; // diisi saat koneksi terbentuk
    TCPState     state;
    uint32_t     seqNum;  // seq berikutnya yang akan dikirim
    uint32_t     ackNum;  // seq berikutnya yang diharapkan dari lawan

    SendCallback sendCallback;
    TCPSocket(
        const std::string& localIP,
        uint16_t           localPort,
        SendCallback       cb
    );

    void activeOpen (const std::string& dstIP, uint16_t dstPort);
    void passiveOpen();
    void handleIncomingSegment(const TCPSegment& seg);

    std::string stateToString() const;

protected:
    void transitionTo(TCPState newState);
    void sendSegment(
        uint8_t                         flags,
        uint32_t                        seq,
        uint32_t                        ack,
        const std::vector<uint8_t>&     payload = {}
    );
};