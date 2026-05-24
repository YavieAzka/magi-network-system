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

HTTPClient::HTTPClient(Host &host)
    : host_(host), socket_(nullptr), responseReady_(false)
{
}

HTTPResponse HTTPClient::get(const std::string &serverIp, uint16_t serverPort, const std::string &path)
{
    responseReady_ = false;
    uint16_t localPort = 49152;

    while (host_.getTcpSockets().find(localPort) != host_.getTcpSockets().end())
        localPort++;

    socket_ = new TCPSocket(host_.getIpAddress(), localPort,
        [this](const TCPSegment &seg, const std::string &dstIp, uint16_t dstPort)
        {
            std::vector<uint8_t> tcpBytes = seg.toBytes();
            host_.sendTCP(dstIp, seg.srcPort, dstPort, tcpBytes);
        });

    host_.getTcpSockets()[localPort] = socket_;

    socket_->activeOpen(serverIp, serverPort);

    auto startTime = std::chrono::steady_clock::now();
    while (socket_->state != TCPState::ESTABLISHED)
    {
        if (socket_->state == TCPState::CLOSED)
        {
            std::cout << "[HTTPClient " << host_.getName() << "] Connection failed\n";
            return HTTPResponse::make400("Connection Failed");
        }
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count() >= 5000)
        {
            std::cout << "[HTTPClient " << host_.getName() << "] Connection timeout\n";
            return HTTPResponse::make400("Connection Timeout");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    HTTPRequest req;
    req.method = "GET";
    req.path = path;
    req.version = "HTTP/1.1";
    req.headers["Host"] = serverIp;
    req.headers["Connection"] = "close";
    req.headers["User-Agent"] = "MagiHTTP/1.0";

    std::string serialized = req.serialize();
    std::vector<uint8_t> payload(serialized.begin(), serialized.end());
    socket_->sendData(payload);

    std::cout << "[HTTPClient " << host_.getName() << "] Sent GET " << path
              << " to " << serverIp << ":" << serverPort << "\n";

    startTime = std::chrono::steady_clock::now();
    while (true)
    {
        auto data = socket_->recvData();
        if (!data.empty())
        {
            std::string raw(data.begin(), data.end());
            HTTPResponse resp = HTTPResponse::deserialize(raw);
            lastResponse_ = resp;
            responseReady_ = true;
            std::cout << "[HTTPClient " << host_.getName() << "] Received "
                      << resp.statusCode << " " << resp.reasonPhrase << "\n";
            return resp;
        }

        if (socket_->state == TCPState::CLOSE_WAIT ||
            socket_->state == TCPState::TIME_WAIT ||
            socket_->state == TCPState::CLOSED)
        {
            data = socket_->recvData();
            if (!data.empty())
            {
                std::string raw(data.begin(), data.end());
                lastResponse_ = HTTPResponse::deserialize(raw);
                responseReady_ = true;
                return lastResponse_;
            }
            std::cout << "[HTTPClient " << host_.getName() << "] Connection closed by server\n";
            return HTTPResponse::make400("Connection Closed");
        }

        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count() >= 5000)
        {
            std::cout << "[HTTPClient " << host_.getName() << "] Response timeout\n";
            data = socket_->recvData();
            if (!data.empty())
            {
                std::string raw(data.begin(), data.end());
                lastResponse_ = HTTPResponse::deserialize(raw);
                responseReady_ = true;
                return lastResponse_;
            }
            return HTTPResponse::make400("Response Timeout");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void HTTPClient::getAsync(const std::string &serverIp, uint16_t serverPort, const std::string &path)
{
    HTTPResponse resp = get(serverIp, serverPort, path);
}