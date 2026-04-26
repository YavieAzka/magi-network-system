#include "../../include/core/TopologyParser.hpp"
#include "../../include/core/JsonParser.hpp"
#include <stdexcept>

static HostConfig parseHost(const JsonValue &j)
{
    HostConfig h;
    h.name = j["name"].as_string();
    h.ip_address = j["ip_address"].as_string();
    h.default_gateway = j["default_gateway"].as_string();
    return h;
}

static VlanConfig parseVlan(const JsonValue &j)
{
    VlanConfig v;
    v.port = static_cast<int>(j["port"].as_int());
    v.mode = j["mode"].as_string();

    if (j.has("vlan_id"))
        v.vlan_id = static_cast<int>(j["vlan_id"].as_int());

    return v;
}

static SwitchConfig parseSwitch(const JsonValue &j)
{
    SwitchConfig s;
    s.name = j["name"].as_string();
    s.num_ports = static_cast<int>(j["num_ports"].as_int());

    if (j.has("vlans"))
    {
        const auto &vlans = j["vlans"].as_array();
        for (size_t i = 0; i < vlans.size(); i++)
            s.vlans.push_back(parseVlan(vlans[i]));
    }

    return s;
}

static RouterInterfaceConfig parseRouterInterface(const JsonValue &j)
{
    RouterInterfaceConfig ri;
    ri.port = static_cast<int>(j["port"].as_int());
    ri.ip_address = j["ip_address"].as_string();
    return ri;
}

static RouteEntry parseRoute(const JsonValue &j)
{
    RouteEntry r;
    r.destination = j["destination"].as_string();
    r.next_hop = j["next_hop"].as_string();
    r.interface = static_cast<int>(j["interface"].as_int());
    return r;
}

static RouterConfig parseRouter(const JsonValue &j)
{
    RouterConfig r;
    r.name = j["name"].as_string();

    if (j.has("interfaces"))
    {
        const auto &ifs = j["interfaces"].as_array();
        for (size_t i = 0; i < ifs.size(); i++)
            r.interfaces.push_back(parseRouterInterface(ifs[i]));
    }

    if (j.has("routing_table"))
    {
        const auto &routes = j["routing_table"].as_array();
        for (size_t i = 0; i < routes.size(); i++)
            r.routing_table.push_back(parseRoute(routes[i]));
    }

    return r;
}

static LinkConfig parseLink(const JsonValue &j)
{
    LinkConfig l;
    const auto &eps = j["endpoints"].as_array();

    if (eps.size() != 2)
        throw runtime_error("Link must have exactly 2 endpoints.");

    l.endpoint_a = eps[0].as_string();
    l.endpoint_b = eps[1].as_string();

    if (j.has("delay"))
        l.delay = static_cast<int>(j["delay"].as_int());
    else
        l.delay = 0;

    return l;
}

static TopologyConfig parseJson(const JsonValue &root)
{
    TopologyConfig config;

    if (root.has("hosts"))
    {
        const auto &hosts = root["hosts"].as_array();
        for (size_t i = 0; i < hosts.size(); i++)
            config.hosts.push_back(parseHost(hosts[i]));
    }

    if (root.has("switches"))
    {
        const auto &switches = root["switches"].as_array();
        for (size_t i = 0; i < switches.size(); i++)
            config.switches.push_back(parseSwitch(switches[i]));
    }

    if (root.has("routers"))
    {
        const auto &routers = root["routers"].as_array();
        for (size_t i = 0; i < routers.size(); i++)
            config.routers.push_back(parseRouter(routers[i]));
    }

    if (root.has("links"))
    {
        const auto &links = root["links"].as_array();
        for (size_t i = 0; i < links.size(); i++)
            config.links.push_back(parseLink(links[i]));
    }

    return config;
}

TopologyConfig TopologyParser::parseFile(const string &filepath)
{
    JsonValue root = JsonParser::parseFile(filepath);
    return parseJson(root);
}

TopologyConfig TopologyParser::parseString(const string &jsonStr)
{
    JsonValue root = JsonParser::parse(jsonStr);
    return parseJson(root);
}
