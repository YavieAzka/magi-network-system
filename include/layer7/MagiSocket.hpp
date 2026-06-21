#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <deque>
#include <functional>

class Host;
class TCPSocket;

constexpr int MAGI_AF_INET     = 2; 
constexpr int MAGI_SOCK_STREAM = 1; 
constexpr int MAGI_SOCK_DGRAM  = 2; 

class MagiSocket {
private:
    Host* hostNode_;
    int family_;
    int sockType_;
    TCPSocket* tcpSocket_;

    std::string localIp_;
    uint16_t localPort_;
    std::string remoteIp_;
    uint16_t remotePort_;

    bool isBound_;
    bool isListening_;
    bool isConnected_;
    bool isClosed_;

    std::deque<uint8_t> udpReceiveBuffer_;


    MagiSocket(Host*              host,
               int                sockType,
               int                family,
               const std::string& localIp,
               uint16_t           localPort,
               const std::string& remoteIp,
               uint16_t           remotePort,
               TCPSocket*         existingSocket);

    std::function<void(const std::string&, const std::vector<uint8_t>&)> onReceive_;
public:
    explicit MagiSocket(Host* host,
                        int   sock_type,
                        int   family = MAGI_AF_INET);
    ~MagiSocket();
    MagiSocket(const MagiSocket&)            = delete;
    MagiSocket& operator=(const MagiSocket&) = delete;
    MagiSocket(MagiSocket&&)                 = delete;
    MagiSocket& operator=(MagiSocket&&)      = delete;
    void bind(const std::string& ip, uint16_t port);

    void listen();

    MagiSocket* accept();
    void connect(const std::string& ip, uint16_t port);

    void send(const std::vector<uint8_t>& data);
    std::vector<uint8_t> recv(size_t max_bytes = 65535);
    void close();
    bool isBound()       const { return isBound_; }
    bool isConnected()   const { return isConnected_; }
    int  getSockType()   const { return sockType_; }
    int  getFamily()     const { return family_; }
    std::string getLocalIp()    const { return localIp_; }
    uint16_t getLocalPort()  const { return localPort_; }
    std::string getRemoteIp()   const { return remoteIp_; }
    uint16_t getRemotePort() const { return remotePort_; }
    TCPSocket* getTcpSocket() const { return tcpSocket_; }
    void pushUdpData(const std::string& srcIp, const std::vector<uint8_t>& data);
    void setOnReceive(std::function<void(const std::string&, const std::vector<uint8_t>&)> cb) {
        onReceive_ = cb;
    }
};