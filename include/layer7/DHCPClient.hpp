#pragma once
#include "layer7/DHCPMessage.hpp"
#include "layer7/MagiSocket.hpp"
#include <string>

class Host;

class DHCPClient {
public:
    explicit DHCPClient(Host* host);

    ~DHCPClient();
    bool discover();

private:
    Host* host_;
    MagiSocket* socket_; 

    uint32_t xid_;

    DHCPMessage recvMessage();
    void sendMessage(const DHCPMessage& msg, const std::string& dstIp);
};