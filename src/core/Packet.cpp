#include "Packet.hpp"

#include <stdexcept>

Packet::Packet()
    : payload()
{
}

Packet::Packet(const vector<uint8_t> &payload)
    : payload(payload)
{
}

Packet::Packet(const string &payloadText)
    : payload(payloadText.begin(), payloadText.end())
{
}

const vector<uint8_t> &Packet::getPayload() const
{
    return payload;
}

string Packet::getPayloadAsString() const
{
    return string(payload.begin(), payload.end());
}

void Packet::setPayload(const vector<uint8_t> &newPayload)
{
    payload = newPayload;
}

void Packet::setPayload(const string &newPayloadText)
{
    payload = vector<uint8_t>(newPayloadText.begin(), newPayloadText.end());
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

    // Format:
    // [ Length: 4 bytes big-endian ][ Payload: N bytes ][ Checksum: 1 byte ]

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
    // Minimal:
    // Length 4 bytes + Checksum 1 byte = 5 bytes
    if (rawBytes.size() < 5)
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

    uint32_t length = 0;
    length |= static_cast<uint32_t>(rawBytes[0]) << 24;
    length |= static_cast<uint32_t>(rawBytes[1]) << 16;
    length |= static_cast<uint32_t>(rawBytes[2]) << 8;
    length |= static_cast<uint32_t>(rawBytes[3]);

    size_t expectedSize = 4 + length + 1;

    if (rawBytes.size() != expectedSize)
    {
        throw runtime_error("Invalid packet: payload length mismatch.");
    }

    vector<uint8_t> parsedPayload(rawBytes.begin() + 4, rawBytes.end() - 1);

    return Packet(parsedPayload);
}