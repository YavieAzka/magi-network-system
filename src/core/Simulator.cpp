#include "core/Simulator.hpp"
#include "core/Node.hpp"
#include "core/Interface.hpp"
#include "core/Link.hpp"
#include "core/Router.hpp"
#include "layer2/Switch.hpp"
#include "util/TopologyParser.hpp"
#include "util/TopologySerializer.hpp"
#include "core/Host.hpp"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>
using namespace std;

string Simulator::generateMac()
{
    ++macCounter;
    ostringstream oss;
    oss << "CA:FE:" << hex << uppercase << setfill('0')
        << setw(2) << ((macCounter >> 16) & 0xFF) << ":"
        << setw(2) << ((macCounter >> 8) & 0xFF) << ":"
        << setw(2) << (macCounter & 0xFF) << ":01";
    return oss.str();
}

Node *Simulator::findNode(const string &name) const
{
    auto it = nodes.find(name);
    return (it != nodes.end()) ? it->second.get() : nullptr;
}

pair<string, int> Simulator::parseEndpoint(const string &ep) const
{
    auto colon = ep.find(':');
    if (colon == string::npos)
        return {ep, 1};
    return {ep.substr(0, colon), stoi(ep.substr(colon + 1))};
}

Node *Simulator::createNode(const string &type, const string &name)
{
    if (findNode(name))
        return nullptr;
    unique_ptr<Node> newNode;
    string t = type;
    transform(t.begin(), t.end(), t.begin(), ::tolower);

    if (t == "host")
    {
        newNode = make_unique<Host>(name, "0.0.0.0/0", "0.0.0.0");
        HostConfig hc;
        hc.name = name;
        hc.ip_address = "0.0.0.0/0";
        hc.default_gateway = "0.0.0.0";
        topology.hosts.push_back(hc);
    }
    else if (t == "switch")
    {
        newNode = make_unique<Switch>(name);
        SwitchConfig sc;
        sc.name = name;
        sc.num_ports = 24;
        topology.switches.push_back(sc);
    }
    else if(t == "router")
    {
        newNode = make_unique<Router>(name);
        RouterConfig rc;
        rc.name = name;
        RouterInterfaceConfig ifc;
        ifc.port = 1;
        ifc.ip_address = "0.0.0.0/0";
        rc.interfaces.push_back(ifc);
        topology.routers.push_back(rc);
    }   
    else
    {
        return nullptr;
    }

    newNode->addInterface(1, generateMac());
    Node *ptr = newNode.get();
    nodes[name] = std::move(newNode);

    return ptr;
}

bool Simulator::linkNodes(const string &ep1, const string &ep2, int delayMs)
{
    auto [name1, port1] = parseEndpoint(ep1);
    auto [name2, port2] = parseEndpoint(ep2);

    Node *n1 = findNode(name1);
    Node *n2 = findNode(name2);
    if (!n1 || !n2)
        return false;

    Interface *ifc1 = n1->getInterface(port1);
    if (!ifc1)
        ifc1 = n1->addInterface(port1, generateMac());

    Interface *ifc2 = n2->getInterface(port2);
    if (!ifc2)
        ifc2 = n2->addInterface(port2, generateMac());

    if (ifc1->isConnected() || ifc2->isConnected())
        return false;

    auto lnk = make_unique<Link>(ifc1, ifc2, delayMs);
    links.push_back(std::move(lnk));

    LinkConfig lc;
    lc.endpoint_a = ep1;
    lc.endpoint_b = ep2;
    lc.delay = delayMs;
    topology.links.push_back(lc);

    return true;
}

bool Simulator::unlinkNodes(const string &ep1, const string &ep2)
{
    auto [name1, port1] = parseEndpoint(ep1);
    auto [name2, port2] = parseEndpoint(ep2);

    Node *n1 = findNode(name1);
    Node *n2 = findNode(name2);
    if (!n1 || !n2)
        return false;

    Interface *ifc1 = n1->getInterface(port1);
    Interface *ifc2 = n2->getInterface(port2);
    if (!ifc1 || !ifc2)
        return false;

    Link *target = nullptr;
    for (auto &lnk : links)
    {
        bool ab = (lnk->getInterfaceA() == ifc1 && lnk->getInterfaceB() == ifc2);
        bool ba = (lnk->getInterfaceA() == ifc2 && lnk->getInterfaceB() == ifc1);
        if (ab || ba)
        {
            target = lnk.get();
            break;
        }
    }
    if (!target)
        return false;

    ifc1->detachLink();
    ifc2->detachLink();

    links.erase(remove_if(links.begin(), links.end(),
                          [target](const unique_ptr<Link> &l)
                          { return l.get() == target; }),
                links.end());

    topology.links.erase(remove_if(topology.links.begin(), topology.links.end(),
                                   [&ep1, &ep2](const LinkConfig &lc)
                                   {
                                       return (lc.endpoint_a == ep1 && lc.endpoint_b == ep2) ||
                                              (lc.endpoint_a == ep2 && lc.endpoint_b == ep1);
                                   }),
                         topology.links.end());

    return true;
}

void Simulator::buildFromConfig()
{
    nodes.clear();
    links.clear();

    for (const auto &h : topology.hosts)
    {
        auto n = make_unique<Host>(h.name, h.ip_address, h.default_gateway);
        n->addInterface(1, generateMac());
        nodes[h.name] = std::move(n);
    }
    for (const auto &sw : topology.switches)
    {
        auto n = make_unique<Switch>(sw.name);
        n->addInterface(1, generateMac());
        for (const auto &vl : sw.vlans)
            if (!n->getInterface(vl.port))
                n->addInterface(vl.port, generateMac());
        nodes[sw.name] = std::move(n);
    }
    for (const auto &r : topology.routers)
    {
        auto n = make_unique<Router>(r.name);
        for (const auto &ifc : r.interfaces){
            n->addInterface(ifc.port, generateMac());
            n->setInterfaceIp(ifc.port, ifc.ip_address);
        }
        for (const auto& rt : r.routing_table) {
            n->addRoute(rt.destination, rt.next_hop, rt.interface);
        }
        nodes[r.name] = std::move(n);
    }

    for (const auto &lc : topology.links)
    {
        auto [name1, port1] = parseEndpoint(lc.endpoint_a);
        auto [name2, port2] = parseEndpoint(lc.endpoint_b);
        Node *n1 = findNode(name1);
        Node *n2 = findNode(name2);
        if (!n1 || !n2)
            continue;
        Interface *i1 = n1->getInterface(port1);
        Interface *i2 = n2->getInterface(port2);
        if (!i1)
            i1 = n1->addInterface(port1, generateMac());
        if (!i2)
            i2 = n2->addInterface(port2, generateMac());
        auto lnk = make_unique<Link>(i1, i2, lc.delay);
        links.push_back(std::move(lnk));
    }
}

bool Simulator::saveTopology(const string &filepath) const
{
    ofstream out(filepath);
    if (!out.is_open())
        return false;
    out << TopologySerializer::serialize(topology);
    return true;
}

bool Simulator::loadTopology(const string &filepath)
{
    try
    {
        topology = TopologyParser::parseFile(filepath);
        macCounter = 0;
        buildFromConfig();
        return true;
    }
    catch (const exception &e)
    {
        cerr << "[ERR] loadTopology: " << e.what() << "\n";
        return false;
    }
}