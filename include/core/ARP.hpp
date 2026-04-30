#pragma once
#include "Packet.hpp"
#include <iostream>
#include <cstdint>
#include <array>
#include <vector>
#include <map>

class ARPPacket : public Packet{
    /*
    Ngambil dari geeksforgeeks males buka linknya lagi
    Hardware type: This is 16 bits field defining the type of the network on which ARP is running. Ethernet is given type 1. 
    Protocol type: This is 16 bits field defining the protocol. The value of this field for the IPv4 protocol is 0800H.
    Hardware length: This is an 8 bits field defining the length of the physical address in bytes. Ethernet is the value 6.
    Protocol length: This is an 8 bits field defining the length of the logical address in bytes. For the IPv4 protocol, the value is 4.
    Operation (request or reply): This is a 16 bits field defining the type of packet. Packet types are ARP request (1), and ARP reply (2).
    Sender hardware address: This is a variable length field defining the physical address of the sender. For example, for Ethernet, this field is 6 bytes long.
    Sender protocol address: This is also a variable length field defining the logical address of the sender For the IP protocol, this field is 4 bytes long.
    Target hardware address: This is a variable length field defining the physical address of the target. For Ethernet, this field is 6 bytes long. For the ARP request messages, this field is all Os because the sender does not know the physical address of the target.
    Target protocol address: This is also a variable length field defining the logical address of the target. For the IPv4 protocol, this field is 4 bytes long.
    */
    private:
    uint16_t hardware_type;
    uint16_t protocol_type;
    uint8_t hardware_len;
    uint8_t protocol_len;
    uint16_t operation;
    std::array<uint8_t, 6> sender_hw_addr;
    uint32_t sender_prot_addr;
    std::array<uint8_t, 6> target_hw_addr;
    uint32_t target_prot_addr;


    public:
    // operation default value is 0, dont forget to change it yah,  
    // depends on handleRequest sama handleRepla
    ARPPacket()
    : hardware_type(1),        
      protocol_type(0x0800),  
      hardware_len(6),
      protocol_len(4),
      operation(0),
      sender_hw_addr{0},
      sender_prot_addr(0),
      target_hw_addr{0},
      target_prot_addr(0) {}
    ~ARPPacket(){}

    uint16_t getHardwareType() const { return hardware_type; }
    uint16_t getProtocolType() const { return protocol_type; }
    uint8_t getHardwareLen() const { return hardware_len; }
    uint8_t getProtocolLen() const { return protocol_len; }
    uint16_t getOperation() const { return operation; }
    const std::array<uint8_t, 6>& getSenderHwAddr() const { return sender_hw_addr; }
    uint32_t getSenderProtAddr() const { return sender_prot_addr; }
    const std::array<uint8_t, 6>& getTargetHwAddr() const { return target_hw_addr; }
    uint32_t getTargetProtAddr() const { return target_prot_addr; }

    void setHardwareType(uint16_t value) { hardware_type = value; }
    void setProtocolType(uint16_t value) { protocol_type = value; }
    void setHardwareLen(uint8_t value) { hardware_len = value; }
    void setProtocolLen(uint8_t value) { protocol_len = value; }
    void setOperation(uint16_t value) { operation = value; }
    void setSenderHwAddr(const std::array<uint8_t, 6>& addr) { sender_hw_addr = addr; }
    void setSenderProtAddr(uint32_t value) { sender_prot_addr = value; }
    void setTargetHwAddr(const std::array<uint8_t, 6>& addr) { target_hw_addr = addr; }
    void setTargetProtAddr(uint32_t value) { target_prot_addr = value; }

    vector<uint8_t> toBytes() const override;
    bool parseToARP(vector<uint8_t>& data);

    void buildRequest();
    void buildReply();

    void handleReceivedARP();
};


class ARPCache {
    // IP -> MAC
    private:
    std::map<std::string, std::string> arpcache;

    // IP -> vector of packet
    std::map<std::string, std::vector<std::vector<uint8_t>>> ipPacketQueue;

    public:
    bool checkIpInCache(std::string ip) const;
    void addIpToCache(std::string ip, std::string mac);
    void clearCache();
    std::string getMac(std::string ip);

    void addPacketToCache(std::string ip, std::vector<uint8_t> packet);
    void clearQueue();

    std::vector<std::vector<uint8_t>> getAndClearQueue(std::string ip);
};