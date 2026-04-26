
#include "cli.hpp"
#include "../include/core/JsonParser.hpp"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>
using namespace std;

#define C_OK    "\033[32m"
#define C_ERR   "\033[31m"
#define C_INFO  "\033[33m"
#define C_RST   "\033[0m"

void CLI::ok  (const string& m) { cout << C_OK  "[OK]  " C_RST << m << "\n"; }
void CLI::err (const string& m) { cout << C_ERR "[ERR] " C_RST << m << "\n"; }
void CLI::info(const string& m) { cout << C_INFO "[..] " C_RST  << m << "\n"; }


string Simulator::generateMac() {
    ++macCounter;
    ostringstream oss;
    oss << "CA:FE:" << hex << uppercase << setfill('0')
        << setw(2) << ((macCounter >> 16) & 0xFF) << ":"
        << setw(2) << ((macCounter >>  8) & 0xFF) << ":"
        << setw(2) << ( macCounter        & 0xFF) << ":01";
    return oss.str();
}

Node* Simulator::findNode(const string& name) const {
    auto it = nodes.find(name);
    return (it != nodes.end()) ? it->second.get() : nullptr;
}

pair<string, int> Simulator::parseEndpoint(const string& ep) const {
    auto colon = ep.find(':');
    if (colon == string::npos) return {ep, 1};
    return {ep.substr(0, colon), stoi(ep.substr(colon + 1))};
}

Node* Simulator::createNode(const string& type, const string& name) {
    if (nodes.count(name)) return nullptr;

    string t = type;
    transform(t.begin(), t.end(), t.begin(), ::tolower);
    if (t != "host" && t != "switch" && t != "router") return nullptr;

    auto node = make_unique<Node>(name);
    Node* ptr = node.get();
    nodes[name] = move(node);

    // Semua node mulai dengan minimal satu interface di port 1
    ptr->addInterface(1, generateMac());

    // Catat ke TopologyConfig agar save/load konsisten
    if (t == "host") {
        HostConfig hc;
        hc.name            = name;
        hc.ip_address      = "0.0.0.0/0";
        hc.default_gateway = "0.0.0.0";
        topology.hosts.push_back(hc);
    } else if (t == "switch") {
        SwitchConfig sc;
        sc.name      = name;
        sc.num_ports = 24;
        topology.switches.push_back(sc);
    } else { // router
        RouterConfig rc;
        rc.name = name;
        RouterInterfaceConfig ifc;
        ifc.port       = 1;
        ifc.ip_address = "0.0.0.0/0";
        rc.interfaces.push_back(ifc);
        topology.routers.push_back(rc);
    }

    return ptr;
}

bool Simulator::linkNodes(const string& ep1, const string& ep2, int delayMs) {
    auto [name1, port1] = parseEndpoint(ep1);
    auto [name2, port2] = parseEndpoint(ep2);

    Node* n1 = findNode(name1);
    Node* n2 = findNode(name2);
    if (!n1 || !n2) return false;

    // Auto-create interface jika port belum ada
    Interface* ifc1 = n1->getInterface(port1);
    if (!ifc1) ifc1 = n1->addInterface(port1, generateMac());

    Interface* ifc2 = n2->getInterface(port2);
    if (!ifc2) ifc2 = n2->addInterface(port2, generateMac());

    if (ifc1->isConnected() || ifc2->isConnected()) return false;

    auto lnk = make_unique<Link>(ifc1, ifc2, delayMs);
    ifc1->attachLink(lnk.get());
    ifc2->attachLink(lnk.get());
    links.push_back(move(lnk));

    LinkConfig lc;
    lc.endpoint_a = ep1;
    lc.endpoint_b = ep2;
    lc.delay      = delayMs;
    topology.links.push_back(lc);

    return true;
}

bool Simulator::unlinkNodes(const string& ep1, const string& ep2) {
    auto [name1, port1] = parseEndpoint(ep1);
    auto [name2, port2] = parseEndpoint(ep2);

    Node* n1 = findNode(name1);
    Node* n2 = findNode(name2);
    if (!n1 || !n2) return false;

    Interface* ifc1 = n1->getInterface(port1);
    Interface* ifc2 = n2->getInterface(port2);
    if (!ifc1 || !ifc2) return false;

    // Cari link yang menghubungkan persis kedua interface ini
    Link* target = nullptr;
    for (auto& lnk : links) {
        bool ab = (lnk->getInterfaceA() == ifc1 && lnk->getInterfaceB() == ifc2);
        bool ba = (lnk->getInterfaceA() == ifc2 && lnk->getInterfaceB() == ifc1);
        if (ab || ba) { target = lnk.get(); break; }
    }
    if (!target) return false;

    ifc1->detachLink();
    ifc2->detachLink();

    links.erase(remove_if(links.begin(), links.end(),
        [target](const unique_ptr<Link>& l){ return l.get() == target; }),
        links.end());

    // Sinkronkan topology.links
    topology.links.erase(remove_if(topology.links.begin(), topology.links.end(),
        [&ep1, &ep2](const LinkConfig& lc){
            return (lc.endpoint_a == ep1 && lc.endpoint_b == ep2) ||
                   (lc.endpoint_a == ep2 && lc.endpoint_b == ep1);
        }), topology.links.end());

    return true;
}

void Simulator::buildFromConfig() {
    nodes.clear();
    links.clear();

    for (const auto& h : topology.hosts) {
        auto n = make_unique<Node>(h.name);
        n->addInterface(1, generateMac());
        nodes[h.name] = move(n);
    }
    for (const auto& sw : topology.switches) {
        auto n = make_unique<Node>(sw.name);
        // Buat interface untuk setiap port yang ada di konfigurasi VLAN,
        // minimal port 1 agar switch bisa langsung dipakai
        n->addInterface(1, generateMac());
        for (const auto& vl : sw.vlans)
            if (!n->getInterface(vl.port))
                n->addInterface(vl.port, generateMac());
        nodes[sw.name] = move(n);
    }
    for (const auto& r : topology.routers) {
        auto n = make_unique<Node>(r.name);
        for (const auto& ifc : r.interfaces)
            n->addInterface(ifc.port, generateMac());
        nodes[r.name] = move(n);
    }

    // Bangun kabel
    for (const auto& lc : topology.links) {
        auto [name1, port1] = parseEndpoint(lc.endpoint_a);
        auto [name2, port2] = parseEndpoint(lc.endpoint_b);
        Node* n1 = findNode(name1);
        Node* n2 = findNode(name2);
        if (!n1 || !n2) continue;
        Interface* i1 = n1->getInterface(port1);
        Interface* i2 = n2->getInterface(port2);
        if (!i1) i1 = n1->addInterface(port1, generateMac());
        if (!i2) i2 = n2->addInterface(port2, generateMac());
        auto lnk = make_unique<Link>(i1, i2, lc.delay);
        i1->attachLink(lnk.get());
        i2->attachLink(lnk.get());
        links.push_back(move(lnk));
    }
}

string Simulator::topologyToJson() const {
    ostringstream j;
    j << "{\n";

    // hosts
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

    // switches
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

    // routers
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

    // links
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

bool Simulator::saveTopology(const string& filepath) const {
    ofstream out(filepath);
    if (!out.is_open()) return false;
    out << topologyToJson();
    return true;
}

bool Simulator::loadTopology(const string& filepath) {
    try {
        topology = TopologyParser::parseFile(filepath);
        macCounter = 0; // reset agar MAC baru saat rebuild
        buildFromConfig();
        return true;
    } catch (const exception& e) {
        cerr << C_ERR "[ERR] " C_RST "loadTopology: " << e.what() << "\n";
        return false;
    }
}

CLI::CLI(Simulator& sim) : sim(sim) {}

vector<string> CLI::tokenize(const string& line) const {
    vector<string> tokens;
    istringstream ss(line);
    string tok;
    while (ss >> tok) tokens.push_back(tok);
    return tokens;
}

void CLI::handleCreate(const vector<string>& args) {
    if (args.size() < 3) {
        err("Penggunaan: create <host|switch|router> <nama>");
        return;
    }
    const string& type = args[1];
    const string& name = args[2];

    Node* node = sim.createNode(type, name);
    if (!node) {
        // createNode return nullptr jika nama duplikat atau tipe salah
        string t = type;
        transform(t.begin(), t.end(), t.begin(), ::tolower);
        if (t != "host" && t != "switch" && t != "router")
            err("Tipe tidak valid: '" + type + "'. Pilihan: host, switch, router");
        else
            err("Node '" + name + "' sudah ada.");
        return;
    }

    Interface* ifc = node->getInterface(1);
    ok("Dibuat " + type + " \"" + name + "\""
       + (ifc ? "  [MAC=" + ifc->getMacAddress() + "]" : ""));
}

void CLI::handleLink(const vector<string>& args) {
    if (args.size() < 3) {
        err("Penggunaan: link <ep1> <ep2> [delay_ms]");
        err("  ep format: NamaNode  atau  NamaNode:NomorPort");
        return;
    }
    const string& ep1 = args[1];
    const string& ep2 = args[2];
    int delay = 0;
    if (args.size() >= 4) {
        try { delay = stoi(args[3]); }
        catch (...) { err("delay_ms harus berupa angka."); return; }
    }

    auto [n1, p1] = sim.parseEndpoint(ep1);
    auto [n2, p2] = sim.parseEndpoint(ep2);
    if (!sim.findNode(n1)) { err("Node tidak ditemukan: " + n1); return; }
    if (!sim.findNode(n2)) { err("Node tidak ditemukan: " + n2); return; }

    if (!sim.linkNodes(ep1, ep2, delay)) {
        err("Gagal link: salah satu interface sudah terhubung ke kabel lain.");
        return;
    }
    ok("Kabel: " + ep1 + " <--[" + to_string(delay) + "ms]--> " + ep2);
}

void CLI::handleUnlink(const vector<string>& args) {
    if (args.size() < 3) {
        err("Penggunaan: unlink <ep1> <ep2>");
        return;
    }
    if (!sim.unlinkNodes(args[1], args[2])) {
        err("Tidak ada kabel antara " + args[1] + " dan " + args[2] + ".");
        return;
    }
    ok("Kabel diputus: " + args[1] + " <-> " + args[2]);
}

void CLI::handleSave(const vector<string>& args) {
    string filename = (args.size() >= 2) ? args[1] : "topology.json";
    if (!sim.saveTopology(filename))
        err("Gagal menyimpan ke: " + filename);
    else
        ok("Topologi disimpan ke: " + filename);
}


void CLI::handleLoad(const vector<string>& args) {
    if (args.size() < 2) { err("Penggunaan: load <file.json>"); return; }
    info("Memuat: " + args[1] + " ...");
    if (!sim.loadTopology(args[1])) {
        err("Gagal memuat: " + args[1]);
        return;
    }
    ok("Dimuat — " + to_string(sim.nodes.size()) + " node, "
       + to_string(sim.links.size()) + " link.");
}


void CLI::handleTopology(const vector<string>&) {
    if (sim.nodes.empty()) {
        info("Topologi kosong. Gunakan 'create' atau 'load' untuk memulai.");
        return;
    }
    cout << "\n--- Nodes (" << sim.nodes.size() << ") ---\n";
    for (const auto& [name, node] : sim.nodes) {
        cout << "  " << node->getType() << " " << name << "\n";
        // Cetak tiap interface dan status koneksinya
        for (int p = 1; p <= 48; ++p) {
            Interface* ifc = node->getInterface(p);
            if (!ifc) continue;
            cout << "    port" << p << "  MAC=" << ifc->getMacAddress()
                 << (ifc->isConnected() ? "  [connected]" : "  [free]") << "\n";
        }
    }
    cout << "\n--- Links (" << sim.links.size() << ") ---\n";
    for (const auto& lnk : sim.links) {
        Interface* a = lnk->getInterfaceA();
        Interface* b = lnk->getInterfaceB();
        cout << "  " << a->getOwner()->getName() << ":" << a->getPortNumber()
             << " <--[" << lnk->getDelayMs() << "ms]--> "
             << b->getOwner()->getName() << ":" << b->getPortNumber() << "\n";
    }
    cout << "\n";
}

void CLI::handleHelp(const vector<string>&) {
    cout <<
        "\nDaftar Perintah:\n"
        "  create <host|switch|router> <nama>  Buat node baru\n"
        "  link   <ep1> <ep2> [delay_ms]       Sambungkan dua node\n"
        "  unlink <ep1> <ep2>                  Putuskan kabel\n"
        "  topology                            Tampilkan state di memori\n"
        "  save   [file.json]                  Ekspor ke JSON\n"
        "  load   <file.json>                  Impor dari JSON\n"
        "  help                                Daftar perintah ini\n"
        "  exit / quit                         Keluar\n"
        "\nFormat endpoint: NamaNode  atau  NamaNode:NomorPort\n"
        "  Contoh: link H1 SW1:3 10\n\n";
}


bool CLI::processCommand(const string& line) {
    // Trim
    size_t start = line.find_first_not_of(" \t\r\n");
    if (start == string::npos) return true;
    string trimmed = line.substr(start);
    if (trimmed.empty() || trimmed[0] == '#') return true;

    vector<string> args = tokenize(trimmed);
    if (args.empty()) return true;

    string cmd = args[0];
    transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

    try {
        if      (cmd == "exit" || cmd == "quit") { cout << "tamat-sudah offline.\n"; return false; }
        else if (cmd == "create")   handleCreate(args);
        else if (cmd == "link")     handleLink(args);
        else if (cmd == "unlink")   handleUnlink(args);
        else if (cmd == "save")     handleSave(args);
        else if (cmd == "load")     handleLoad(args);
        else if (cmd == "topology") handleTopology(args);
        else if (cmd == "help")     handleHelp(args);
        else err("Perintah tidak dikenal: '" + args[0] + "'. Ketik 'help'.");
    } catch (const exception& e) {
        err(string("Exception: ") + e.what());
    }

    return true;
}

void CLI::run() {
    cout << "Tubes apaan bikin network simulator\nKetik 'help' untuk bantuan.\n\n";
    string line;
    while (true) {
        cout << "tamat-sudah> ";
        cout.flush();
        if (!getline(cin, line)) { cout << "\n"; break; }
        if (!processCommand(line)) break;
    }
}