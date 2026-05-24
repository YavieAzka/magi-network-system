#include "layer2/EthernetFrame.hpp"
#include "util/NetUtils.hpp"

#include <stdexcept>
#include <sstream>
#include <iomanip>

using namespace std;

EthernetFrame::EthernetFrame()
    : destMac(""), srcMac(""), etherType(0), vlanId(-1), fcs(0) {}

EthernetFrame::EthernetFrame(const string &destMac, const string &srcMac,
                             uint16_t etherType, const vector<uint8_t> &pld,
                             int vlanId)
    : destMac(destMac), srcMac(srcMac), etherType(etherType),
      vlanId(vlanId), fcs(0)
{
    payload = pld;
}

string EthernetFrame::getType() const
{
    return "ETHERNET";
}

vector<uint8_t> EthernetFrame::toBytes() const
{
    vector<uint8_t> frame;

    auto dst = macToBytes(destMac);
    frame.insert(frame.end(), dst.begin(), dst.end());

    auto src = macToBytes(srcMac);
    frame.insert(frame.end(), src.begin(), src.end());

    if (vlanId >= 0)
    {
        frame.push_back(0x81);
        frame.push_back(0x00);
        uint16_t tci = vlanId & 0x0FFF;
        frame.push_back((tci >> 8) & 0xFF);
        frame.push_back(tci & 0xFF);
    }

    frame.push_back((etherType >> 8) & 0xFF);
    frame.push_back(etherType & 0xFF);

    frame.insert(frame.end(), payload.begin(), payload.end());

    uint32_t computedFcs = crc32(frame);
    frame.push_back((computedFcs >> 24) & 0xFF);
    frame.push_back((computedFcs >> 16) & 0xFF);
    frame.push_back((computedFcs >> 8) & 0xFF);
    frame.push_back(computedFcs & 0xFF);

    return frame;
}

void EthernetFrame::deserialize(const vector<uint8_t> &raw)
{
    if (raw.size() < 18 || (vlanId >= 0 && raw.size() < 22))
        throw runtime_error("Error: Frame too short");

    size_t pos = 0;

    destMac = bytesToMac(raw, pos);
    pos += 6;
    srcMac = bytesToMac(raw, pos);
    pos += 6;

    uint16_t next = (static_cast<uint16_t>(raw[pos]) << 8) | raw[pos + 1];
    if (next == 0x8100)
    {
        pos += 2;
        uint16_t tci = (static_cast<uint16_t>(raw[pos]) << 8) | raw[pos + 1];
        pos += 2;
        vlanId = tci & 0x0FFF;
        etherType = (static_cast<uint16_t>(raw[pos]) << 8) | raw[pos + 1];
        pos += 2;
    }
    else
    {
        vlanId = -1;
        etherType = next;
        pos += 2;
    }

    if (raw.size() < pos + 4)
        throw runtime_error("Error: Missing FCS");

    size_t payloadLen = raw.size() - pos - 4;
    payload.assign(raw.begin() + pos, raw.begin() + pos + payloadLen);

    size_t fcsOffset = raw.size() - 4;
    fcs = (static_cast<uint32_t>(raw[fcsOffset]) << 24) |
          (static_cast<uint32_t>(raw[fcsOffset + 1]) << 16) |
          (static_cast<uint32_t>(raw[fcsOffset + 2]) << 8) |
          static_cast<uint32_t>(raw[fcsOffset + 3]);
}

void EthernetFrame::calculateAndSetFcs()
{
    vector<uint8_t> frame;

    auto dst = macToBytes(destMac);
    frame.insert(frame.end(), dst.begin(), dst.end());

    auto src = macToBytes(srcMac);
    frame.insert(frame.end(), src.begin(), src.end());

    if (vlanId >= 0)
    {
        frame.push_back(0x81);
        frame.push_back(0x00);
        uint16_t tci = vlanId & 0x0FFF;
        frame.push_back((tci >> 8) & 0xFF);
        frame.push_back(tci & 0xFF);
    }

    frame.push_back((etherType >> 8) & 0xFF);
    frame.push_back(etherType & 0xFF);
    frame.insert(frame.end(), payload.begin(), payload.end());

    fcs = crc32(frame);
}

bool EthernetFrame::verifyFcs() const
{
    vector<uint8_t> frame;

    auto dst = macToBytes(destMac);
    frame.insert(frame.end(), dst.begin(), dst.end());

    auto src = macToBytes(srcMac);
    frame.insert(frame.end(), src.begin(), src.end());

    if (vlanId >= 0)
    {
        frame.push_back(0x81);
        frame.push_back(0x00);
        uint16_t tci = vlanId & 0x0FFF;
        frame.push_back((tci >> 8) & 0xFF);
        frame.push_back(tci & 0xFF);
    }

    frame.push_back((etherType >> 8) & 0xFF);
    frame.push_back(etherType & 0xFF);
    frame.insert(frame.end(), payload.begin(), payload.end());

    return fcs == crc32(frame);
}

bool EthernetFrame::isBroadcast() const
{
    return destMac == "FF:FF:FF:FF:FF:FF";
}

bool EthernetFrame::isTagged() const
{
    return vlanId >= 0;
}

string EthernetFrame::toString() const
{
    ostringstream oss;
    oss << "EthernetFrame {"
        << " dst=" << destMac
        << " src=" << srcMac;
    if (vlanId >= 0)
        oss << " vlan=" << vlanId;
    oss << " ethertype=0x" << hex << uppercase
        << setw(4) << setfill('0') << etherType
        << " payload=" << dec << payload.size() << "B"
        << " fcs=0x" << hex << uppercase
        << setw(8) << setfill('0') << fcs
        << " }";
    return oss.str();
}

uint32_t EthernetFrame::crc32(const vector<uint8_t> &data)
{
    uint32_t crc = 0xFFFFFFFF;
    for (uint8_t byte : data)
    {
        crc ^= byte;
        for (int i = 0; i < 8; i++)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
    }
    return ~crc;
}

vector<uint8_t> EthernetFrame::macToBytes(const string &mac)
{
    return NetUtils::stringToMac(mac);
}

string EthernetFrame::bytesToMac(const vector<uint8_t> &bytes, size_t offset)
{
    if (offset + 6 > bytes.size())
        throw runtime_error("Error: Not enough bytes for MAC address");
    vector<uint8_t> mac_bytes(bytes.begin() + offset, bytes.begin() + offset + 6);
    return NetUtils::macToString(mac_bytes);
}