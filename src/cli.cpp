#include "cli.hpp"

#include <iostream>
#include <sstream>
#include <algorithm>
#include "core/Host.hpp"
using namespace std;

#define C_OK "\033[32m"
#define C_ERR "\033[31m"
#define C_INFO "\033[33m"
#define C_RST "\033[0m"

void CLI::ok(const string &m) { cout << C_OK "[OK]  " C_RST << m << "\n"; }
void CLI::err(const string &m) { cout << C_ERR "[ERR] " C_RST << m << "\n"; }
void CLI::info(const string &m) { cout << C_INFO "[..] " C_RST << m << "\n"; }

CLI::CLI(Simulator &sim) : sim(sim) {}

vector<string> CLI::tokenize(const string &line) const
{
    vector<string> tokens;
    istringstream ss(line);
    string tok;
    while (ss >> tok)
        tokens.push_back(tok);
    return tokens;
}

void CLI::handleCreate(const vector<string> &args)
{
    if (args.size() < 3)
    {
        err("Penggunaan: create <host|switch|router> <nama>");
        return;
    }
    const string &type = args[1];
    const string &name = args[2];

    Node *node = sim.createNode(type, name);
    if (!node)
    {
        string t = type;
        transform(t.begin(), t.end(), t.begin(), ::tolower);
        if (t != "host" && t != "switch" && t != "router")
            err("Tipe tidak valid: '" + type + "'. Pilihan: host, switch, router");
        else
            err("Node '" + name + "' sudah ada.");
        return;
    }

    Interface *ifc = node->getInterface(1);
    ok("Dibuat " + type + " \"" + name + "\"" + (ifc ? "  [MAC=" + ifc->getMacAddress() + "]" : ""));
}

void CLI::handleLink(const vector<string> &args)
{
    if (args.size() < 3)
    {
        err("Penggunaan: link <ep1> <ep2> [delay_ms]");
        err("  ep format: NamaNode  atau  NamaNode:NomorPort");
        return;
    }
    const string &ep1 = args[1];
    const string &ep2 = args[2];
    int delay = 0;
    if (args.size() >= 4)
    {
        try
        {
            delay = stoi(args[3]);
        }
        catch (...)
        {
            err("delay_ms harus berupa angka.");
            return;
        }
    }

    auto [n1, p1] = sim.parseEndpoint(ep1);
    auto [n2, p2] = sim.parseEndpoint(ep2);
    if (!sim.findNode(n1))
    {
        err("Node tidak ditemukan: " + n1);
        return;
    }
    if (!sim.findNode(n2))
    {
        err("Node tidak ditemukan: " + n2);
        return;
    }

    if (!sim.linkNodes(ep1, ep2, delay))
    {
        err("Gagal link: salah satu interface sudah terhubung ke kabel lain.");
        return;
    }
    ok("Kabel: " + ep1 + " <--[" + to_string(delay) + "ms]--> " + ep2);
}

void CLI::handleUnlink(const vector<string> &args)
{
    if (args.size() < 3)
    {
        err("Penggunaan: unlink <ep1> <ep2>");
        return;
    }
    if (!sim.unlinkNodes(args[1], args[2]))
    {
        err("Tidak ada kabel antara " + args[1] + " dan " + args[2] + ".");
        return;
    }
    ok("Kabel diputus: " + args[1] + " <-> " + args[2]);
}

void CLI::handleSave(const vector<string> &args)
{
    string filename = (args.size() >= 2) ? args[1] : "topology.json";
    if (!sim.saveTopology(filename))
        err("Gagal menyimpan ke: " + filename);
    else
        ok("Topologi disimpan ke: " + filename);
}

void CLI::handleLoad(const vector<string> &args)
{
    if (args.size() < 2)
    {
        err("Penggunaan: load <file.json>");
        return;
    }
    info("Memuat: " + args[1] + " ...");
    if (!sim.loadTopology(args[1]))
    {
        err("Gagal memuat: " + args[1]);
        return;
    }
    ok("Dimuat — " + to_string(sim.nodes.size()) + " node, " + to_string(sim.links.size()) + " link.");
}

void CLI::handleTopology(const vector<string> &)
{
    if (sim.nodes.empty())
    {
        info("Topologi kosong. Gunakan 'create' atau 'load' untuk memulai.");
        return;
    }
    cout << "\n--- Nodes (" << sim.nodes.size() << ") ---\n";
    for (const auto &[name, node] : sim.nodes)
    {
        cout << "  " << node->getType() << " " << name << "\n";
        for (int p = 1; p <= 48; ++p)
        {
            Interface *ifc = node->getInterface(p);
            if (!ifc)
                continue;
            cout << "    port" << p << "  MAC=" << ifc->getMacAddress()
                 << (ifc->isConnected() ? "  [connected]" : "  [free]") << "\n";
        }
    }
    cout << "\n--- Links (" << sim.links.size() << ") ---\n";
    for (const auto &lnk : sim.links)
    {
        Interface *a = lnk->getInterfaceA();
        Interface *b = lnk->getInterfaceB();
        cout << "  " << a->getOwner()->getName() << ":" << a->getPortNumber()
             << " <--[" << lnk->getDelayMs() << "ms]--> "
             << b->getOwner()->getName() << ":" << b->getPortNumber() << "\n";
    }
    cout << "\n";
}

void CLI::handleHelp(const vector<string> &)
{
    cout << "\nDaftar Perintah:\n"
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

bool CLI::processCommand(const string &line)
{
    size_t start = line.find_first_not_of(" \t\r\n");
    if (start == string::npos)
        return true;
    string trimmed = line.substr(start);
    if (trimmed.empty() || trimmed[0] == '#')
        return true;

    vector<string> args = tokenize(trimmed);
    if (args.empty())
        return true;

    string firstToken = args[0];
    Node *maybeNode = sim.findNode(args[0]);
    if (maybeNode != nullptr)
    {
        Host *host = dynamic_cast<Host *>(maybeNode);

        if (args.size() >= 2 && args[1] == "arp")
        {
            if (!host)
            {
                err(args[0] + " bukan host");
                return true;
            }
            host->printArpCache();
        }
        else if (args.size() >= 3 && args[1] == "ping")
        {
            if (!host)
            {
                err(args[0] + " bukan host");
                return true;
            }
            int count = (args.size() >= 4) ? stoi(args[3]) : 4;
            host->ping(args[2], count);
        }
        else if (args.size() >= 3 && args[1] == "traceroute")
        {
            if (!host)
            {
                err(args[0] + " bukan host");
                return true;
            }
            host->traceroute(args[2]);
        }
        else
        {
            err("Perintah tidak dikenal untuk node: " + args[1]);
        }
        return true;
    }

    string cmd = firstToken;
    transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

    try
    {
        if (cmd == "exit" || cmd == "quit")
        {
            cout << "tamat-sudah offline.\n";
            return false;
        }
        else if (cmd == "create")
            handleCreate(args);
        else if (cmd == "link")
            handleLink(args);
        else if (cmd == "unlink")
            handleUnlink(args);
        else if (cmd == "save")
            handleSave(args);
        else if (cmd == "load")
            handleLoad(args);
        else if (cmd == "topology")
            handleTopology(args);
        else if (cmd == "help")
            handleHelp(args);
        else
            err("Perintah tidak dikenal: '" + args[0] + "'. Ketik 'help'.");
    }
    catch (const exception &e)
    {
        err(string("Exception: ") + e.what());
    }

    return true;
}

void CLI::run()
{
    cout << "Tubes apaan bikin network simulator\nKetik 'help' untuk bantuan.\n\n";
    string line;
    while (true)
    {
        cout << "tamat-sudah> ";
        cout.flush();
        if (!getline(cin, line))
        {
            cout << "\n";
            break;
        }
        if (!processCommand(line))
            break;
    }
}