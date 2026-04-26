#ifndef TOPOLOGY_TYPES_HPP
#define TOPOLOGY_TYPES_HPP

#include <string>
#include <vector>
using namespace std;

struct VlanConfig
{
    int port;
    string mode;
    int vlan_id;

    VlanConfig() : port(0), vlan_id(0) {}
};

struct HostConfig
{
    string name;
    string ip_address;
    string default_gateway;
};

struct SwitchConfig
{
    string name;
    int num_ports;
    vector<VlanConfig> vlans;
};

struct RouterInterfaceConfig
{
    int port;
    string ip_address;
};

struct RouteEntry
{
    string destination;
    string next_hop;
    int interface;
};

struct RouterConfig
{
    string name;
    vector<RouterInterfaceConfig> interfaces;
    vector<RouteEntry> routing_table;
};

struct LinkConfig
{
    string endpoint_a;
    string endpoint_b;
    int delay;
};

struct TopologyConfig
{
    vector<HostConfig> hosts;
    vector<SwitchConfig> switches;
    vector<RouterConfig> routers;
    vector<LinkConfig> links;
};

#endif