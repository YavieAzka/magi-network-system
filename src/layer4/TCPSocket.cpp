// Bagian 1 — Includes
#include "include/layer4/TCPSocket.hpp"
#include <iostream>
#include <stdexcept>

std::string TCPSocket::stateToString() const {
    switch (state) {
        case TCPState::CLOSED:      return "CLOSED";
        case TCPState::LISTEN:      return "LISTEN";
        case TCPState::SYN_SENT:    return "SYN_SENT";
        case TCPState::SYN_RCVD:    "SYN_RCVD";
        case TCPState::ESTABLISHED:  return "ESTABLISHED";
        case TCPState::FIN_WAIT_1:  return "FIN_WAIT_1";
        case TCPState::FIN_WAIT_2:  return "FIN_WAIT_2";
        case TCPState::TIME_WAIT:    return "TIME_WAIT";
        case TCPState::CLOSE_WAIT:   return "CLOSE_WAIT";
        case TCPState::LAST_ACK:     return "LAST_ACK";
        default:                    return "UNKNOWN";
    }
}

TCPSocket::TCPSocket(
    const std::string& localIP,
    uint16_t           localPort,
    SendCallback       cb
)
    : localIP(localIP),
      localPort(localPort),
      remoteIP(""),
      remotePort(0),
      state(TCPState::CLOSED),
      seqNum(1000),  // ISN sederhana; di implementasi real ini random
      ackNum(0),
      sendCallback(cb)
{}

void TCPSocket::transitionTo(TCPState newState) {
    std::cout << "[TCPSocket " << localIP << ":" << localPort << "] "
              << "State: " << stateToString()
              << " --> ";
    state = newState;
    std::cout << stateToString() << "\n";
}

void TCPSocket::sendSegment(
    uint8_t                         flags,
    uint32_t                        seq,
    uint32_t                        ack,
    const std::vector<uint8_t>&     payload
) {
    TCPSegment seg;
    seg.srcPort    = localPort;
    seg.dstPort    = remotePort;
    seg.seqNum     = seq;
    seg.ackNum     = ack;
    seg.flags      = flags;
    seg.windowSize = 65535;
    seg.dataOffset = 5;
    seg.payload    = payload;

    seg.calculateChecksum(localIP, remoteIP);

    std::cout << "[TCPSocket " << localIP << ":" << localPort << "] "
              << "Mengirim: ";
    seg.print();

    sendCallback(seg, remoteIP, remotePort);
}
void TCPSocket::activeOpen(
    const std::string& dstIP,
    uint16_t           dstPort
) {
    if (state != TCPState::CLOSED) {
        throw std::runtime_error(
            "activeOpen() dipanggil saat state bukan CLOSED, state saat ini: "
            + stateToString()
        );
    }

    remoteIP   = dstIP;
    remotePort = dstPort;

    sendSegment(TCP_FLAG_SYN, seqNum, 0);
    seqNum += 1;

    transitionTo(TCPState::SYN_SENT);
}

void TCPSocket::passiveOpen() {
    if (state != TCPState::CLOSED) {
        throw std::runtime_error(
            "passiveOpen() dipanggil saat state bukan CLOSED, state saat ini: "
            + stateToString()
        );
    }
    transitionTo(TCPState::LISTEN);
}
void TCPSocket::handleIncomingSegment(const TCPSegment& seg) {
    std::cout << "[TCPSocket " << localIP << ":" << localPort << "] "
              << "Menerima: ";
    seg.print();

    switch (state) {

        case TCPState::LISTEN: {
            if (!(seg.flags & TCP_FLAG_SYN)) break;

            remotePort = seg.srcPort;
            ackNum = seg.seqNum + 1;

            sendSegment(TCP_FLAG_SYN | TCP_FLAG_ACK, seqNum, ackNum);
            seqNum += 1;

            transitionTo(TCPState::SYN_RCVD);
            break;
        }

        case TCPState::SYN_RCVD: {
            if (!(seg.flags & TCP_FLAG_ACK)) break;
            if (seg.ackNum != seqNum) {
                std::cout << "[TCPSocket] Peringatan: ackNum tidak sesuai, "
                          << "ekspektasi: " << seqNum
                          << " diterima: "  << seg.ackNum << "\n";
                break;
            }

            transitionTo(TCPState::ESTABLISHED);
            break;
        }
        case TCPState::SYN_SENT: {
            bool isSynAck = (seg.flags & TCP_FLAG_SYN)
                          && (seg.flags & TCP_FLAG_ACK);
            if (!isSynAck) break;
            if (seg.ackNum != seqNum) {
                std::cout << "[TCPSocket] Peringatan: ackNum tidak sesuai, "
                          << "ekspektasi: " << seqNum
                          << " diterima: "  << seg.ackNum << "\n";
                break;
            }
            ackNum = seg.seqNum + 1;
            sendSegment(TCP_FLAG_ACK, seqNum, ackNum);

            transitionTo(TCPState::ESTABLISHED);
            break;
        }

        default:
            break;
    }
}