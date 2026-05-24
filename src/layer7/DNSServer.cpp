#include "layer7/DNSServer.hpp"
#include "core/Host.hpp"
#include <iostream>
#include <algorithm>
#include <stdexcept>

DNSServer::DNSServer(Host *host,
                     const std::unordered_map<std::string, std::string> &records)
    : host_(host), socket_(nullptr), records_(records)
{
    if (!host_)
        throw std::invalid_argument("[DNSServer] Pointer host tidak boleh null.");
}

DNSServer::~DNSServer()
{
    if (socket_)
    {
        socket_->close();
        delete socket_;
        socket_ = nullptr;
    }
}

void DNSServer::addRecord(const std::string &domain, const std::string &ip)
{
    records_[toLower(domain)] = ip;
    std::cout << "[DNSServer:" << host_->getName() << "] "
              << "Record ditambah: " << toLower(domain) << " -> " << ip << "\n";
}

void DNSServer::start()
{
    socket_ = new MagiSocket(host_, MAGI_SOCK_DGRAM, MAGI_AF_INET);
    socket_->bind(host_->getIpAddress(), DNS_SERVER_PORT);

    // Daftarkan callback agar setiap data UDP masuk langsung diproses
    socket_->setOnReceive([this](const std::vector<uint8_t> &data)
                          { this->handleIncoming("0.0.0.0", data); });

    std::cout << "[DNSServer:" << host_->getName() << "] "
              << "DNS Server aktif di " << host_->getIpAddress()
              << ":" << DNS_SERVER_PORT << "\n";
}

void DNSServer::handleIncoming(const std::string &srcIp,
                               const std::vector<uint8_t> &udpPayload)
{
    // udpPayload adalah isi UDPSegment.payload (sudah dikupas dari header UDP
    // oleh Host::handleUDPSegment).
    // Tapi di sini kita terima raw UDP bytes termasuk header,
    // konsisten dengan pola DHCPServer::handleIncoming.

    DNSMessage query;
    try
    {
        query = DNSMessage::fromBytes(udpPayload);
    }
    catch (const std::exception &e)
    {
        std::cerr << "[DNSServer:" << host_->getName() << "] "
                  << "Gagal parse DNS query dari " << srcIp
                  << ": " << e.what() << "\n";
        return;
    }

    if (query.isResponse())
    {
        // Bukan query, abaikan
        return;
    }

    std::cout << "[DNSServer:" << host_->getName() << "] "
              << "Menerima DNS query dari " << srcIp
              << " untuk domain \"" << query.qname << "\"\n";

    // Cari di record table (case-insensitive)
    std::string key = toLower(query.qname);
    auto it = records_.find(key);

    DNSMessage response;
    response.id = query.id;
    response.qdCount = 1;
    response.qname = query.qname;
    response.qtype = query.qtype;
    response.qclass = query.qclass;

    if (it != records_.end())
    {
        // Record ditemukan — kirim answer
        response.flags = DNS_FLAG_QR_RESPONSE | DNS_FLAG_AA | DNS_FLAG_RD | DNS_FLAG_RA;
        response.anCount = 1;
        response.rdata = it->second;
        response.ttl = 60;

        std::cout << "[DNSServer:" << host_->getName() << "] "
                  << "Resolve \"" << query.qname << "\" -> " << it->second << "\n";
    }
    else
    {
        // NXDOMAIN
        response.flags = DNS_FLAG_QR_RESPONSE | DNS_FLAG_AA | DNS_FLAG_RD | DNS_FLAG_RA | DNS_RCODE_NXDOMAIN;
        response.anCount = 0;

        std::cout << "[DNSServer:" << host_->getName() << "] "
                  << "Domain \"" << query.qname << "\" tidak ditemukan (NXDOMAIN).\n";
    }

    // Kirim response ke client
    // srcIp adalah IP pengirim query. Port balikan adalah DNS_CLIENT_PORT.
    sendResponse(response, srcIp, DNS_CLIENT_PORT);
}

void DNSServer::sendResponse(const DNSMessage &response,
                             const std::string &dstIp,
                             uint16_t dstPort)
{
    std::vector<uint8_t> payload = response.toBytes();

    // Broadcast response — dalam simulator single-threaded, client yang
    // punya socket di dstPort akan menerima ini. Pola sama dengan DHCPServer.
    host_->sendUDPBroadcast(DNS_SERVER_PORT, dstPort, payload);

    std::cout << "[DNSServer:" << host_->getName() << "] "
              << "DNS response dikirim ke " << dstIp << ":" << dstPort
              << " (" << payload.size() << " bytes)\n";
}

void DNSServer::printRecords() const
{
    std::cout << "\n[DNSServer:" << host_->getName() << "] Record table:\n";
    if (records_.empty())
    {
        std::cout << "  (kosong)\n";
        return;
    }
    for (const auto &[domain, ip] : records_)
    {
        std::cout << "  " << domain << "  ->  " << ip << "\n";
    }
    std::cout << "\n";
}

std::string DNSServer::toLower(const std::string &s)
{
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c)
                   { return std::tolower(c); });
    return out;
}