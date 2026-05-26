#include "middleboxes/ACL.hpp"
#include "util/NetUtils.hpp"
#include <iostream>
#include <iomanip>
#include <stdexcept>

using namespace std;

static string actionStr(ACLRule::Action a)
{
    return a == ACLRule::Action::PERMIT ? "permit" : "deny";
}
static string dirStr(ACLRule::Direction d)
{
    return d == ACLRule::Direction::IN ? "in" : "out";
}
static string protoStr(uint8_t p)
{
    if (p == 1)
        return "icmp";
    if (p == 6)
        return "tcp";
    if (p == 17)
        return "udp";
    return "any";
}

string ACLRule::toString() const
{
    string s = actionStr(action) + " " + dirStr(direction);
    s += " port=" + (port == -1 ? "any" : to_string(port));
    s += " src=" + srcNet;
    s += " dst=" + dstNet;
    s += " proto=" + protoStr(protocol);
    if (srcPort != -1)
        s += " sport=" + to_string(srcPort);
    if (dstPort != -1)
        s += " dport=" + to_string(dstPort);
    return s;
}

void ACLList::addRule(const ACLRule &rule)
{
    rules_.push_back(rule);
}

void ACLList::removeRule(int index)
{
    if (index < 0 || index >= (int)rules_.size())
        throw out_of_range("ACL index out of range");
    rules_.erase(rules_.begin() + index);
}

void ACLList::printRules() const
{
    if (rules_.empty())
    {
        cout << "  (tidak ada ACL rule)\n";
        return;
    }
    cout << left << setw(4) << "#"
         << setw(8) << "Action"
         << setw(5) << "Dir"
         << setw(6) << "Port"
         << setw(20) << "Src"
         << setw(20) << "Dst"
         << setw(6) << "Proto"
         << setw(7) << "Sport"
         << "Dport\n";
    cout << string(76, '-') << "\n";
    for (int i = 0; i < (int)rules_.size(); ++i)
    {
        const auto &r = rules_[i];
        cout << left << setw(4) << i
             << setw(8) << actionStr(r.action)
             << setw(5) << dirStr(r.direction)
             << setw(6) << (r.port == -1 ? "any" : to_string(r.port))
             << setw(20) << r.srcNet
             << setw(20) << r.dstNet
             << setw(6) << protoStr(r.protocol)
             << setw(7) << (r.srcPort == -1 ? "any" : to_string(r.srcPort))
             << (r.dstPort == -1 ? "any" : to_string(r.dstPort)) << "\n";
    }
}

bool ACLList::matchNetwork(const string &ip, const string &cidrOrAny) const
{
    if (cidrOrAny == "any")
        return true;
    auto slash = cidrOrAny.find('/');
    if (slash == string::npos)
        return ip == cidrOrAny;
    string netStr = cidrOrAny.substr(0, slash);
    uint8_t prefix = static_cast<uint8_t>(stoi(cidrOrAny.substr(slash + 1)));
    uint32_t net = NetUtils::applyMask(NetUtils::stringToIp(netStr), prefix);
    uint32_t masked = NetUtils::applyMask(NetUtils::stringToIp(ip), prefix);
    return net == masked;
}

bool ACLList::isPermitted(const string &srcIp, const string &dstIp,
                          uint8_t protocol, int srcPort, int dstPort,
                          int inPort, int outPort) const
{
    for (const auto &rule : rules_)
    {
        int checkPort = (rule.direction == ACLRule::Direction::IN) ? inPort : outPort;
        if (rule.port != -1 && rule.port != checkPort)
            continue;
        if (!matchNetwork(srcIp, rule.srcNet))
            continue;
        if (!matchNetwork(dstIp, rule.dstNet))
            continue;
        if (rule.protocol != 0 && rule.protocol != protocol)
            continue;
        if (rule.srcPort != -1 && rule.srcPort != srcPort)
            continue;
        if (rule.dstPort != -1 && rule.dstPort != dstPort)
            continue;

        if (rule.action == ACLRule::Action::DENY)
        {
            cout << "[ACL] DENY: " << srcIp << " -> " << dstIp
                 << " proto=" << protoStr(protocol) << "\n";
            return false;
        }
        return true;
    }
    return true;
}