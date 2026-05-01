#include <iostream>
#include <vector>
#include <string>
#include "../include/layer2/Switch.hpp"
#include "../include/layer2/EthernetFrame.hpp"

static int testsPassed = 0;
static int testsFailed = 0;

static void check(const std::string& testName, bool condition) {
    if (condition) {
        std::cout << "[PASS] " << testName << "\n";
        testsPassed++;
    } else {
        std::cout << "[FAIL] " << testName << "\n";
        testsFailed++;
    }
}

static std::vector<uint8_t> makeFrame(
    const std::string& dst,
    const std::string& src,
    int vlanId = -1)
{
    EthernetFrame f(dst, src, 0x0800, {}, vlanId);
    f.calculateAndSetFcs();
    return f.toBytes();
}

void testLearnAndLookup() {
    std::cout << "\n── TEST: learnMAC & lookupMAC ──\n";
    Switch sw("SW1", 4);

    sw.learnMAC(10, "aa:bb:cc:dd:ee:ff", 3);
    check("Lookup MAC yang sudah di-learn",
          sw.lookupMAC(10, "aa:bb:cc:dd:ee:ff") == 3);

    check("Lookup MAC yang belum di-learn return -1",
          sw.lookupMAC(10, "11:22:33:44:55:66") == -1);

    sw.learnMAC(20, "aa:bb:cc:dd:ee:ff", 2);
    check("MAC sama di VLAN berbeda (VLAN 10)",
          sw.lookupMAC(10, "aa:bb:cc:dd:ee:ff") == 3);
    check("MAC sama di VLAN berbeda (VLAN 20)",
          sw.lookupMAC(20, "aa:bb:cc:dd:ee:ff") == 2);
}

void testMACUpdate() {
    std::cout << "\n── TEST: MAC update (device pindah port) ──\n";
    Switch sw("SW1", 4);

    sw.learnMAC(10, "aa:bb:cc:dd:ee:ff", 3);
    check("MAC pertama kali di-learn di port 3",
          sw.lookupMAC(10, "aa:bb:cc:dd:ee:ff") == 3);

    sw.learnMAC(10, "aa:bb:cc:dd:ee:ff", 1);
    check("MAC update ke port baru setelah device pindah",
          sw.lookupMAC(10, "aa:bb:cc:dd:ee:ff") == 1);
}

void testConfigurePort() {
    std::cout << "\n── TEST: configurePort ──\n";
    Switch sw("SW1", 4);

    bool noThrow = true;
    try {
        sw.configurePort(1, Switch::ACCESS, 10);
        sw.configurePort(2, Switch::TRUNK,  1, {10, 20});
    } catch (...) { noThrow = false; }
    check("configurePort ACCESS dan TRUNK tidak crash", noThrow);

    bool noThrowInvalid = true;
    try {
        sw.configurePort(0,  Switch::ACCESS, 10);
        sw.configurePort(99, Switch::ACCESS, 10);
    } catch (...) { noThrowInvalid = false; }
    check("configurePort port tidak valid tidak crash", noThrowInvalid);
}

void testHandleReceiveLearnMAC() {
    std::cout << "\n── TEST: handleReceive MAC learning ──\n";
    Switch sw("SW1", 4);
    sw.configurePort(1, Switch::ACCESS, 10);

    auto raw = makeFrame("ff:ff:ff:ff:ff:ff", "aa:bb:cc:dd:ee:ff");
    sw.handleReceive(1, raw);

    check("handleReceive learn src MAC di VLAN pvid port masuk",
          sw.lookupMAC(10, "aa:bb:cc:dd:ee:ff") == 1);
}

void testHandleReceiveDoesNotLearnBroadcastSrc() {
    std::cout << "\n── TEST: handleReceive tidak learn broadcast src ──\n";
    Switch sw("SW1", 4);
    sw.configurePort(1, Switch::ACCESS, 10);

    auto raw = makeFrame("aa:bb:cc:dd:ee:ff", "ff:ff:ff:ff:ff:ff");
    sw.handleReceive(1, raw);

    check("Broadcast src MAC tidak di-learn",
          sw.lookupMAC(10, "ff:ff:ff:ff:ff:ff") == -1);
}

void testHandleReceiveInvalidPort() {
    std::cout << "\n── TEST: handleReceive port tidak valid ──\n";
    Switch sw("SW1", 4);

    auto raw = makeFrame("ff:ff:ff:ff:ff:ff", "aa:bb:cc:dd:ee:ff");

    bool noThrow = true;
    try {
        sw.handleReceive(0,  raw);
        sw.handleReceive(99, raw);
    } catch (...) { noThrow = false; }
    check("handleReceive port tidak valid tidak crash", noThrow);
}

void testGetType() {
    std::cout << "\n── TEST: getType ──\n";
    Switch sw("SW1", 4);
    check("getType return \"switch\"", sw.getType() == "switch");
}

int main() {
    std::cout << "╔══════════════════════════════════╗\n";
    std::cout << "║       SWITCH UNIT TESTS          ║\n";
    std::cout << "╚══════════════════════════════════╝\n";

    testLearnAndLookup();
    testMACUpdate();
    testConfigurePort();
    testHandleReceiveLearnMAC();
    testHandleReceiveDoesNotLearnBroadcastSrc();
    testHandleReceiveInvalidPort();
    testGetType();

    std::cout << "\n══════════════════════════════════\n";
    std::cout << "PASSED : " << testsPassed << "\n";
    std::cout << "FAILED : " << testsFailed << "\n";
    std::cout << "══════════════════════════════════\n";

    return testsFailed > 0 ? 1 : 0;
}