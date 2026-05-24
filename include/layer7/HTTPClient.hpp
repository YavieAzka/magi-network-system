#pragma once
#include <string>
#include <vector>
#include <cstdint>

class Host;
class MagiSocket;

class HTTPClient {
public:
    explicit HTTPClient(Host* host);
    ~HTTPClient();

    std::string get(const std::string& url,const std::string& dnsServerIp = "");

    bool connectAndSend(const std::string& url,const std::string& dnsServerIp = "");

    std::string recvAndClose();

    static bool isIpAddress(const std::string& str);
    static void parseUrl(const std::string& url,std::string& outHost,std::string& outPath);

private:
    Host* host_;
    MagiSocket* sock_;   
    std::string urlHost_;

    static uint16_t nextEphemeralPort_;
    static std::string extractBody(const std::string& response);
};