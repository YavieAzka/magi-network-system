#include "layer7/HTTPClient.hpp"
#include "util/Exceptions.hpp"
#include "layer7/DNSClient.hpp"
#include "layer7/MagiSocket.hpp"
#include "layer7/HTTPMessage.hpp"
#include "core/Host.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>

uint16_t HTTPClient::nextEphemeralPort_ = 49152;

HTTPClient::HTTPClient(Host* host)
    : host_(host), sock_(nullptr) {
    if (!host_) throw invalid_argument("[HTTPClient] Host tidak boleh null.");
}

HTTPClient::~HTTPClient() {
    if (sock_) {
        try { sock_->close(); } catch (...) {}
        delete sock_;
        sock_ = nullptr;
    }
}

bool HTTPClient::isIpAddress(const string& str) {
    istringstream ss(str);
    string octet;
    int count = 0;
    while (getline(ss, octet, '.')) {
        if (octet.empty()) return false;
        if (octet.size() > 3) return false;
        for (char c : octet)
            if (c < '0' || c > '9') return false;
        if (octet.size() > 1 && octet[0] == '0') return false;
        int val = stoi(octet);
        if (val < 0 || val > 255) return false;
        count++;
    }
    return count == 4;
}

void HTTPClient::parseUrl(const string& url, string& outHost, string& outPath) {
    string s = url;
    if (s.substr(0, 7) == "http://") s = s.substr(7);
    size_t slashPos = s.find('/');
    if (slashPos == string::npos) {
        outHost = s;
        outPath = "/";
    } else {
        outHost = s.substr(0, slashPos);
        outPath = s.substr(slashPos);
    }
}

string HTTPClient::extractBody(const string& response) {
    const string sep = "\r\n\r\n";
    size_t pos = response.find(sep);
    if (pos == string::npos) return response;
    return response.substr(pos + sep.size());
}


bool HTTPClient::connectAndSend(const string& url, const string& dnsServerIp) {
    string urlPath;
    parseUrl(url, urlHost_, urlPath);

    cout << "[HTTPClient] HTTP GET " << urlHost_ << urlPath << "\n";

    // Resolve IP
    string targetIp;
    if (isIpAddress(urlHost_)) {
        targetIp = urlHost_;
        cout << "[HTTPClient] Target IP langsung: " << targetIp << "\n";
    } else {
        if (dnsServerIp.empty()) {
            throw NetworkException("HTTPClient DNS server IP is required");
        }
        DNSClient dns(host_);
        targetIp = dns.resolve(dnsServerIp, urlHost_);
        if (targetIp.empty()) {
            throw NetworkException("HTTPClient DNS resolution failed for " + urlHost_);
        }
        cout << "[HTTPClient] DNS: " << urlHost_ << " -> " << targetIp << "\n";
    }

    // Buat socket + connect
    uint16_t port = nextEphemeralPort_++;
    if (nextEphemeralPort_ > 65535) nextEphemeralPort_ = 49152;

    try {
        sock_ = new MagiSocket(host_, MAGI_SOCK_STREAM, MAGI_AF_INET);
        sock_->bind(host_->getIpAddress(), port);
        sock_->connect(targetIp, 80);

        // Kirim HTTP request
        string request =
            "GET " + urlPath + " HTTP/1.1\r\n"
            "Host: " + urlHost_ + "\r\n"
            "Connection: close\r\n"
            "\r\n";
        vector<uint8_t> reqBytes(request.begin(), request.end());
        sock_->send(reqBytes);
        cout << "[HTTPClient] Request terkirim (" << reqBytes.size() << " bytes)\n";
        return true;

    } catch (const exception& e) {
        if (sock_) { try { sock_->close(); } catch (...) {} delete sock_; sock_ = nullptr; }
        throw NetworkException("HTTPClient connectAndSend error: " + string(e.what()));
    }
}

string HTTPClient::recvAndClose() {
    if (!sock_) return "[ERR] Tidak ada koneksi aktif.";
    try {
        vector<uint8_t> respBytes = sock_->recv();
        string rawResponse(respBytes.begin(), respBytes.end());
        cout << "[HTTPClient] Response diterima (" << respBytes.size() << " bytes)\n";

        sock_->close();
        delete sock_;
        sock_ = nullptr;

        HTTPResponse resp = HTTPResponse::deserialize(rawResponse);
        cout << "[HTTPClient] HTTP/" << resp.version << " " << resp.statusCode << " " << resp.reasonPhrase << "\n";
        cout << "[HTTPClient] Body (" << resp.body.size() << " bytes):\n" << resp.body << "\n";
        return resp.body;

    } catch (const exception& e) {
        if (sock_) { try { sock_->close(); } catch (...) {} delete sock_; sock_ = nullptr; }
        throw NetworkException("HTTPClient recvAndClose error: " + string(e.what()));
    }
}


string HTTPClient::get(const string& url, const string& dnsServerIp) {
    if (!connectAndSend(url, dnsServerIp)) return "[ERR] Gagal connect.";
    return recvAndClose();
}