#include "layer4/TCPSocket.hpp"
#include <stdexcept>
#include <algorithm>

std::string TCPSocket::stateToString() const
{
    switch (state)
    {
    case TCPState::CLOSED:
        return "CLOSED";
    case TCPState::LISTEN:
        return "LISTEN";
    case TCPState::SYN_SENT:
        return "SYN_SENT";
    case TCPState::SYN_RCVD:
        return "SYN_RCVD";
    case TCPState::ESTABLISHED:
        return "ESTABLISHED";
    case TCPState::FIN_WAIT_1:
        return "FIN_WAIT_1";
    case TCPState::FIN_WAIT_2:
        return "FIN_WAIT_2";
    case TCPState::TIME_WAIT:
        return "TIME_WAIT";
    case TCPState::CLOSE_WAIT:
        return "CLOSE_WAIT";
    case TCPState::LAST_ACK:
        return "LAST_ACK";
    default:
        return "UNKNOWN";
    }
}

TCPSocket::TCPSocket(
    const std::string &localIP,
    uint16_t localPort,
    SendCallback cb)
    : localIP(localIP),
      localPort(localPort),
      remoteIP(""),
      remotePort(0),
      state(TCPState::CLOSED),
      seqNum(1000),
      ackNum(0),
      sendCallback(cb)
{
}

void TCPSocket::transitionTo(TCPState newState)
{
    std::cout << "[TCPSocket " << localIP << ":" << localPort << "] "
              << "State: " << stateToString()
              << " --> ";
    state = newState;
    std::cout << stateToString() << "\n";
}

void TCPSocket::sendSegment(
    uint8_t flags,
    uint32_t seq,
    uint32_t ack,
    const std::vector<uint8_t> &payload)
{
    TCPSegment seg;
    seg.srcPort = localPort;
    seg.dstPort = remotePort;
    seg.seqNum = seq;
    seg.ackNum = ack;
    seg.flags = flags;
    seg.windowSize = 65535;
    seg.dataOffset = 5;
    seg.payload = payload;

    seg.calculateChecksum(localIP, remoteIP);

    std::cout << "[TCPSocket " << localIP << ":" << localPort << "] "
              << "Sending: ";
    seg.print();

    sendCallback(seg, remoteIP, remotePort);
}

void TCPSocket::activeOpen(const std::string &dstIP, uint16_t dstPort)
{
    if (state != TCPState::CLOSED)
    {
        throw std::runtime_error(
            "activeOpen() called in non-CLOSED state, current state: " + stateToString());
    }

    remoteIP = dstIP;
    remotePort = dstPort;
    transitionTo(TCPState::SYN_SENT);
    seqNum += 1;
    sendSegment(TCP_FLAG_SYN, seqNum - 1, 0);
}

void TCPSocket::passiveOpen()
{
    if (state != TCPState::CLOSED)
    {
        throw std::runtime_error(
            "passiveOpen() called in non-CLOSED state, current state: " + stateToString());
    }
    transitionTo(TCPState::LISTEN);
}

void TCPSocket::close()
{
    switch (state)
    {
    case TCPState::ESTABLISHED:
        seqNum += 1;
        transitionTo(TCPState::FIN_WAIT_1);
        sendSegment(TCP_FLAG_FIN | TCP_FLAG_ACK, seqNum - 1, ackNum);
        break;
    case TCPState::CLOSE_WAIT:
        seqNum += 1;
        transitionTo(TCPState::LAST_ACK);
        sendSegment(TCP_FLAG_FIN | TCP_FLAG_ACK, seqNum - 1, ackNum);
        break;
    default:
        std::cout << "[TCPSocket " << localIP << ":" << localPort << "] "
                  << "close() called in state " << stateToString()
                  << ", invalid transition.\n";
        break;
    }
}

void TCPSocket::sendData(const std::vector<uint8_t> &data)
{
    if (state != TCPState::ESTABLISHED)
    {
        std::cout << "[TCPSocket " << localIP << ":" << localPort << "] "
                  << "sendData() called in state " << stateToString()
                  << ", must be ESTABLISHED.\n";
        return;
    }

    uint32_t seqToSend = seqNum;
    seqNum += static_cast<uint32_t>(data.size());
    sendSegment(TCP_FLAG_PSH | TCP_FLAG_ACK, seqToSend, ackNum, data);
}

std::vector<uint8_t> TCPSocket::recvData()
{
    std::vector<uint8_t> result(receiveBuffer.begin(), receiveBuffer.end());
    receiveBuffer.clear();
    return result;
}

void TCPSocket::handleIncomingSegment(const TCPSegment &seg)
{
    std::cout << "[TCPSocket " << localIP << ":" << localPort << "] "
              << "Receiving: ";
    seg.print();

    switch (state)
    {

    case TCPState::LISTEN:
    {
        if (!(seg.flags & TCP_FLAG_SYN))
            break;

        remotePort = seg.srcPort;
        ackNum = seg.seqNum + 1;

        transitionTo(TCPState::SYN_RCVD);
        seqNum += 1;
        sendSegment(TCP_FLAG_SYN | TCP_FLAG_ACK, seqNum - 1, ackNum);

        break;
    }

    case TCPState::SYN_SENT:
    {
        bool isSynAck = (seg.flags & TCP_FLAG_SYN) && (seg.flags & TCP_FLAG_ACK);
        if (!isSynAck)
            break;
        if (seg.ackNum != seqNum)
        {
            std::cout << "[TCPSocket " << localIP << ":" << localPort << "] "
                      << "Warning: ackNum mismatch, "
                      << "expected: " << seqNum
                      << " got: " << seg.ackNum << "\n";
            break;
        }
        ackNum = seg.seqNum + 1;
        transitionTo(TCPState::ESTABLISHED);
        sendSegment(TCP_FLAG_ACK, seqNum, ackNum);
        break;
    }

    case TCPState::SYN_RCVD:
    {
        if (!(seg.flags & TCP_FLAG_ACK))
            break;
        if (seg.seqNum != ackNum)
        {
            break;
        }
        if (seg.ackNum != seqNum)
        {
            std::cout << "[TCPSocket " << localIP << ":" << localPort << "] "
                      << "Warning: ackNum mismatch, "
                      << "expected: " << seqNum
                      << " got: " << seg.ackNum << "\n";
            break;
        }

        transitionTo(TCPState::ESTABLISHED);
        break;
    }

    case TCPState::ESTABLISHED:
    {
        if (seg.flags & TCP_FLAG_FIN)
        {
            ackNum = seg.seqNum + 1;
            transitionTo(TCPState::CLOSE_WAIT);
            sendSegment(TCP_FLAG_ACK, seqNum, ackNum);
            break;
        }

        if (seg.flags & TCP_FLAG_ACK)
        {
            if (seg.ackNum > seqNum)
            {
                std::cout << "[TCPSocket " << localIP << ":" << localPort << "] "
                          << "Warning: ACK for unsent seq.\n";
            }
        }

        if (!seg.payload.empty())
        {
            receiveBuffer.insert(receiveBuffer.end(),
                                 seg.payload.begin(), seg.payload.end());
            ackNum = seg.seqNum + static_cast<uint32_t>(seg.payload.size());

            std::cout << "[TCPSocket " << localIP << ":" << localPort << "] "
                      << "Received " << seg.payload.size()
                      << " bytes, buffer size: " << receiveBuffer.size() << "\n";
            sendSegment(TCP_FLAG_ACK, seqNum, ackNum);
        }
        break;
    }

    case TCPState::FIN_WAIT_1:
    {
        bool isFinAck = (seg.flags & TCP_FLAG_FIN) && (seg.flags & TCP_FLAG_ACK);
        if (isFinAck)
        {
            ackNum = seg.seqNum + 1;
            sendSegment(TCP_FLAG_ACK, seqNum, ackNum);
            transitionTo(TCPState::TIME_WAIT);
            break;
        }
        if (seg.flags & TCP_FLAG_ACK)
        {
            if (seg.ackNum != seqNum)
            {
                break;
            }
            transitionTo(TCPState::FIN_WAIT_2);
        }
        if (seg.flags & TCP_FLAG_FIN)
        {
            ackNum = seg.seqNum + 1;
            sendSegment(TCP_FLAG_ACK, seqNum, ackNum);
            transitionTo(TCPState::TIME_WAIT);
            break;
        }
        break;
    }

    case TCPState::FIN_WAIT_2:
    {
        if (seg.flags & TCP_FLAG_FIN)
        {
            ackNum = seg.seqNum + 1;
            sendSegment(TCP_FLAG_ACK, seqNum, ackNum);
            transitionTo(TCPState::TIME_WAIT);
        }
        break;
    }

    case TCPState::CLOSE_WAIT:
    {
        break;
    }

    case TCPState::LAST_ACK:
    {
        if (seg.flags & TCP_FLAG_ACK)
        {
            transitionTo(TCPState::CLOSED);
        }
        break;
    }

    case TCPState::TIME_WAIT:
    {
        transitionTo(TCPState::CLOSED);
        break;
    }

    default:
        break;
    }
}

void TCPSocket::setState(TCPState newState)
{
    if (state != newState)
    {
        std::string oldStateStr = stateToString();
        state = newState;
        std::string newStateStr = stateToString();

        std::cout << "[TCP Socket] State changed: "
                  << oldStateStr << " -> " << newStateStr << "\n";
    }
}