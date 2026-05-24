#include "layer7/DNSMessage.hpp"
#include <sstream>
#include <stdexcept>

static void pushU16(std::vector<uint8_t> &buf, uint16_t v)
{
    buf.push_back(static_cast<uint8_t>(v >> 8));
    buf.push_back(static_cast<uint8_t>(v & 0xFF));
}
static uint16_t readU16(const std::vector<uint8_t> &buf, size_t offset)
{
    if (offset + 1 >= buf.size())
        throw std::runtime_error("[DNS] Buffer terlalu pendek saat baca uint16.");
    return (static_cast<uint16_t>(buf[offset]) << 8) | buf[offset + 1];
}

static void pushU32(std::vector<uint8_t> &buf, uint32_t v)
{
    buf.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
    buf.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>(v & 0xFF));
}

std::vector<uint8_t> DNSMessage::encodeName(const std::string &name)
{
    std::vector<uint8_t> out;
    std::istringstream ss(name);
    std::string label;
    while (std::getline(ss, label, '.'))
    {
        if (label.empty())
            continue;
        out.push_back(static_cast<uint8_t>(label.size()));
        for (char c : label)
            out.push_back(static_cast<uint8_t>(c));
    }
    out.push_back(0x00);
    return out;
}

std::string DNSMessage::decodeName(const std::vector<uint8_t> &raw, size_t &offset)
{
    std::string result;
    bool first = true;
    while (offset < raw.size())
    {
        uint8_t len = raw[offset];
        if ((len & 0xC0) == 0xC0)
        {
            if (offset + 1 >= raw.size())
                throw std::runtime_error("[DNS] Pointer compression offset invalid.");
            size_t ptr = ((len & 0x3F) << 8) | raw[offset + 1];
            offset += 2;
            std::string rest = decodeName(raw, ptr);
            result += (first ? "" : ".") + rest;
            return result;
        }

        if (len == 0)
        {
            offset++;
            break;
        }

        offset++;
        if (offset + len > raw.size())
            throw std::runtime_error("[DNS] Label melebihi ukuran buffer.");

        if (!first)
            result += '.';
        result += std::string(raw.begin() + offset, raw.begin() + offset + len);
        offset += len;
        first = false;
    }
    return result;
}
std::vector<uint8_t> DNSMessage::toBytes() const
{
    std::vector<uint8_t> buf;
    buf.reserve(64);

    pushU16(buf, id);
    pushU16(buf, flags);
    pushU16(buf, qdCount);
    pushU16(buf, anCount);
    pushU16(buf, 0);
    pushU16(buf, 0);
    if (qdCount > 0)
    {
        auto encoded = encodeName(qname);
        buf.insert(buf.end(), encoded.begin(), encoded.end());
        pushU16(buf, qtype);
        pushU16(buf, qclass);
    }

    if (anCount > 0 && !rdata.empty())
    {
        buf.push_back(0xC0);
        buf.push_back(0x0C);
        pushU16(buf, DNS_QTYPE_A);
        pushU16(buf, DNS_QCLASS_IN);
        pushU32(buf, ttl);
        pushU16(buf, 4);
        std::istringstream ss(rdata);
        std::string octet;
        int count = 0;
        while (std::getline(ss, octet, '.') && count < 4)
        {
            buf.push_back(static_cast<uint8_t>(std::stoi(octet)));
            count++;
        }
        if (count != 4)
            throw std::runtime_error("[DNS] Format IP pada rdata tidak valid: " + rdata);
    }

    return buf;
}

DNSMessage DNSMessage::fromBytes(const std::vector<uint8_t> &raw)
{
    if (raw.size() < 12)
        throw std::runtime_error("[DNS] Packet terlalu pendek (< 12 byte header).");

    DNSMessage msg;
    size_t offset = 0;

    msg.id = readU16(raw, offset);
    offset += 2;
    msg.flags = readU16(raw, offset);
    offset += 2;
    msg.qdCount = readU16(raw, offset);
    offset += 2;
    msg.anCount = readU16(raw, offset);
    offset += 2;
    offset += 2;
    offset += 2;

    if (msg.qdCount > 0)
    {
        msg.qname = decodeName(raw, offset);
        if (offset + 3 >= raw.size())
            throw std::runtime_error("[DNS] Buffer habis sebelum QTYPE/QCLASS.");
        msg.qtype = readU16(raw, offset);
        offset += 2;
        msg.qclass = readU16(raw, offset);
        offset += 2;
    }

    if (msg.anCount > 0 && offset < raw.size())
    {
        if (offset < raw.size() && (raw[offset] & 0xC0) == 0xC0)
        {
            offset += 2;
        }
        else
        {
            decodeName(raw, offset);
        }

        if (offset + 9 >= raw.size())
            throw std::runtime_error("[DNS] Buffer habis sebelum answer fields.");

        offset += 2;
        offset += 2;
        msg.ttl = 0;
        msg.ttl |= (static_cast<uint32_t>(raw[offset++]) << 24);
        msg.ttl |= (static_cast<uint32_t>(raw[offset++]) << 16);
        msg.ttl |= (static_cast<uint32_t>(raw[offset++]) << 8);
        msg.ttl |= (static_cast<uint32_t>(raw[offset++]));

        uint16_t rdlen = readU16(raw, offset);
        offset += 2;

        if (rdlen == 4 && offset + 3 < raw.size())
        {
            msg.rdata = std::to_string(raw[offset]) + "." +
                        std::to_string(raw[offset + 1]) + "." +
                        std::to_string(raw[offset + 2]) + "." +
                        std::to_string(raw[offset + 3]);
            offset += 4;
        }
    }

    return msg;
}