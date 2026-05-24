#pragma once
#include <cstdint>
#include <string>
#include <vector>

constexpr uint16_t DNS_SERVER_PORT = 53;
constexpr uint16_t DNS_CLIENT_PORT = 1053;
constexpr uint16_t DNS_QTYPE_A = 1;
constexpr uint16_t DNS_QCLASS_IN = 1;

constexpr uint16_t DNS_FLAG_QR_RESPONSE = 0x8000;
constexpr uint16_t DNS_FLAG_AA = 0x0400;
constexpr uint16_t DNS_FLAG_RD = 0x0100;
constexpr uint16_t DNS_FLAG_RA = 0x0080;

constexpr uint16_t DNS_RCODE_OK = 0;
constexpr uint16_t DNS_RCODE_NXDOMAIN = 3;

// ============================================================
//  DNSMessage
//  Format minimal untuk A record query & response.
//
//  Header (12 byte):
//    ID       (2B)
//    Flags    (2B)
//    QDCount  (2B) — jumlah question
//    ANCount  (2B) — jumlah answer
//    NSCount  (2B) — selalu 0
//    ARCount  (2B) — selalu 0
//
//  Question section:
//    QNAME    (variable, label encoding)
//    QTYPE    (2B) = 1 (A)
//    QCLASS   (2B) = 1 (IN)
//
//  Answer section (hanya jika response & RCODE=OK):
//    NAME     (2B, pointer 0xC00C ke question)
//    TYPE     (2B) = 1
//    CLASS    (2B) = 1
//    TTL      (4B)
//    RDLENGTH (2B) = 4
//    RDATA    (4B) — IPv4 address
// ============================================================

struct DNSMessage
{
    uint16_t id = 0;
    uint16_t flags = 0;
    uint16_t qdCount = 0;
    uint16_t anCount = 0;

    std::string qname; // domain name, e.g. "www.magi.com"
    uint16_t qtype = DNS_QTYPE_A;
    uint16_t qclass = DNS_QCLASS_IN;

    std::string rdata; // IP address hasil resolusi, e.g. "10.0.0.5"
    uint32_t ttl = 60;

        bool isResponse() const { return (flags & DNS_FLAG_QR_RESPONSE) != 0; }
    bool isNxDomain() const { return (flags & 0x000F) == DNS_RCODE_NXDOMAIN; }

    std::vector<uint8_t> toBytes() const;

    static DNSMessage fromBytes(const std::vector<uint8_t> &raw);

private:
    static std::vector<uint8_t> encodeName(const std::string &name);
    static std::string decodeName(const std::vector<uint8_t> &raw, size_t &offset);
};