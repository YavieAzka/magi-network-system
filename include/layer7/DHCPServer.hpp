#pragma once
#include "layer7/DHCPMessage.hpp"
#include "layer7/MagiSocket.hpp"
#include <string>
#include <map>
#include <vector>

class Host;

class DHCPServer {
public:
    // host        node tempat server ini berjalan
    // poolStart   IP pertama yang boleh dialokasikan
    // poolEnd     IP terakhir yang boleh dialokasikan
    // gateway     default gateway yang akan diberitahu ke client
    // prefixLen   panjang prefix subnet
    DHCPServer(Host* host,
               const std::string& poolStart,
               const std::string& poolEnd,
               const std::string& gateway,
               uint8_t prefixLen);

    ~DHCPServer();
    void start();
    void handleIncoming(const std::string& srcIp,
                        const std::vector<uint8_t>& udpPayload);
    void printLeases() const;

private:
    Host* host_;
    MagiSocket* socket_;

    std::string gateway_;
    uint8_t prefixLen_;
    std::string serverIp_;  

    std::vector<std::string> ipPool_;

    // MAC address client -> IP yang sudah diberikan
    std::map<std::string, std::string> leaseTable_;

    // IP yang sedang di-offer tapi belum di-ACK
    // MAC -> IP yang ditawarkan di Offer
    std::map<std::string, std::string> pendingOffers_;

    // Mencari IP yang belum terpakai dari pool
    // Return "" jika pool sudah habis
    std::string allocateIp(const std::string& clientMac);

    void handleDiscover(const DHCPMessage& msg, const std::string& srcIp);
    void handleRequest (const DHCPMessage& msg, const std::string& srcIp);
    void sendReply(const DHCPMessage& reply, const std::string& dstIp);
    static uint32_t ipToUint32(const std::string& ip);
    static std::string uint32ToIp(uint32_t ip);
};