#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include "layer4/UDPSegment.hpp"

using namespace std;

static string bytesToHex(const vector<uint8_t> &bytes)
{
    ostringstream oss;
    for (size_t i = 0; i < bytes.size(); i++)
    {
        oss << hex << setw(2) << setfill('0') << uppercase
            << static_cast<int>(bytes[i]);
        if (i + 1 < bytes.size())
            oss << " ";
    }
    return oss.str();
}

void test_toBytes_big_endian()
{
    cout << "\n=== TEST 1: toBytes() Big-Endian ===\n";

    UDPSegment seg;
    seg.srcPort = 0x1234;
    seg.dstPort = 0x0035;
    seg.length = 0x000D;
    seg.checksum = 0x0000;
    seg.payload = {0x68, 0x65, 0x6C, 0x6C, 0x6F};

    cout << "Input:\n";
    cout << "  srcPort  = 0x1234 (4660)\n";
    cout << "  dstPort  = 0x0035 (53)\n";
    cout << "  length   = 0x000D (13)\n";
    cout << "  checksum = 0x0000\n";
    cout << "  payload  = {68 65 6C 6C 6F} (\"hello\")\n";

    vector<uint8_t> raw = seg.toBytes();

    cout << "Output:   " << bytesToHex(raw) << "\n";
    cout << "Expected: 12 34 00 35 00 0D 00 00 68 65 6C 6C 6F\n";

    assert(raw.size() == 13);
    assert(raw[0] == 0x12 && raw[1] == 0x34);
    assert(raw[2] == 0x00 && raw[3] == 0x35);
    assert(raw[4] == 0x00 && raw[5] == 0x0D);
    assert(raw[8] == 0x68 && raw[12] == 0x6F);
    cout << "[PASS] TEST 1\n";
}

void test_deserialize_roundtrip()
{
    cout << "\n=== TEST 2: deserialize() Round-Trip ===\n";

    vector<uint8_t> raw = {
        0x23, 0x28,      // srcPort = 9000
        0x00, 0x35,      // dstPort = 53
        0x00, 0x0C,      // length  = 12
        0xAB, 0xCD,      // checksum
        0x41, 0x42, 0x43 // "ABC"
    };

    cout << "Input (hex): " << bytesToHex(raw) << "\n";

    UDPSegment seg = UDPSegment::deserialize(raw);

    cout << "Output:\n";
    cout << "  srcPort  = " << seg.srcPort << "  (expected: 9000)\n";
    cout << "  dstPort  = " << seg.dstPort << "  (expected: 53)\n";
    cout << "  length   = " << seg.length << "  (expected: 12)\n";
    cout << "  checksum = 0x" << hex << setw(4) << setfill('0') << uppercase
         << seg.checksum << dec << "  (expected: 0xABCD)\n";
    cout << "  payload  = {41 42 43}  (expected: {41 42 43})\n";

    assert(seg.srcPort == 9000);
    assert(seg.dstPort == 53);
    assert(seg.length == 12);
    assert(seg.checksum == 0xABCD);
    assert(seg.payload == vector<uint8_t>({0x41, 0x42, 0x43}));
    cout << "[PASS] TEST 2\n";
}

void test_buildPseudoHeader()
{
    cout << "\n=== TEST 3: buildPseudoHeader() ===\n";

    string srcIP = "192.168.1.10";
    string dstIP = "192.168.1.20";
    uint16_t udpLen = 13;

    cout << "Input:\n";
    cout << "  srcIP  = " << srcIP << "  → C0 A8 01 0A\n";
    cout << "  dstIP  = " << dstIP << "  → C0 A8 01 14\n";
    cout << "  udpLen = " << udpLen << "\n";

    vector<uint8_t> pseudo = UDPSegment::buildPseudoHeader(srcIP, dstIP, udpLen);

    cout << "Output:   " << bytesToHex(pseudo) << "\n";
    cout << "Expected: C0 A8 01 0A C0 A8 01 14 00 11 00 0D\n";
    //                 |---srcIP---|  |---dstIP---|  |0 |17|len|

    assert(pseudo.size() == 12);
    assert(pseudo[0] == 0xC0 && pseudo[3] == 0x0A);   // 192.168.1.10
    assert(pseudo[4] == 0xC0 && pseudo[7] == 0x14);   // 192.168.1.20
    assert(pseudo[8] == 0x00);                        // zero
    assert(pseudo[9] == 0x11);                        // protocol=17
    assert(pseudo[10] == 0x00 && pseudo[11] == 0x0D); // length=13
    cout << "[PASS] TEST 3\n";
}

void test_checksum_valid()
{
    cout << "\n=== TEST 4: calculateChecksum() + verifyChecksum() Valid ===\n";

    string srcIP = "10.0.0.1";
    string dstIP = "10.0.0.2";

    UDPSegment seg;
    seg.srcPort = 5000;
    seg.dstPort = 5001;
    seg.payload = {'d', 'a', 't', 'a'};
    seg.length = 8 + (uint16_t)seg.payload.size();

    cout << "Input:\n";
    cout << "  srcIP   = " << srcIP << "\n";
    cout << "  dstIP   = " << dstIP << "\n";
    cout << "  srcPort = " << seg.srcPort << "\n";
    cout << "  dstPort = " << seg.dstPort << "\n";
    cout << "  payload = \"data\"\n";

    seg.checksum = UDPSegment::calculateChecksum(srcIP, dstIP, seg);

    cout << "Output:\n";
    cout << "  checksum        = 0x" << hex << setw(4) << setfill('0')
         << uppercase << seg.checksum << dec << "  (expected: != 0x0000)\n";
    cout << "  verifyChecksum  = " << (seg.verifyChecksum(srcIP, dstIP) ? "true" : "false")
         << "  (expected: true)\n";

    assert(seg.checksum != 0);
    assert(seg.verifyChecksum(srcIP, dstIP) == true);
    cout << "[PASS] TEST 4\n";
}

void test_checksum_corrupt_payload()
{
    cout << "\n=== TEST 5: verifyChecksum() — Payload Dikorupsi ===\n";

    string srcIP = "172.16.0.1";
    string dstIP = "172.16.0.2";

    UDPSegment seg;
    seg.srcPort = 8080;
    seg.dstPort = 9090;
    seg.payload = {'t', 'e', 's', 't', '!'};
    seg.length = 8 + (uint16_t)seg.payload.size();
    seg.checksum = UDPSegment::calculateChecksum(srcIP, dstIP, seg);

    cout << "Input:\n";
    cout << "  payload asli   = \"test!\"\n";
    cout << "  checksum asli  = 0x" << hex << setw(4) << setfill('0')
         << uppercase << seg.checksum << dec << "\n";

    seg.payload[2] ^= 0xFF; // korupsi 1 byte
    cout << "  payload korup  = payload[2] XOR 0xFF\n";

    bool result = seg.verifyChecksum(srcIP, dstIP);
    cout << "Output:   verifyChecksum = " << (result ? "true" : "false") << "\n";
    cout << "Expected: false\n";

    assert(result == false);
    cout << "[PASS] TEST 5\n";
}

void test_checksum_corrupt_header()
{
    cout << "\n=== TEST 6: verifyChecksum() — Header Dikorupsi ===\n";

    string srcIP = "192.168.0.1";
    string dstIP = "192.168.0.2";

    UDPSegment seg;
    seg.srcPort = 1234;
    seg.dstPort = 5678;
    seg.payload = {0xDE, 0xAD, 0xBE, 0xEF};
    seg.length = 8 + (uint16_t)seg.payload.size();
    seg.checksum = UDPSegment::calculateChecksum(srcIP, dstIP, seg);

    cout << "Input:\n";
    cout << "  srcPort asli  = " << seg.srcPort << "\n";
    seg.srcPort = 9999;
    cout << "  srcPort korup = " << seg.srcPort << "\n";

    bool result = seg.verifyChecksum(srcIP, dstIP);
    cout << "Output:   verifyChecksum = " << (result ? "true" : "false") << "\n";
    cout << "Expected: false\n";

    assert(result == false);
    cout << "[PASS] TEST 6\n";
}

void test_checksum_zero_skip()
{
    cout << "\n=== TEST 7: verifyChecksum() — Checksum 0x0000 (RFC 768 Skip) ===\n";

    UDPSegment seg;
    seg.srcPort = 1111;
    seg.dstPort = 2222;
    seg.length = 8;
    seg.checksum = 0x0000;

    cout << "Input:    checksum = 0x0000 (pengirim tidak menghitung)\n";

    bool result = seg.verifyChecksum("1.2.3.4", "5.6.7.8");
    cout << "Output:   verifyChecksum = " << (result ? "true" : "false") << "\n";
    cout << "Expected: true (skip per RFC 768)\n";

    assert(result == true);
    cout << "[PASS] TEST 7\n";
}

void test_deserialize_too_short()
{
    cout << "\n=== TEST 8: deserialize() — Input < 8 Bytes ===\n";

    vector<uint8_t> bad = {0x00, 0x35, 0x00};
    cout << "Input:    " << bytesToHex(bad) << " (" << bad.size() << " bytes, min=8)\n";

    bool threw = false;
    try
    {
        UDPSegment::deserialize(bad);
    }
    catch (const std::invalid_argument &e)
    {
        threw = true;
        cout << "Output:   exception = \"" << e.what() << "\"\n";
    }
    cout << "Expected: std::invalid_argument dilempar\n";

    assert(threw == true);
    cout << "[PASS] TEST 8\n";
}

void test_empty_payload()
{
    cout << "\n=== TEST 9: Segment Tanpa Payload (Header Only) ===\n";

    string srcIP = "10.10.10.1";
    string dstIP = "10.10.10.2";

    UDPSegment seg;
    seg.srcPort = 5353;
    seg.dstPort = 5353;
    seg.payload = {};
    seg.length = 8;
    seg.checksum = UDPSegment::calculateChecksum(srcIP, dstIP, seg);

    cout << "Input:  srcPort=5353, dstPort=5353, payload={} (kosong)\n";

    vector<uint8_t> raw = seg.toBytes();
    UDPSegment parsed = UDPSegment::deserialize(raw);
    bool valid = seg.verifyChecksum(srcIP, dstIP);

    cout << "Output:\n";
    cout << "  toBytes() size   = " << raw.size() << "  (expected: 8)\n";
    cout << "  parsed.srcPort   = " << parsed.srcPort << "  (expected: 5353)\n";
    cout << "  payload.size()   = " << parsed.payload.size() << "  (expected: 0)\n";
    cout << "  verifyChecksum   = " << (valid ? "true" : "false") << "  (expected: true)\n";

    assert(raw.size() == 8);
    assert(parsed.srcPort == 5353);
    assert(parsed.payload.empty());
    assert(valid == true);
    cout << "[PASS] TEST 9\n";
}

void test_full_pipeline()
{
    cout << "\n=== TEST 10: Full Pipeline (Kirim → Serialize → Deserialize → Verify) ===\n";

    string srcIP = "192.168.1.10";
    string dstIP = "192.168.1.20";

    UDPSegment sender;
    sender.srcPort = 12345;
    sender.dstPort = 53;
    sender.payload = {'M', 'A', 'G', 'I'};
    sender.length = 8 + (uint16_t)sender.payload.size();
    sender.checksum = UDPSegment::calculateChecksum(srcIP, dstIP, sender);

    vector<uint8_t> on_wire = sender.toBytes();

    cout << "Pengirim:\n";
    cout << "  srcPort  = " << sender.srcPort << "\n";
    cout << "  dstPort  = " << sender.dstPort << "\n";
    cout << "  payload  = \"MAGI\"\n";
    cout << "  checksum = 0x" << hex << setw(4) << setfill('0')
         << uppercase << sender.checksum << dec << "\n";
    cout << "  on-wire  = " << bytesToHex(on_wire) << "\n";

    UDPSegment receiver = UDPSegment::deserialize(on_wire);
    bool valid = receiver.verifyChecksum(srcIP, dstIP);
    string recv_str(receiver.payload.begin(), receiver.payload.end());

    cout << "Penerima:\n";
    cout << "  srcPort        = " << receiver.srcPort << "  (expected: 12345)\n";
    cout << "  dstPort        = " << receiver.dstPort << "  (expected: 53)\n";
    cout << "  payload        = \"" << recv_str << "\"  (expected: \"MAGI\")\n";
    cout << "  verifyChecksum = " << (valid ? "true" : "false") << "  (expected: true)\n";

    assert(receiver.srcPort == 12345);
    assert(receiver.dstPort == 53);
    assert(recv_str == "MAGI");
    assert(valid == true);
    cout << "[PASS] TEST 10\n";
}

int main()
{
    cout << "╔══════════════════════════════════════════╗\n";
    cout << "║     UDP Segment Unit Tests - P2          ║\n";
    cout << "╚══════════════════════════════════════════╝\n";

    test_toBytes_big_endian();
    test_deserialize_roundtrip();
    test_buildPseudoHeader();
    test_checksum_valid();
    test_checksum_corrupt_payload();
    test_checksum_corrupt_header();
    test_checksum_zero_skip();
    test_deserialize_too_short();
    test_empty_payload();
    test_full_pipeline();

    cout << "\n╔══════════════════════════════════════════╗\n";
    cout << "║  Semua 10 test PASSED.                   ║\n";
    cout << "╚══════════════════════════════════════════╝\n";
    return 0;
}