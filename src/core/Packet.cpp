#include "Packet.hpp"

#include <stdexcept>

Packet::Packet()
    : type(TYPE_BASIC), payload()
{
}

Packet::Packet(uint8_t type, const vector<uint8_t> &payload)
    : type(type), payload(payload)
{
    if (!isValidType(type))
    {
        throw runtime_error("Invalid packet: unknown packet type.");
    }
}

Packet::Packet(uint8_t type, const string &payloadText)
    : type(type), payload(payloadText.begin(), payloadText.end())
{
    if (!isValidType(type))
    {
        throw runtime_error("Invalid packet: unknown packet type.");
    }
}

uint8_t Packet::getType() const
{
    return type;
}

const vector<uint8_t> &Packet::getPayload() const
{
    return payload;
}

string Packet::getPayloadAsString() const
{
    return string(payload.begin(), payload.end());
}

void Packet::setType(uint8_t newType)
{
    if (!isValidType(newType))
    {
        throw runtime_error("Invalid packet: unknown packet type.");
    }

    type = newType;
}

void Packet::setPayload(const vector<uint8_t> &newPayload)
{
    payload = newPayload;
}

void Packet::setPayload(const string &newPayloadText)
{
    payload = vector<uint8_t>(newPayloadText.begin(), newPayloadText.end());
}

bool Packet::isValidType(uint8_t type)
{
    return type == TYPE_BASIC || type == TYPE_UNKNOWN;
}

uint8_t Packet::calculateChecksum(const vector<uint8_t> &bytes)
{
    uint8_t result = 0x00;

    for (uint8_t byte : bytes)
    {
        result ^= byte;
    }

    return result;
}

vector<uint8_t> Packet::toBytes() const
{
    vector<uint8_t> bytes;

    uint32_t length = static_cast<uint32_t>(payload.size());

    // [ Type: 1 byte ][ Length: 4 bytes big-endian ][ Payload: N bytes ][ Checksum: 1 byte ]

    bytes.push_back(type);

    bytes.push_back(static_cast<uint8_t>((length >> 24) & 0xFF));
    bytes.push_back(static_cast<uint8_t>((length >> 16) & 0xFF));
    bytes.push_back(static_cast<uint8_t>((length >> 8) & 0xFF));
    bytes.push_back(static_cast<uint8_t>(length & 0xFF));

    bytes.insert(bytes.end(), payload.begin(), payload.end());

    uint8_t checksum = calculateChecksum(bytes);
    bytes.push_back(checksum);

    return bytes;
}

Packet Packet::fromBytes(const vector<uint8_t> &rawBytes)
{
    // Minimal: Type 1 byte + Length 4 bytes + Checksum 1 byte = 6 bytes
    if (rawBytes.size() < 6)
    {
        throw runtime_error("Invalid packet: raw bytes too short.");
    }

    uint8_t receivedChecksum = rawBytes.back();

    vector<uint8_t> bytesWithoutChecksum(rawBytes.begin(), rawBytes.end() - 1);
    uint8_t computedChecksum = calculateChecksum(bytesWithoutChecksum);

    if (receivedChecksum != computedChecksum)
    {
        throw runtime_error("Invalid packet: checksum mismatch.");
    }

    uint8_t parsedType = rawBytes[0];

    if (!isValidType(parsedType))
    {
        throw runtime_error("Invalid packet: unknown packet type.");
    }

    uint32_t length = 0;
    length |= static_cast<uint32_t>(rawBytes[1]) << 24;
    length |= static_cast<uint32_t>(rawBytes[2]) << 16;
    length |= static_cast<uint32_t>(rawBytes[3]) << 8;
    length |= static_cast<uint32_t>(rawBytes[4]);

    size_t expectedSize = 1 + 4 + length + 1;

    if (rawBytes.size() != expectedSize)
    {
        throw runtime_error("Invalid packet: payload length mismatch.");
    }

    vector<uint8_t> parsedPayload(rawBytes.begin() + 5, rawBytes.end() - 1);

    return Packet(parsedType, parsedPayload);
}