#pragma once
#include "core/Host.hpp"
#include "layer4/TCPSocket.hpp"
#include "layer7/HTTPMessage.hpp"
#include <string>
#include <map>
#include <functional>
#include <iostream>
#include <fstream>
#include <sstream>

class HTTPServer
{
public:
    using Handler = std::function<HTTPResponse(const HTTPRequest &)>;

    HTTPServer(Host &host, uint16_t port = 80);

    void start(Handler handler);
    void startDefault(const std::string &webRoot = "");

    Host &getHost() { return host_; }
    uint16_t getPort() const { return port_; }

private:
    Host &host_;
    uint16_t port_;
    TCPSocket *socket_;
    Handler handler_;

    void onSegmentReceived(const TCPSegment &seg);
    bool tryParseRequest(const std::vector<uint8_t> &buffer, HTTPRequest &req);
    void sendResponse(HTTPResponse &resp);
};

class HTTPClient
{
public:
    HTTPClient(Host &host);

    HTTPResponse get(const std::string &serverIp, uint16_t serverPort, const std::string &path);
    void getAsync(const std::string &serverIp, uint16_t serverPort, const std::string &path);

    bool hasResponse() const { return responseReady_; }
    HTTPResponse getResponse() const { return lastResponse_; }

private:
    Host &host_;
    TCPSocket *socket_;
    HTTPResponse lastResponse_;
    bool responseReady_;
};