#include "layer7/DNSClient.hpp"
#include "util/Exceptions.hpp"
#include "core/Host.hpp"
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <ctime>

DNSClient::DNSClient(Host *host)
    : host_(host), socket_(nullptr)
{
    if (!host_)
        throw std::invalid_argument("DNSClient pointer host cannot be null");
}

DNSClient::~DNSClient()
{
    if (socket_)
    {
        socket_->close();
        delete socket_;
        socket_ = nullptr;
    }
}

std::string DNSClient::resolve(const std::string &serverIp,
                               const std::string &domain)
{
    std::cout << "[DNSClient:" << host_->getName() << "] "
              << "Resolve \"" << domain << "\" via DNS server " << serverIp << "...\n";

    socket_ = new MagiSocket(host_, MAGI_SOCK_DGRAM, MAGI_AF_INET);
    socket_->bind(host_->getIpAddress(), DNS_CLIENT_PORT);

    DNSMessage query;
    query.id = static_cast<uint16_t>(std::rand() & 0xFFFF);
    query.flags = DNS_FLAG_RD;
    query.qdCount = 1;
    query.anCount = 0;
    query.qname = domain;
    query.qtype = DNS_QTYPE_A;
    query.qclass = DNS_QCLASS_IN;

    sendQuery(query, serverIp);

    DNSMessage response;
    try
    {
        response = recvResponse();
    }
    catch (const std::exception &e)
    {
        socket_->close();
        delete socket_;
        socket_ = nullptr;
        throw NetworkException("DNSClient on " + host_->getName() + " failed to receive DNS response: " + e.what());
    }

    socket_->close();
    delete socket_;
    socket_ = nullptr;

    if (response.id != query.id)
    {
        throw NetworkException("DNSClient on " + host_->getName() + " ID response mismatch (got " + std::to_string(response.id) + ", expected " + std::to_string(query.id) + "), ignored.");
    }
    if (response.isNxDomain())
    {
        std::cout << "[DNSClient:" << host_->getName() << "] "
                  << "Domain \"" << domain << "\" tidak ditemukan (NXDOMAIN).\n";
        return "";
    }

    if (response.rdata.empty())
    {
        throw NetworkException("DNSClient on " + host_->getName() + " empty response for domain " + domain);
    }

    std::cout << "[DNSClient:" << host_->getName() << "] "
              << "Resolve berhasil: \"" << domain << "\" -> " << response.rdata << "\n";

    return response.rdata;
}

void DNSClient::sendQuery(const DNSMessage &query, const std::string &serverIp)
{
    std::vector<uint8_t> payload = query.toBytes();

    socket_->connect(serverIp, DNS_SERVER_PORT);
    socket_->send(payload);

    std::cout << "[DNSClient:" << host_->getName() << "] "
              << "DNS query dikirim ke " << serverIp << ":" << DNS_SERVER_PORT
              << " (" << payload.size() << " bytes)\n";
}

DNSMessage DNSClient::recvResponse()
{
    std::vector<uint8_t> raw = socket_->recv(512);

    if (raw.empty())
        throw NetworkException("DNSClient empty response or timeout");

    return DNSMessage::fromBytes(raw);
}