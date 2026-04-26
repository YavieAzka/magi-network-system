#ifndef PACKET_HPP
#define PACKET_HPP

#include <vector>
#include <cstdint>
#include <string>
#include <memory>
#include <stdexcept>
using namespace std;


/* 
Helper functions untuk byte manipulation
Jaringan pakai Big_Endian, CPU pakai Little-Endian
*/

namespace ByteUtils{
    // Menulis uint16_t ke buffer dalam Big-Endian order
    inline void write_u16_be(vector<uint8_t>& buf, uint16_t val){
        buf.push_back((val >> 8) & 0xFF);
        buf.push_back(val & 0xFF);
    }

    // Membaca uint16_t dari buffer dalam Big-endian Order
    inline uint16_t read_u16_be(const vector<uint8_t>& buf, size_t offset){
        if (offset + 1 >= buf.size()) throw out_of_range("Buffer too smal for u16");
        return (static_cast<uint16_t>(buf[offset]) << 8 | buf[offset+1]);
    }


    // Menulis uint32_t ke buffer dalam Big-Endian order
    inline void write_u32_be(vector<uint8_t>& buf, uint32_t val){
        buf.push_back((val >> 24) & 0xFF);
        buf.push_back((val >> 16) & 0xFF);
        buf.push_back((val>>8) & 0xFF);
        buf.push_back(val & 0xFF);
    }

    // Membaca uint32_t dari buffer dalam Big-endian Order
    inline uint32_t read_u32_be(const vector<uint8_t>& buf, size_t offset) {
        if (offset + 3 >= buf.size()) throw std::out_of_range("Buffer too small for u32");
        return (static_cast<uint32_t>(buf[offset]) << 24)
             | (static_cast<uint32_t>(buf[offset + 1]) << 16)
             | (static_cast<uint32_t>(buf[offset + 2]) << 8)
             | static_cast<uint32_t>(buf[offset + 3]);
    }

    inline void append_bytes(vector<uint8_t>& dst, const vector<uint8_t>& src) {
        dst.insert(dst.end(), src.begin(), src.end());
    }
}

class Packet
{
public:
    vector<uint8_t> payload;
    Packet() = default;
    explicit Packet(vector<uint8_t> data);

    virtual ~Packet() = default;
    virtual vector<uint8_t> toBytes() const = 0;
    virtual void deserialize(const vector<uint8_t>& raw) = 0;

    virtual string get_type() const = 0;

    void set_payload(const vector<uint8_t>& data);
    void set_payload_from_packet(const Packet& inner_packet);

    const vector<uint8_t>& get_payload_bytes() const;
    virtual size_t total_size() const;

};

/* Buat testing aja */
class RawPacket : public Packet {
public:
    RawPacket() = default;
    explicit RawPacket(vector<uint8_t> data);
    explicit RawPacket(const string& text);

    vector<uint8_t> toBytes() const override;
    void deserialize(const vector<uint8_t>& raw) override;
    string get_type() const override;
    string as_string() const;
};

#endif