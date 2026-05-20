#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <deque>
#include <functional>
#include <iostream>
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
    std::string  remoteIP;
    uint16_t     remotePort;
    TCPState     state;
    uint32_t     seqNum;
    uint32_t     ackNum;
    std::deque<uint8_t> receiveBuffer;

    SendCallback sendCallback;

    TCPSocket(
        const std::string& localIP,
        uint16_t           localPort,
        SendCallback       cb = [](const TCPSegment&, const std::string&, uint16_t){}
    );

    void activeOpen (const std::string& dstIP, uint16_t dstPort);
    void passiveOpen();
    void close();

    void sendData(const std::vector<uint8_t>& data);
    std::vector<uint8_t> recvData();

    void handleIncomingSegment(const TCPSegment& seg);

    std::string stateToString() const;
    void setState(TCPState newState);

protected:
    void transitionTo(TCPState newState);
    void sendSegment(
        uint8_t                     flags,
        uint32_t                    seq,
        uint32_t                    ack,
        const std::vector<uint8_t>& payload = {}
    );
};