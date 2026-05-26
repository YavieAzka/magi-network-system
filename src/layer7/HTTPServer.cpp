#include "layer7/HTTPServer.hpp"
#include <chrono>
#include <thread>

HTTPServer::HTTPServer(Host &host, uint16_t port)
    : host_(host), port_(port), socket_(nullptr)
{
}

void HTTPServer::start(Handler handler)
{
    handler_ = handler;
    socket_ = new TCPSocket(host_.getIpAddress(), port_,
        [this](const TCPSegment &seg, const std::string &dstIp, uint16_t dstPort)
        {
            std::vector<uint8_t> tcpBytes = seg.toBytes();
            host_.sendTCP(dstIp, seg.srcPort, dstPort, tcpBytes);
        });
    socket_->passiveOpen();
    host_.getTcpSockets()[port_] = socket_;
    std::cout << "[HTTPServer " << host_.getName() << ":" << port_ << "] Listening on port " << port_ << "\n";
}

void HTTPServer::startDefault(const std::string &webRoot)
{
    auto fileHandler = [webRoot](const HTTPRequest &req) -> HTTPResponse
    {
        if (req.method != "GET")
            return HTTPResponse::make400("Method Not Allowed");

        std::string filePath = webRoot + req.path;
        if (filePath.empty() || filePath.back() == '/')
            filePath += "index.html";

        std::ifstream file(filePath);
        if (!file.is_open())
            return HTTPResponse::make404();

        std::ostringstream content;
        content << file.rdbuf();
        file.close();

        std::string ext;
        size_t dot = filePath.rfind('.');
        if (dot != std::string::npos)
            ext = filePath.substr(dot + 1);

        std::string contentType = "text/html";
        if (ext == "txt")
            contentType = "text/plain";
        else if (ext == "json")
            contentType = "application/json";
        else if (ext == "css")
            contentType = "text/css";
        else if (ext == "js")
            contentType = "application/javascript";

        return HTTPResponse::make200(contentType, content.str());
    };

    start(fileHandler);
}

bool HTTPServer::tryParseRequest(const std::vector<uint8_t> &buffer, HTTPRequest &req)
{
    std::string raw(buffer.begin(), buffer.end());
    size_t headerEnd = raw.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        return false;

    req = HTTPRequest::deserialize(raw);
    return true;
}

void HTTPServer::onSegmentReceived(const TCPSegment &seg)
{
    if (socket_->state != TCPState::ESTABLISHED)
        return;
}

void HTTPServer::sendResponse(HTTPResponse &resp)
{
    if (socket_->state != TCPState::ESTABLISHED)
    {
        std::cout << "[HTTPServer " << host_.getName() << ":" << port_
                  << "] Cannot send response: not ESTABLISHED\n";
        return;
    }

    std::string serialized = resp.serialize();
    std::vector<uint8_t> payload(serialized.begin(), serialized.end());
    socket_->sendData(payload);
    std::cout << "[HTTPServer " << host_.getName() << ":" << port_
              << "] Sent " << resp.statusCode << " " << resp.reasonPhrase
              << " (" << payload.size() << " bytes)\n";
}

void HTTPServer::serveOne() {
    if (!socket_ || socket_->state == TCPState::CLOSED) return;
    
    // Baca request dari buffer
    auto data = socket_->recvData();
    if (data.empty()) return;
    
    HTTPRequest req;
    std::string raw(data.begin(), data.end());
    if (!tryParseRequest(data, req)) return;
    
    std::cout << "[HTTPServer] " << req.method << " " << req.path << "\n";
    
    // Generate response via handler
    HTTPResponse resp = handler_(req);
    sendResponse(resp);
}