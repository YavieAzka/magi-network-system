#pragma once
#include "layer7/DNSMessage.hpp"
#include "layer7/MagiSocket.hpp"
#include <string>

class Host;
class DNSClient
{
public:
    explicit DNSClient(Host *host);
    ~DNSClient();

    std::string resolve(const std::string &serverIp,
                        const std::string &domain);

private:
    Host *host_;
    MagiSocket *socket_;

    void sendQuery(const DNSMessage &query, const std::string &serverIp);
    DNSMessage recvResponse();
};