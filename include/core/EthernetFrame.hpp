#ifndef ETHERNET_FRAME_HPP
#define ETHERNET_FRAME_HPP

#include "../../include/core/Packet.hpp"
#include <vector>
#include <cstdint>
#include <string>

class EthernetFrame : public Packet {
public:
    std::vector<uint8_t> dest_mac;
    std::vector<uint8_t> src_mac;
    uint16_t ethertype;
    uint32_t fcs;

    EthernetFrame();
    EthernetFrame(const std::vector<uint8_t>& d_mac, const std::vector<uint8_t>& s_mac, 
                  uint16_t type, const std::vector<uint8_t>& payload_data);

    std::vector<uint8_t> toBytes() const override;
    void deserialize(const std::vector<uint8_t>& raw) override;
    std::string get_type() const override;

    void calculate_and_set_fcs();
    bool verify_fcs() const;

private:
    uint32_t compute_crc32(const std::vector<uint8_t>& data) const;
};

#endif