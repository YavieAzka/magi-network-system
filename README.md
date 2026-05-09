# Milestone 2 WBS

## 5 people WBS division

P1
| task | deliverables |
| ---- | ------------ |
| IPv4Packet struct | hpp file: version, IHL, TOS, totalLen, TTL, protocol, checksum, srcIP, dstIP, payload |
| IPv4Packet::toBytes() | serialize header + payload ke byte array, big-endian |
| IPv4Packet::deserialize() | parse raw bytes jadi IPv4Packet object |
| calculateChecksum() | 16-bit one's complement sum, checksum field di-0 dulu saat hitung |
| verifyChecksum() | return bool, drop paket kalau false |
| decrementTTL() | TTL -= 1, lalu recalculate checksum otomatis |

P2
| task | deliverables |
| ---- | ------------ |
| ICMPMessage struct | hpp file: type, code, checksum, payload |
| ICMPMessage::toBytes() | serialize ke byte array |
| ICMPMessage::deserialize() | parse raw bytes jadi ICMPMessage object |
| buildEchoRequest() | type=8, code=0, isi identifier + sequence number |
| buildEchoReply() | type=0, code=0, balik payload dari request |
| buildTimeExceeded() | type=11, code=0, dikirim Router saat TTL=0 |
| buildUnreachable() | type=3, code=0, dikirim Router saat no route |
| ICMP checksum | cover header + data, berbeda dari IPv4 checksum |

P3
| task | deliverables |
| ---- | ------------ |
| Router class | extension of Node |
| RoutingTableEntry struct | destNetwork (CIDR string), prefixLen (int), nextHop (string), outInterface (int) |
| longestPrefixMatch() | loop routing table, return entry dengan prefix terpanjang yang match destIP |
| routePacket() | decrement TTL, verify checksum, recalculate checksum, forward ke interface |
| handleReceive() | decapsulate EthernetFrame -> IPv4Packet, cek checksum, route atau kirim ICMP error |
| kirim ICMP Unreachable | dipanggil handleReceive() saat longestPrefixMatch() return null |
| kirim ICMP Time Exceeded | dipanggil routePacket() saat TTL == 0 setelah decrement |
| printRoutingTable() | untuk CLI R1 route |
| getType() | return "router" |

P4
| task | deliverables |
| ---- | ------------ |
| NetUtils | stringToIP(), ipToString(), ipToBytes(), cidrToMask(), applyMask() |
| update CMakeLists.txt | tambah layer3/ ke build system |
| Host::sendLayer3Packet() | bungkus payload ke IPv4Packet, tentukan nexthop, ARP jika perlu |
| Host::ping() | bangun ICMP Echo Request, kirim, catat timestamp, tunggu reply, hitung RTT |
| Host::traceroute() | loop TTL 1,2,3..., kirim ping dengan TTL terbatas, cetak IP dari tiap ICMP Time Exceeded yang balik |
| Host::handleReceive() update | tambah handling ethertype=0x0800 untuk terima IPv4, dispatch ke ping/traceroute handler |

P5
| task | deliverables |
| ---- | ------------ |
| buildFromConfig() untuk Router | baca interfaces + routing_table dari JSON, construct Router object |
| CLI H1 ping \<ip\> | dispatch ke Host::ping() |
| CLI H1 traceroute \<ip\> | dispatch ke Host::traceroute() |
| CLI R1 route | dispatch ke Router::printRoutingTable() |
| CLI H1 arp dan R1 arp | dispatch ke printARPCache() |
| unit testing IPv4 | test toBytes/deserialize, checksum valid/invalid, TTL decrement |
| unit testing Router | test longestPrefixMatch dengan berbagai prefix, test ICMP error |
| integrasi end-to-end | H1 ping H2 lintas Router, traceroute, ICMP Unreachable ke IP fiktif |

remember to write unit testing ~~~.