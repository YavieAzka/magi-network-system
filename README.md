# Milestone 4 WBS — Magi System
Socket API & Application Layer (L7) · 5 orang · C++

---

## P1 — MagiSocket API

| Task | Deliverables |
|------|-------------|
| `MagiSocket` class | `include/layer7/MagiSocket.hpp` — field: `host_node`, `family` (AF_INET=2), `sock_type` (SOCK_STREAM=1, SOCK_DGRAM=2), internal transport pointer |
| `MagiSocket::bind()` | Ikat socket ke IP & port tertentu pada node |
| `MagiSocket::listen()` | Set socket ke mode passive open (wrap `TCPSocket::passiveOpen()`) |
| `MagiSocket::accept()` | Tunggu koneksi masuk, return socket baru yang sudah ESTABLISHED |
| `MagiSocket::connect()` | Inisiasi active open ke IP:port tujuan (wrap `TCPSocket::activeOpen()`) |
| `MagiSocket::send()` | Kirim bytes lewat transport layer (TCP atau UDP) |
| `MagiSocket::recv()` | Baca bytes dari receive buffer transport layer |
| `MagiSocket::close()` | Trigger 4-way teardown untuk TCP, cleanup resource |
| Build system | Tambah `add_library(layer7 ...)` di `CMakeLists.txt`, link ke `core layer4` |
| Unit testing | Test bind/connect/send/recv/close dengan dua host dalam satu subnet |

---

## P2 — DHCP + Host UDP Routing Patch

| Task | Deliverables |
|------|-------------|
| `DHCPMessage` struct | `include/layer7/DHCPMessage.hpp` — field: op, htype, hlen, xid, ciaddr, yiaddr, siaddr, chaddr, options |
| `DHCPMessage::toBytes()` | Serialize ke byte array sesuai format BOOTP/DHCP |
| `DHCPMessage::deserialize()` | Parse raw bytes jadi `DHCPMessage` object |
| `DHCPServer` class | `src/layer7/DHCPServer.cpp` — IP pool (range), lease map `mac→ip`, exclude gateway & broadcast |
| DORA flow — server side | Terima Discover (broadcast) → kirim Offer (unicast) → terima Request → kirim ACK |
| `DHCPClient` class | `src/layer7/DHCPClient.cpp` — kirim Discover, terima Offer, kirim Request, terima ACK |
| Host update setelah ACK | Panggil `Host::setIpAddress()` dan `Host::setDefaultGateway()` dengan nilai dari ACK |
| CLI integration | Dispatch `H1 dhcp_discover` di `cli.cpp` ke `DHCPClient` |
| **Host UDP routing patch** | Tambah `udpSockets_` map di `Host` (pola sama dengan `tcpSockets_` yang sudah ada) |
| **`handleUDPSegment()` patch** | Isi implementasi dispatch ke socket yang listening di `dstPort` yang sesuai — **saat ini masih stub di M3** |
| Unit testing | Test DORA flow antara satu DHCPServer dan satu DHCPClient, test UDP routing patch |

---

## P3 — DNS

| Task | Deliverables |
|------|-------------|
| `DNSMessage` struct | `include/layer7/DNSMessage.hpp` — field: id, flags, QNAME (string), RDATA (IP string). Cukup support A record |
| `DNSMessage::toBytes()` | Serialize ke byte array format DNS minimal |
| `DNSMessage::deserialize()` | Parse raw bytes jadi `DNSMessage` object |
| `DNSServer` class | `src/layer7/DNSServer.cpp` — record store `unordered_map<string,string>` (domain→IP), listen UDP port 53 |
| `DNSClient` class | `src/layer7/DNSClient.cpp` — expose fungsi `resolve(hostIp, domain) → string` yang bisa dipanggil HTTPClient |
| Record loading | Opsional: load record dari field `"dns_records"` di topology JSON |
| Integrasi ke HTTP flow | `DNSClient::resolve()` dipanggil di HTTPClient P5 saat URL berupa domain bukan IP |
| Unit testing | Test resolve "www.magi.com" → "10.0.0.5", test domain tidak ditemukan |

---

## P4 — HTTP Server

| Task | Deliverables |
|------|-------------|
| `HTTPServer` class | `include/layer7/HTTPServer.hpp` — field: `server_socket` (MagiSocket), `web_root` (path file), `running` flag |
| HTTP request parser | Parse request line `GET /path HTTP/1.1` dan header `Host:` dari raw bytes |
| HTTP response builder | Bangun string `HTTP/1.1 200 OK\r\nContent-Length: N\r\n\r\n{body}` sebagai fungsi murni |
| File serving | Baca file dari disk jika ada, fallback ke hardcoded HTML jika tidak ada |
| `HTTPServer::start(file)` | Bind port 80, listen, loop terima koneksi → parse → serve → close |
| `HTTPServer::stop()` | Set `running=false`, close server socket |
| CLI integration | Dispatch `H1 http_server start index.html` dan `H1 http_server stop` di `cli.cpp` |
| Unit testing | Test start/stop, test request GET valid, test file tidak ditemukan |

---

## P5 — HTTP Client, CLI & End-to-End Testing

| Task | Deliverables |
|------|-------------|
| `HTTPClient` class | `include/layer7/HTTPClient.hpp` — fungsi `get(host, url) → string` |
| URL parser | Parse `www.magi.com` atau `10.0.0.5` — deteksi apakah domain atau IP langsung |
| DNS integration | Jika domain, panggil `DNSClient::resolve()` dari P3 sebelum connect |
| HTTP GET flow | `connect(ip, 80)` → kirim `GET / HTTP/1.1\r\nHost: ...\r\n\r\n` → terima response → print body |
| CLI integration | Tambah handler `H1 http_get <url>` di `cli.cpp` |
| Unit testing | Test `http_get` ke IP langsung, test `http_get` ke domain lewat DNS |
| **End-to-end testing** | Skenario lengkap: load topology → `dhcp_discover` → `http_server start` → `http_get www.nerv.com` lintas router, pastikan log tiap layer tercetak |
| **README update** | Dokumentasi cara run dan test semua perintah M4 |

---

## Urutan pengerjaan

### Fase 1 — Paralel (mulai sekarang)

| Orang | Yang dikerjakan |
|-------|----------------|
| **P1** | Implement MagiSocket penuh — ini blocker, prioritas tertinggi |
| **P2** | Desain byte layout DHCPMessage, tulis serialize/deserialize, siapkan IP pool logic, **kerjakan patch `udpSockets_` & `handleUDPSegment()` di Host** |
| **P3** | Desain DNSMessage format, isi record table statis, tulis serialize/deserialize |
| **P4** | Tulis HTTP request parser dan response builder sebagai fungsi murni, unit test tanpa socket |
| **P5** | Tambah CLI handler stub untuk `http_get`, `http_server`, tulis URL parser |

> P2 **wajib** menyelesaikan patch `handleUDPSegment()` di fase ini agar P3 bisa langsung test di fase 2.

### Fase 2 — Integrasi protokol (setelah P1 merge)

| Orang | Yang dikerjakan |
|-------|----------------|
| **P1** | Fix bug MagiSocket yang muncul saat dipakai P2/P3/P4, review PR |
| **P2** | Wiring DHCP ke MagiSocket UDP, test DORA flow, pastikan Host update IP setelah ACK |
| **P3** | Wiring DNS ke MagiSocket UDP, test resolve domain lewat simulator — **bisa jalan setelah P2 selesai patch** |
| **P4** | Wiring HTTPServer ke MagiSocket TCP, test `http_server start/stop` |
| **P5** | Wiring HTTPClient ke socket + DNS P3, test `http_get` ke IP langsung dulu, lalu ke domain |

### Fase 3 — End-to-end & demo prep (semua merge)

Semua merge, **P5 memimpin** end-to-end test: jalankan skenario demo lengkap lintas subnet, pastikan log tiap layer tercetak, fix bug bersama, P5 update README.

---

## Dependency pengerjaan

```
P1 (MagiSocket)
    ↓
    ├── P2 (DHCP + patch udpSockets_) ──┐
    │         ↓                         │
    │    P3 bisa test                   │
    │         │                         │
    ├── P3 (DNS) ───────────────────────┤
    │                                   │
    └── P4 (HTTP Server) ───────────────┤
                                        │
                                   P5 (HTTP Client & CLI & e2e testing)
```

| Orang | Bisa mulai | Blokir sampai |
|-------|-----------|---------------|
| P1 | Sekarang | — |
| P2 | Sekarang (logic + patch Host), test setelah P1 | P1 merge |
| P3 | Sekarang (logic), test setelah P2 selesai patch | P1 merge + P2 selesai `udpSockets_` |
| P4 | Sekarang (logic), test setelah P1 | P1 merge |
| P5 | Sekarang (CLI stub), full test setelah semua merge | P1 + P3 + P4 merge |
