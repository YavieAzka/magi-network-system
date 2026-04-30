#include "util/TopologySerializer.hpp"

#include <sstream>
#include <string>
using namespace std;

string TopologySerializer::serialize(const TopologyConfig &topology) {
    ostringstream j;
    j << "{\n";

    j << "  \"hosts\": [\n";
    for (size_t i = 0; i < topology.hosts.size(); ++i) {
        const auto& h = topology.hosts[i];
        j << "    {\n"
          << "      \"name\": \""            << h.name            << "\",\n"
          << "      \"ip_address\": \""      << h.ip_address      << "\",\n"
          << "      \"default_gateway\": \"" << h.default_gateway << "\"\n"
          << "    }" << (i+1 < topology.hosts.size() ? "," : "") << "\n";
    }
    j << "  ],\n";

    j << "  \"switches\": [\n";
    for (size_t i = 0; i < topology.switches.size(); ++i) {
        const auto& sw = topology.switches[i];
        j << "    {\n"
          << "      \"name\": \""      << sw.name      << "\",\n"
          << "      \"num_ports\": "   << sw.num_ports << ",\n"
          << "      \"vlans\": [\n";
        for (size_t v = 0; v < sw.vlans.size(); ++v) {
            const auto& vl = sw.vlans[v];
            j << "        { \"port\": " << vl.port << ", \"mode\": \"" << vl.mode << "\"";
            if (vl.vlan_id > 0) j << ", \"vlan_id\": " << vl.vlan_id;
            j << " }" << (v+1 < sw.vlans.size() ? "," : "") << "\n";
        }
        j << "      ]\n"
          << "    }" << (i+1 < topology.switches.size() ? "," : "") << "\n";
    }
    j << "  ],\n";

    j << "  \"routers\": [\n";
    for (size_t i = 0; i < topology.routers.size(); ++i) {
        const auto& r = topology.routers[i];
        j << "    {\n"
          << "      \"name\": \"" << r.name << "\",\n"
          << "      \"interfaces\": [\n";
        for (size_t k = 0; k < r.interfaces.size(); ++k)
            j << "        { \"port\": " << r.interfaces[k].port
              << ", \"ip_address\": \"" << r.interfaces[k].ip_address << "\" }"
              << (k+1 < r.interfaces.size() ? "," : "") << "\n";
        j << "      ],\n"
          << "      \"routing_table\": [\n";
        for (size_t k = 0; k < r.routing_table.size(); ++k)
            j << "        { \"destination\": \"" << r.routing_table[k].destination
              << "\", \"next_hop\": \""           << r.routing_table[k].next_hop
              << "\", \"interface\": "            << r.routing_table[k].interface << " }"
              << (k+1 < r.routing_table.size() ? "," : "") << "\n";
        j << "      ]\n"
          << "    }" << (i+1 < topology.routers.size() ? "," : "") << "\n";
    }
    j << "  ],\n";

    j << "  \"links\": [\n";
    for (size_t i = 0; i < topology.links.size(); ++i) {
        const auto& lc = topology.links[i];
        j << "    { \"endpoints\": [\"" << lc.endpoint_a << "\", \"" << lc.endpoint_b
          << "\"], \"delay\": " << lc.delay << " }"
          << (i+1 < topology.links.size() ? "," : "") << "\n";
    }
    j << "  ]\n}\n";

    return j.str();
}