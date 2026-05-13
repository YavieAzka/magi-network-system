#ifndef ARP_HPP
#define ARP_HPP

#include "core/Packet.hpp"
#include <string>
#include <vector>
#include <map>
#include <cstdint>

constexpr uint16_t ARP_OP_REQUEST = 1;
constexpr uint16_t ARP_OP_REPLY = 2;

class ARPPacket : public Packet
{
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
    std::vector<uint8_t> sender_hw_addr;
    std::string sender_prot_addr;
    std::vector<uint8_t> target_hw_addr;
    std::string target_prot_addr;

public:
    ARPPacket();

    uint16_t getOperation() const { return operation; }
    const std::vector<uint8_t> &getSenderHwAddr() const { return sender_hw_addr; }
    const std::string &getSenderProtAddr() const { return sender_prot_addr; }
    const std::vector<uint8_t> &getTargetHwAddr() const { return target_hw_addr; }
    const std::string &getTargetProtAddr() const { return target_prot_addr; }

    void setOperation(uint16_t op) { operation = op; }
    void setSenderHwAddr(const std::vector<uint8_t> &addr) { sender_hw_addr = addr; }
    void setSenderProtAddr(const std::string &ip) { sender_prot_addr = ip; }
    void setTargetHwAddr(const std::vector<uint8_t> &addr) { target_hw_addr = addr; }
    void setTargetProtAddr(const std::string &ip) { target_prot_addr = ip; }

    std::vector<uint8_t> toBytes() const override;
    void deserialize(const std::vector<uint8_t> &raw) override;
    std::string getType() const override { return "ARP"; }

    static ARPPacket createRequest(const std::vector<uint8_t> &senderMac,
                                   const std::string &senderIp,
                                   const std::string &targetIp);
    static ARPPacket createReply(const std::vector<uint8_t> &senderMac,
                                 const std::string &senderIp,
                                 const std::vector<uint8_t> &targetMac,
                                 const std::string &targetIp);
};

class ARPCache
{
private:
    std::map<std::string, std::string> arpcache;
    std::map<std::string, std::vector<std::vector<uint8_t>>> ipPacketQueue;

public:
    bool hasIp(const std::string &ip) const;
    void addEntry(const std::string &ip, const std::string &mac);
    void clearCache();
    std::string getMac(const std::string &ip) const;

    void enqueuePacket(const std::string &ip, const std::vector<uint8_t> &packet);
    void clearQueue();
    std::vector<std::vector<uint8_t>> dequeueAll(const std::string &ip);
    const std::map<std::string, std::string> &getTable() const;
};

#endif