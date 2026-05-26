#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct ACLRule
{
    enum class Action
    {
        PERMIT,
        DENY
    };
    enum class Direction
    {
        IN,
        OUT
    };

    Action action;
    Direction direction;
    int port;
    std::string srcNet;
    std::string dstNet;
    uint8_t protocol;
    int srcPort;
    int dstPort;

    std::string toString() const;
};

class ACLList
{
public:
    void addRule(const ACLRule &rule);
    void removeRule(int index);
    void printRules() const;
    bool isPermitted(const std::string &srcIp, const std::string &dstIp,
                     uint8_t protocol, int srcPort, int dstPort,
                     int inPort, int outPort) const;
    bool empty() const { return rules_.empty(); }

private:
    std::vector<ACLRule> rules_;
    bool matchNetwork(const std::string &ip, const std::string &cidrOrAny) const;
};