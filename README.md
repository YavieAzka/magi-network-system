
# Milestone 3 WBS
## 5 People WBS Division

### P1 — TCP Segment Structure & Checksum
| Task | Deliverables |
|------|-------------|
| `TCPSegment` struct | hpp file: srcPort, dstPort, seqNum, ackNum, dataOffset, flags (bitmask SYN/ACK/FIN/PSH/RST), windowSize, checksum, payload |
| `TCPSegment::toBytes()` | serialize header + payload ke byte array, big-endian |
| `TCPSegment::deserialize()` | parse raw bytes jadi `TCPSegment` object |
| `buildPseudoHeader()` | bangun pseudo-header (srcIP, dstIP, protocol=6, TCP length) untuk keperluan checksum |
| `calculateChecksum()` | 16-bit one's complement sum atas pseudo-header + TCP segment, checksum field di-0 dulu saat hitung |
| `verifyChecksum()` | return bool, drop segment kalau false |
| unit testing TCP Segment | test toBytes/deserialize, checksum valid/invalid, flag bitmask |

---

### P2 — UDP Segment Structure & Checksum
| Task | Deliverables |
|------|-------------|
| `UDPSegment` struct | hpp file: srcPort, dstPort, length, checksum, payload |
| `UDPSegment::toBytes()` | serialize ke byte array, big-endian |
| `UDPSegment::deserialize()` | parse raw bytes jadi `UDPSegment` object |
| `buildPseudoHeader()` | bangun pseudo-header (srcIP, dstIP, protocol=17, UDP length) — reuse pattern dari P1 |
| `calculateChecksum()` | 16-bit one's complement sum atas pseudo-header + UDP segment |
| `verifyChecksum()` | return bool |
| unit testing UDP Segment | test toBytes/deserialize, checksum valid/invalid |

---

### P3 — TCP State Machine & 3-Way Handshake
| Task | Deliverables |
|------|-------------|
| `TCPState` enum | CLOSED, LISTEN, SYN_SENT, SYN_RCVD, ESTABLISHED, FIN_WAIT_1, FIN_WAIT_2, TIME_WAIT, CLOSE_WAIT, LAST_ACK |
| `TCPSocket` class | localIP, localPort, remoteIP, remotePort, seqNum, expectedAck, state, receiveBuffer |
| `activeOpen()` | kirim SYN, transisi ke SYN_SENT |
| `passiveOpen()` | transisi ke LISTEN |
| `handleIncomingSegment()` | dispatch berdasarkan kombinasi state + flags |
| 3-Way Handshake logic | SYN → SYN-ACK → ACK, state harus ESTABLISHED sebelum boleh kirim PSH |
| logging state transition | cetak ke terminal setiap kali state berubah |
| unit testing handshake | simulasikan exchange segment SYN/SYN-ACK/ACK, verifikasi state akhir ESTABLISHED |

---

### P4 — Receive Buffer & 4-Way Teardown
| Task | Deliverables |
|------|-------------|
| Receive Buffer | struktur buffer di `TCPSocket`, simpan segmen berdasarkan seqNum, rakit ulang jika out-of-order |
| `receiveData()` | masukkan segment PSH ke buffer, kirim ACK balik |
| `readBuffer()` | kembalikan data yang sudah lengkap dan berurutan dari buffer |
| 4-Way Teardown logic | FIN → ACK → FIN → ACK, transisi state FIN_WAIT_1/2, TIME_WAIT, CLOSE_WAIT, LAST_ACK |
| `activeClose()` | kirim FIN, mulai proses teardown |
| `passiveClose()` | terima FIN, balas ACK, kirim FIN balik |
| unit testing buffer | test penerimaan segment out-of-order, verifikasi rakitan data utuh |
| unit testing teardown | simulasikan 4-way teardown, verifikasi state akhir CLOSED |

---

### P5 — Integrasi Layer, CLI & End-to-End Testing
| Task | Deliverables |
|------|-------------|
| update `CMakeLists.txt` | tambah `layer4/` ke build system |
| `Host::sendTCP()` | bungkus data ke `TCPSegment`, kemas ke `IPv4Packet` (protocol=6), teruskan via L3 |
| `Host::sendUDP()` | bungkus data ke `UDPSegment`, kemas ke `IPv4Packet` (protocol=17), teruskan via L3 |
| `Host::handleReceive()` update | tambah dispatch protocol=6 ke TCP handler, protocol=17 ke UDP handler |
| `Host::tcpConnect()` | inisiasi `TCPSocket`, panggil `activeOpen()`, tunggu ESTABLISHED |
| CLI `H1 tcp_connect <ip> <port>` | dispatch ke `Host::tcpConnect()` |
| integrasi end-to-end | `tcp_connect` lintas Router, log state machine tercatat, buffer assembly berjalan |
| unit testing integrasi | full handshake + kirim data (PSH) + teardown dalam satu skenario |

---

> **Catatan koordinasi:** P3 dan P4 bekerja pada kelas `TCPSocket` yang sama — disarankan P3 menyelesaikan skeleton state machine dan interface `handleIncomingSegment()` terlebih dahulu sebelum P4 mulai mengisi logika buffer dan teardown. Gunakan Git branch terpisah dan merge setelah keduanya selesai. P5 baru bisa mulai integrasi penuh setelah P3 dan P4 siap.
