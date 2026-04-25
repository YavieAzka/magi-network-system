#ifndef PACKET_HPP
#define PACKET_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <cstddef>
using namespace std;

class Packet
{
private:
    // Format byte: [ Length ][ Payload ][ Checksum ]
    vector<uint8_t> payload;

    static uint8_t calculateChecksum(const vector<uint8_t> &bytes);

public:
    Packet();
    explicit Packet(const vector<uint8_t> &payload);
    explicit Packet(const string &payloadText);

    const vector<uint8_t> &getPayload() const;
    string getPayloadAsString() const;

    void setPayload(const vector<uint8_t> &newPayload);
    void setPayload(const string &newPayloadText);

    // Serialization: object -> byte array
    virtual vector<uint8_t> toBytes() const;

    // Deserialization: byte array -> object
    static Packet fromBytes(const vector<uint8_t> &rawBytes);

    virtual ~Packet() = default;
};

#endif