#include "layer3/FragmentReassembler.hpp"
#include <algorithm>
#include <iostream>

using namespace std;

optional<IPv4Packet> FragmentReassembler::addFragment(const IPv4Packet &frag)
{
    FragmentKey key;
    key.srcIP = frag.srcIP;
    key.dstIP = frag.dstIP;
    key.identification = frag.identification;
    key.protocol = frag.protocol;

    auto it = buffers_.find(key);
    if (it == buffers_.end())
    {
        ReassemblyBuffer buf;
        buf.createdAt = chrono::steady_clock::now();
        buf.complete = false;
        buf.fragments.push_back(frag);
        buffers_[key] = std::move(buf);
        it = buffers_.find(key);
    }
    else
    {
        bool duplicate = false;
        for (const auto &existing : it->second.fragments)
        {
            if (existing.getFragmentOffset() == frag.getFragmentOffset() &&
                existing.isMoreFragments() == frag.isMoreFragments() &&
                existing.payload.size() == frag.payload.size())
            {
                duplicate = true;
                break;
            }
        }
        if (!duplicate)
        {
            it->second.fragments.push_back(frag);
        }
    }

    bool hasLastFragment = false;
    for (const auto &f : it->second.fragments)
    {
        if (!f.isMoreFragments())
        {
            hasLastFragment = true;
            break;
        }
    }

    if (!hasLastFragment)
        return nullopt;

    uint16_t lastOffset = 0;
    size_t totalPayloadLen = 0;
    for (const auto &f : it->second.fragments)
    {
        uint16_t off = f.getFragmentOffset();
        size_t end = off + f.payload.size();
        if (end > totalPayloadLen)
            totalPayloadLen = end;

        if (!f.isMoreFragments())
        {
            hasLastFragment = true;
        }
    }

    if (totalPayloadLen > MAX_BUFFER_SIZE)
    {
        buffers_.erase(it);
        return nullopt;
    }

    sort(it->second.fragments.begin(), it->second.fragments.end(),
         [](const IPv4Packet &a, const IPv4Packet &b)
         {
             return a.getFragmentOffset() < b.getFragmentOffset();
         });

    vector<uint8_t> reassembledPayload(totalPayloadLen, 0);
    size_t filled = 0;

    for (const auto &f : it->second.fragments)
    {
        uint16_t off = f.getFragmentOffset();
        if (off + f.payload.size() > totalPayloadLen)
        {
            buffers_.erase(it);
            return nullopt;
        }
        copy(f.payload.begin(), f.payload.end(), reassembledPayload.begin() + off);
        filled += f.payload.size();
    }

    if (filled < totalPayloadLen)
        return nullopt;

    IPv4Packet result;
    result.version = 4;
    result.ihl = 5;
    result.tos = frag.tos;
    result.totalLen = static_cast<uint16_t>(20 + totalPayloadLen);
    result.identification = frag.identification;
    result.flagsFragment = 0;
    result.ttl = frag.ttl;
    result.protocol = frag.protocol;
    result.srcIP = frag.srcIP;
    result.dstIP = frag.dstIP;
    result.payload = std::move(reassembledPayload);
    result.checksum = result.calculateChecksum();

    buffers_.erase(it);

    cout << "[" << frag.dstIP << "] Fragments reassembled: "
         << result.payload.size() << " bytes payload" << endl;

    return result;
}

void FragmentReassembler::cleanup(std::chrono::seconds timeout)
{
    auto now = chrono::steady_clock::now();
    for (auto it = buffers_.begin(); it != buffers_.end();)
    {
        auto elapsed = chrono::duration_cast<chrono::seconds>(now - it->second.createdAt);
        if (elapsed > timeout)
        {
            cout << "[FragmentReassembler] Timeout, dropping fragments from "
                 << it->first.srcIP << " id=" << it->first.identification << endl;
            it = buffers_.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

size_t FragmentReassembler::pendingCount() const
{
    return buffers_.size();
}