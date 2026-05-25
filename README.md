# Magi System — OSI Layer Network Simulator

Simulator jaringan berbasis OSI Layer yang dibangun dari nol menggunakan C++, tanpa menggunakan library jaringan bawaan sistem operasi. Proyek ini mensimulasikan komunikasi data dari Layer 2 hingga Layer 7, mencakup Ethernet, ARP, IPv4, ICMP, TCP, UDP, DHCP, DNS, dan HTTP.

Dikerjakan sebagai Tugas Besar IF2230 Jaringan Komputer.

---

## Prerequisites

- **Compiler**: GCC / G++ dengan dukungan C++17
- **Build tool**: CMake 3.14 atau lebih baru
- **OS**: Linux / WSL (Windows Subsystem for Linux)
- **Library eksternal**: `nlohmann/json` (sudah disertakan via `util/JsonParser`)

Pastikan tools berikut tersedia:
```bash
g++ --version   # minimal GCC 9
cmake --version # minimal 3.14
make --version
```

---

## Cara Setup dan Run

### 1. Clone repository

```bash
git clone <url-repo>
cd tugas-besar-jarkom-tamat_sudah
```

### 2. Build

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
```

### 3. Jalankan simulator

```bash
./jarkom
```

Program akan masuk ke mode interactive prompt:
```
Tubes apaan bikin network simulator
Ketik 'help' untuk bantuan.

tamat-sudah>
```

---

## Cara Test Semua Perintah

### Setup topologi dasar

```
create host H1
create host H2
create switch SW1
link H1 SW1:1 10
link H2 SW1:2 10
H1 set-ip 192.168.1.10/24
H1 set-gw 192.168.1.1
H2 set-ip 192.168.1.20/24
H2 set-gw 192.168.1.1
```

### Milestone 1 — Ping dalam subnet (L2)

```
H1 ping 192.168.1.20
H1 arp
SW1 mac
```

### Milestone 2 — Routing lintas subnet (L3)

```
create host H3
create router R1
link SW1:3 R1:1 5
link H3 R1:2 10
R1 set-ip 1 192.168.1.1/24
R1 set-ip 2 10.0.0.1/24
H1 set-gw 192.168.1.1
H3 set-ip 10.0.0.10/24
H3 set-gw 10.0.0.1
H1 ping 10.0.0.10
H1 traceroute 10.0.0.10
R1 route
```

### Milestone 3 — TCP Handshake (L4)

```
H2 tcp_listen 8080
H1 tcp_connect 192.168.1.20 8080
H1 tcp_send 12345 "hello"
H1 tcp_close 12345
```

### Milestone 4 — DHCP (L7)

```
H2 dhcp_server start 192.168.1.100 192.168.1.200 192.168.1.1 24
H1 dhcp_discover
```

### Milestone 4 — HTTP (L7)

Siapkan file HTML terlebih dahulu:
```bash
mkdir -p build/www
cat > build/www/index.html << 'EOF'
<html><body><h1>Hello from Magi System</h1></body></html>
EOF
```

Lalu di simulator:
```
H2 http_server 80 www
H1 http_get 192.168.1.20 80 /
```

### End-to-end — HTTP lintas router

```
H3 http_server 80 www
H1 http_get 10.0.0.10 80 /
```

### Manajemen topologi

```
topology
save topology.json
load topology.json
```

---

## Daftar Perintah CLI

| Perintah | Format | Keterangan |
|----------|--------|------------|
| create | `create <host\|switch\|router> <nama>` | Buat node baru |
| link | `link <ep1> <ep2> [delay_ms]` | Sambungkan dua node |
| unlink | `unlink <ep1> <ep2>` | Putuskan kabel |
| topology | `topology` | Tampilkan topologi aktif |
| save | `save [file.json]` | Ekspor topologi ke JSON |
| load | `load <file.json>` | Muat topologi dari JSON |
| ping | `<host> ping <IP>` | Kirim ICMP Echo Request |
| traceroute | `<host> traceroute <IP>` | Lacak rute hop-by-hop |
| arp | `<host\|router> arp` | Tampilkan ARP cache |
| route | `<router> route` | Tampilkan routing table |
| mac | `<switch> mac` | Tampilkan MAC table |
| set-ip | `<host> set-ip <IP/CIDR>` | Set IP host |
| set-gw | `<host> set-gw <IP>` | Set default gateway |
| tcp_listen | `<host> tcp_listen <port>` | Buka port TCP |
| tcp_connect | `<host> tcp_connect <IP> <port>` | Inisiasi TCP handshake |
| tcp_send | `<host> tcp_send <port> <msg>` | Kirim data TCP |
| tcp_close | `<host> tcp_close <port>` | Tutup koneksi TCP |
| dhcp_server | `<host> dhcp_server start [poolStart] [poolEnd] [gw] [prefix]` | Jalankan DHCP server |
| dhcp_discover | `<host> dhcp_discover` | Minta IP otomatis via DHCP |
| http_server | `<host> http_server <port> [webroot]` | Jalankan HTTP server |
| http_get | `<host> http_get <IP> <port> <path>` | Kirim HTTP GET request |

---

## Struktur Direktori

```
tugas-besar-jarkom-tamat_sudah/
├── CMakeLists.txt
├── Makefile
├── README.md
├── topology.json
├── include/
│   ├── core/          # Node, Interface, Link, Packet (M0)
│   ├── layer2/        # EthernetFrame, ARP, Switch (M1)
│   ├── layer3/        # IPv4Packet, ICMPMessage (M2)
│   ├── layer4/        # TCPSocket, TCPSegment, UDPSegment (M3)
│   ├── layer7/        # MagiSocket, DHCP, DNS, HTTP (M4)
│   └── util/
├── src/
│   ├── core/
│   ├── layer2/
│   ├── layer3/
│   ├── layer4/
│   ├── layer7/
│   ├── util/
│   ├── cli.cpp
│   └── main.cpp
└── tests/
```

---

## Pembagian Tugas

| NIM | Nama | Jobdesc |
|-----|------|---------|
| 13524027 | Aufa Rienaldifaza Ahmad | — |
| 13524041 | Nathan Adhika Santosa | — |
| 13524049 | Arina Azka | M0: Packet; M1: Ethernet & VLAN; M2: Host; M3: UDP; M4: DNS |
| 13524079 | Angelina Andra Alanna | M0: Link, Interface; M4 P5: HTTPClient, URL parser, CLI integration (http_get, http_server, dhcp_discover, dhcp_server), end-to-end testing, README, Bonus: Topology Visualizer |
| 13524077 | Yavie Azka Putra Araly | M0: CLI, Integration; M1: ARP; M2: ICMP; M3: TCP state machine & 3-way handshake; M4: DHCP; Bonus: RIP |

---

## Bahasa Pemrograman

C++ 