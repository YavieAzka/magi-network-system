#include "layer7/DNSClient.hpp"
#include "core/Host.hpp"
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <ctime>

DNSClient::DNSClient(Host *host)
    : host_(host), socket_(nullptr)
{
    if (!host_)
        throw std::invalid_argument("[DNSClient] Pointer host tidak boleh null.");
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
        std::cerr << "[DNSClient:" << host_->getName() << "] "
                  << "Gagal terima DNS response: " << e.what() << "\n";
        socket_->close();
        delete socket_;
        socket_ = nullptr;
        return "";
    }

    socket_->close();
    delete socket_;
    socket_ = nullptr;

    if (response.id != query.id)
    {
        std::cerr << "[DNSClient:" << host_->getName() << "] "
                  << "ID response tidak cocok (got " << response.id
                  << ", expect " << query.id << "), diabaikan.\n";
        return "";
    }
    if (response.isNxDomain())
    {
        std::cout << "[DNSClient:" << host_->getName() << "] "
                  << "Domain \"" << domain << "\" tidak ditemukan (NXDOMAIN).\n";
        return "";
    }

    if (response.rdata.empty())
    {
        std::cerr << "[DNSClient:" << host_->getName() << "] "
                  << "Response kosong untuk domain \"" << domain << "\".\n";
        return "";
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
        throw std::runtime_error("[DNSClient] Response kosong / timeout.");

    return DNSMessage::fromBytes(raw);
}