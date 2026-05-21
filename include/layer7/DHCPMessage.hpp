#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <map>

constexpr uint8_t DHCP_OP_REQUEST = 1; // client -> server
constexpr uint8_t DHCP_OP_REPLY   = 2; // server -> client

// Nilai field 'htype', tipe hardware
constexpr uint8_t DHCP_HTYPE_ETHERNET = 1;

// Panjang MAC address dalam byte
constexpr uint8_t DHCP_HLEN_ETHERNET = 6;

// Magic cookie wajib ada di awal field options (RFC 2131)
// Nilainya selalu 99.130.83.99 dalam bentuk 4 byte
constexpr uint32_t DHCP_MAGIC_COOKIE = 0x63825363;

// Port standar DHCP
constexpr uint16_t DHCP_SERVER_PORT = 67;
constexpr uint16_t DHCP_CLIENT_PORT = 68;

// Kode option DHCP yang dipakai (RFC 2132)
constexpr uint8_t DHCP_OPT_MESSAGE_TYPE  = 53; // tipe pesan (Discover/Offer/dll)
constexpr uint8_t DHCP_OPT_SUBNET_MASK   = 1;  // subnet mask
constexpr uint8_t DHCP_OPT_ROUTER        = 3;  // default gateway
constexpr uint8_t DHCP_OPT_LEASE_TIME    = 51; // durasi lease dalam detik
constexpr uint8_t DHCP_OPT_SERVER_ID     = 54; // IP DHCP server
constexpr uint8_t DHCP_OPT_END          = 255; // penanda akhir options

constexpr uint8_t DHCP_MSG_DISCOVER = 1;
constexpr uint8_t DHCP_MSG_OFFER    = 2;
constexpr uint8_t DHCP_MSG_REQUEST  = 3;
constexpr uint8_t DHCP_MSG_ACK      = 5;


struct DHCPMessage {

    /*
    DHCP packet format (source: GeeksForGeeks)
    Hardware Length (8 bits): Length of MAC address (e.g., 6 for Ethernet).
    Hop Count : Maximum number of hops the packet can travel.
    Transaction ID (32 bits): Set by client, used to match requests and replies.
    Number of Seconds (16 bits): Time elapsed since the client started booting.
    Flags (16 bits): Leftmost bit indicates broadcast reply requirement.
    Client IP Address (4 bytes): Filled if the client already has an IP, else 0.
    Your IP Address (4 bytes): Client IP assigned by the server.
    Server IP Address (4 bytes): IP address of the responding DHCP server.
    Gateway IP Address (4 bytes): Stores the IP of the DHCP relay agent, not the client’s default gateway.
    Client Hardware Address: The device’s MAC address.
    Server Name (64 bytes): Optional server hostname.
    Boot Filename (128 bytes): Pathname of boot file (for diskless clients).
    Options (variable): Vendor-specific or additional configuration.
    */

    uint8_t  op;       
    uint8_t  htype;    
    uint8_t  hlen;     
    uint8_t  hops;     
    uint32_t xid; 
    uint16_t secs;     
    uint16_t flags;    

    // Keempat field IP di bawah masing-masing 4 byte, disimpan sebagai string
    // agar mudah dibaca dan dicetak ke log
    std::string ciaddr;
    std::string yiaddr;
    std::string siaddr;
    std::string giaddr;

    // 16 byte — hardware address client (MAC address, sisa 10 byte dipadding 0)
    std::string chaddr; 

    // sname (64 byte) dan file (128 byte) diabaikan, tidak dipakai
    // di simulator ini, akan diisi 0 saat serialisasi

    // opitons disimpan sebagai map: option_code -> list of value bytes
    // Contoh: options[53] = {1}  berarti DHCP Discover
    //         options[1]  = {255,255,255,0}  berarti subnet /24
    std::map<uint8_t, std::vector<uint8_t>> options;

    // Menulis option tipe pesan (option 53)
    void setMessageType(uint8_t msgType);

    // Membaca option tipe pesan, return 0 jika tidak ada
    uint8_t getMessageType() const;

    // Menulis option subnet mask (option 1) dari string CIDR prefix
    // Contoh: prefixLen=24 → options[1] = {255,255,255,0}
    void setSubnetMask(uint8_t prefixLen);

    // Menulis option router/gateway (option 3) dari string IP
    void setRouter(const std::string& gatewayIp);

    // Menulis option lease time (option 51) dalam detik
    void setLeaseTime(uint32_t seconds);

    // Menulis option server identifier (option 54) dari string IP
    void setServerId(const std::string& serverIp);

    // Membaca option server identifier, return "" jika tidak ada
    std::string getServerId() const;

    // Membaca option router, return "" jika tidak ada
    std::string getRouter() const;

    // Membaca option subnet mask, return "" jika tidak ada
    std::string getSubnetMask() const;

    // Mengubah DHCPMessage menjadi byte array untuk dikirim lewat UDP
    // Layout: op(1) htype(1) hlen(1) hops(1) xid(4) secs(2) flags(2)
    //         ciaddr(4) yiaddr(4) siaddr(4) giaddr(4) chaddr(16)
    //         sname(64) file(128) magic_cookie(4) options(var) end(1)
    std::vector<uint8_t> toBytes() const;

    // Mengurai byte array menjadi DHCPMessage object
    // Melempar std::invalid_argument jika format tidak valid
    static DHCPMessage deserialize(const std::vector<uint8_t>& raw);

private:
    static std::vector<uint8_t> ipToBytes(const std::string& ip);
    static std::string bytesToIp(const uint8_t* ptr);
    static std::vector<uint8_t> macToBytes(const std::string& mac);
    static std::string bytesToMac(const uint8_t* ptr);
};