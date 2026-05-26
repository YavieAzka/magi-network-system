#pragma once
#include "layer7/DNSMessage.hpp"
#include "layer7/MagiSocket.hpp"
#include <string>
#include <unordered_map>
#include <vector>

class Host;

class DNSServer
{
public:
    explicit DNSServer(Host *host,
                       const std::unordered_map<std::string, std::string> &records = {});
    ~DNSServer();

    void addRecord(const std::string &domain, const std::string &ip);
    void start();
    void handleIncoming(const std::string &srcIp,
                        const std::vector<uint8_t> &udpPayload);
    void printRecords() const;

private:
    Host *host_;
    MagiSocket *socket_;

    std::unordered_map<std::string, std::string> records_;

    void sendResponse(const DNSMessage &response,
                      const std::string &dstIp,
                      uint16_t dstPort);
    static std::string toLower(const std::string &s);
};