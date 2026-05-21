#include "layer7/MagiSocket.hpp"
#include "core/Host.hpp"
#include "layer4/TCPSocket.hpp"
#include <iostream>
#include <stdexcept>
#include <algorithm>


MagiSocket::MagiSocket(Host* host, int sock_type, int family)
    : hostNode_(host),
      family_(family),
      sockType_(sock_type),
      tcpSocket_(nullptr),
      localPort_(0),
      remotePort_(0),
      isBound_(false),
      isListening_(false),
      isConnected_(false),
      isClosed_(false) 
{
    if (!host) {
        throw std::invalid_argument("[MagiSocket] Pointer host tidak boleh null.");
    }
    if (sock_type != MAGI_SOCK_STREAM && sock_type != MAGI_SOCK_DGRAM) {
        throw std::invalid_argument("[MagiSocket] sockType tidak valid.");
    }
}

MagiSocket::MagiSocket(Host*              host,
                       int                sockType,
                       int                family,
                       const std::string& localIp,
                       uint16_t           localPort,
                       const std::string& remoteIp,
                       uint16_t           remotePort,
                       TCPSocket*         existingSocket)
    : hostNode_(host),
      family_(family),
      sockType_(sockType),
      tcpSocket_(existingSocket),
      localIp_(localIp),
      localPort_(localPort),
      remoteIp_(remoteIp),
      remotePort_(remotePort),
      isBound_(true),
      isListening_(false),
      isConnected_(true),
      isClosed_(false) 
{
}

MagiSocket::~MagiSocket() {
    if (!isClosed_) {
        try {
            close();
        } catch (const std::exception& e) {
            std::cerr << "[MagiSocket] Warning saat destructor memanggil close(): " << e.what() << "\n";
        } 
    }
}


void MagiSocket::bind(const std::string& ip, uint16_t port) {
    if (isClosed_) throw std::logic_error("[MagiSocket] Socket sudah ditutup.");
    if (isBound_) throw std::logic_error("[MagiSocket] Socket sudah di-bind.");

    localIp_ = ip;
    localPort_ = port;
    isBound_ = true;

    if (sockType_ == MAGI_SOCK_STREAM) {
        Host* host = hostNode_;
        uint16_t port = localPort_;
        auto cb = [host, port](const TCPSegment& seg, const std::string& dstIp, uint16_t /*dstPort*/) {
            host->sendTCP(dstIp, port, seg.dstPort, seg.toBytes());
        };
        tcpSocket_ = new TCPSocket(localIp_, localPort_, cb);
        hostNode_->getTcpSockets()[localPort_] = tcpSocket_;
        std::cout << "[MagiSocket] TCP bind ke " << localIp_ << ":" << localPort_ << "\n";
    } else {
        hostNode_->getUdpSockets()[localPort_] = this;
        std::cout << "[MagiSocket] UDP bind ke " << localIp_ << ":" << localPort_ << "\n";
    }
}

void MagiSocket::close() {
    if (isClosed_) return;

    if (sockType_ == MAGI_SOCK_STREAM && tcpSocket_) {
        std::cout << "[MagiSocket] Memulai TCP 4-way teardown untuk port " << localPort_ << "...\n";
        tcpSocket_->close();
        hostNode_->getTcpSockets().erase(localPort_);
        
        delete tcpSocket_;
        tcpSocket_ = nullptr;
    } else if (sockType_ == MAGI_SOCK_DGRAM) {
        std::cout << "[MagiSocket] Membersihkan UDP socket di port " << localPort_ << ".\n";
        hostNode_->getUdpSockets().erase(localPort_);
        udpReceiveBuffer_.clear();
    }

    isConnected_ = false;
    isListening_ = false;
    isClosed_ = true;
}

void MagiSocket::listen() {
    if (isClosed_) throw std::logic_error("[MagiSocket] Socket sudah ditutup.");
    if (sockType_ != MAGI_SOCK_STREAM) throw std::logic_error("[MagiSocket] listen() khusus TCP.");
    if (!isBound_) throw std::logic_error("[MagiSocket] Harus bind() sebelum listen().");

    tcpSocket_->passiveOpen();
    isListening_ = true;
    std::cout << "[MagiSocket] Listening di port " << localPort_ << "...\n";
}

MagiSocket* MagiSocket::accept() {
    if (isClosed_) throw std::logic_error("[MagiSocket] Socket sudah ditutup.");
    if (sockType_ != MAGI_SOCK_STREAM) throw std::logic_error("[MagiSocket] accept() khusus TCP.");
    if (!isListening_) throw std::logic_error("[MagiSocket] Harus listen() sebelum accept().");

    if (tcpSocket_->state == TCPState::LISTEN) {
        throw std::runtime_error("[MagiSocket] Belum ada koneksi TCP yang masuk.");
    }

    std::cout << "[MagiSocket] Menerima koneksi dari " << tcpSocket_->remoteIP << ":" << tcpSocket_->remotePort << "\n";

    MagiSocket* clientSocket = new MagiSocket(
        hostNode_, sockType_, family_,
        tcpSocket_->localIP, tcpSocket_->localPort,
        tcpSocket_->remoteIP, tcpSocket_->remotePort,
        tcpSocket_
    );

    tcpSocket_ = nullptr;
    isListening_ = false;

    return clientSocket;
}

void MagiSocket::connect(const std::string& ip, uint16_t port) {
    if (isClosed_) throw std::logic_error("[MagiSocket] Socket sudah ditutup.");
    if (sockType_ != MAGI_SOCK_STREAM) {
        remoteIp_ = ip;
        remotePort_ = port;
        isConnected_ = true;
        return;
    }

    if (!isBound_) throw std::logic_error("[MagiSocket] Harus bind() sebelum connect().");

    remoteIp_ = ip;
    remotePort_ = port;

    tcpSocket_->activeOpen(ip, port);
    isConnected_ = true;
    std::cout << "[MagiSocket] Connecting ke " << remoteIp_ << ":" << remotePort_ << "...\n";
}

void MagiSocket::send(const std::vector<uint8_t>& data) {
    if (isClosed_) throw std::logic_error("[MagiSocket] Socket sudah ditutup.");
    if (data.empty()) return;

    if (sockType_ == MAGI_SOCK_STREAM) {
        if (!tcpSocket_) throw std::logic_error("[MagiSocket] Socket TCP belum dikonfigurasi dengan benar.");
        tcpSocket_->sendData(data);
    } else {
        if (remoteIp_.empty() || remotePort_ == 0) {
            throw std::logic_error("[MagiSocket] Alamat remote tujuan UDP belum diset (gunakan connect).");
        }
        hostNode_->sendUDP(remoteIp_, localPort_, remotePort_, data);
        std::cout << "[MagiSocket] Mengirim UDP " << data.size() << " bytes ke " << remoteIp_ << ":" << remotePort_ << "\n";
    }
}

std::vector<uint8_t> MagiSocket::recv(size_t max_bytes) {
    if (isClosed_) throw std::logic_error("[MagiSocket] Socket sudah ditutup.");

    std::vector<uint8_t> result;

    if (sockType_ == MAGI_SOCK_STREAM) {
        if (!tcpSocket_) return result;
        result = tcpSocket_->recvData();
        if (result.size() > max_bytes) {
            result.resize(max_bytes);
        }
    } else {
        size_t bytesToRead = std::min(max_bytes, udpReceiveBuffer_.size());
        result.assign(udpReceiveBuffer_.begin(), udpReceiveBuffer_.begin() + bytesToRead);
        udpReceiveBuffer_.erase(udpReceiveBuffer_.begin(), udpReceiveBuffer_.begin() + bytesToRead);
    }

    return result;
}

void MagiSocket::pushUdpData(const std::vector<uint8_t>& data) {
    udpReceiveBuffer_.insert(udpReceiveBuffer_.end(), data.begin(), data.end());
    std::cout << "[MagiSocket] UDP buffer menerima " << data.size() << " bytes tambahan.\n";
    if (onReceive_) {
        onReceive_(data);
    }
}
