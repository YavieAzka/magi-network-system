#ifndef PACKET_HPP
#define PACKET_HPP

#include <cstdint>
#include <vector>
using namespace std;

class Packet
{
public:
    virtual ~Packet() = default;
    virtual vector<uint8_t> toBytes() const = 0;
};

#endif