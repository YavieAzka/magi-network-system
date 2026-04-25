#ifndef PACKET_HPP
#define PACKET_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <cstddef>
using namespace std;

class Packet
{
public:
    static constexpr uint8_t TYPE_BASIC = 0x01;
    static constexpr uint8_t TYPE_UNKNOWN = 0xFF;

private:
    // Format byte: [ Type ][ Length ][ Payload ][ Checksum ]
    uint8_t type;
    vector<uint8_t> payload;

    static uint8_t calculateChecksum(const vector<uint8_t> &bytes);
    static bool isValidType(uint8_t type);

public:
    Packet();
    Packet(uint8_t type, const vector<uint8_t> &payload);
    Packet(uint8_t type, const string &payloadText);

    uint8_t getType() const;
    const vector<uint8_t> &getPayload() const;
    string getPayloadAsString() const;

    void setType(uint8_t newType);
    void setPayload(const vector<uint8_t> &newPayload);
    void setPayload(const string &newPayloadText);

    // Serialization: object -> byte array
    virtual vector<uint8_t> toBytes() const;

    // Deserialization: byte array -> object
    static Packet fromBytes(const vector<uint8_t> &rawBytes);

    virtual ~Packet() = default;
};

#endif