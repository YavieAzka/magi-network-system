#include "cli.hpp"

#include <iostream>
#include <sstream>
#include <algorithm>
#include "core/Host.hpp"
#include "core/Router.hpp"
#include "layer2/Switch.hpp"
#include "layer7/HTTPMessage.hpp"
#include "util/TopologyVisualizer.hpp"
#include "layer7/DHCPClient.hpp"
#include "layer7/DHCPServer.hpp"
#include "layer7/DNSServer.hpp"
#include "layer7/DNSClient.hpp"
#include "layer7/HTTPClient.hpp"
#include "core/Interface.hpp"
#include <map>
using namespace std;

map<string, DHCPServer *> g_dhcpServers;
map<string, DNSServer *> g_dnsServers;
map<string, string> g_hostDnsIps;

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
            "  create   <host|switch|router> <nama>             Buat node baru\n"
            "  link     <ep1> <ep2> [delay_ms]                  Sambungkan dua node\n"
            "  unlink   <ep1> <ep2>                             Putuskan kabel\n"
            "  topology                                         Tampilkan state di memori\n"
            "  save     [file.json]                             Ekspor ke JSON\n"
            "  load     <file.json>                             Impor dari JSON\n"
            "  help                                             Daftar perintah ini\n"
            "  exit / quit                                      Keluar\n"
            "  <host>   set-ip <IP/CIDR>                        Set IP host\n"
            "  <host>   set-gw <IP>                             Set gateway host\n"
            "  <host>   ping <IP> [count]                       Kirim ICMP ping\n"
            "  <host>   traceroute <IP>                         Lacak rute hop L3\n"
            "  <host>   arp                                     Tampilkan ARP cache host\n"
            "  <host>   udp_send <dIP> <dPort> <sPort> <m>      Kirim datagram UDP\n"
            "  <host>   tcp_listen <port>                       Buka port TCP LISTEN\n"
            "  <host>   tcp_connect <IP> <port>                 Inisiasi TCP Handshake\n"
            "  <host>   tcp_send <port> <msg>                   Kirim data TCP PSH|ACK\n"
            "  <host>   tcp_close <port>                        Initiate TCP teardown\n"
            "  <host>   http_server <port> [webroot]            Start HTTP server on port\n"
            "  <host>   http_get <url>                          HTTP GET (domain auto-resolved via set-dns)\n"
            "  <host>   set-dns <IP>                            Set DNS server IP for this host\n"
            "  <host>   dns_server start                        Start DNS server on host\n"
            "  <host>   dns_server add <domain> <ip>            Add A record to DNS server\n"
            "  <host>   dns_server records                      Show DNS records\n"
            "  <host>   dns_server stop                         Stop DNS server\n"
            "  <host>   dns_resolve <domain>                    Resolve domain (needs set-dns first)\n"
            "  <router> set-ip <port> <IP/CIDR>                 Set IP port router\n"
            "  <router> ip route <net> <hop> <port>             Tambah static route\n"
            "  <router> route                                   Tampilkan routing table\n"
            "  <router> arp                                     Tampilkan ARP cache router\n"
            "  <router> nat enable <pubIP> <privPort> <pubPort> Aktifkan NAT/PAT\n"
            "  <router> nat table                               Tampilkan NAT translation table\n"
            "  <switch> mac / mactable                          Tampilkan MAC table\n"
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
        Router *router = dynamic_cast<Router *>(maybeNode);
        Switch *sw = dynamic_cast<Switch *>(maybeNode);
        if (args.size() >= 2 && args[1] == "arp")
        {
            if (host)
            {
                host->printArpCache();
            }
            else if (router)
            {
                router->printArpCache();
            }
            else
            {
                err(args[0] + " bukan host");
                return true;
            }
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
        else if (args.size() >= 4 && args[1] == "tcp_connect")
        {
            if (host)
            {
                string destIp = args[2];
                uint16_t destPort = stoi(args[3]);
                host->tcpConnect(destIp, destPort);
                ok("Memicu tcpConnect ke " + destIp + ":" + to_string(destPort));
            }
            else
            {
                err("Hanya host yang bisa melakukan tcp_connect.");
            }
        }
        else if (args.size() >= 6 && args[1] == "udp_send")
        {
            if (host)
            {
                string destIp = args[2];
                uint16_t destPort = stoi(args[3]);
                uint16_t srcPort = stoi(args[4]);
                string message = args[5];
                for (size_t i = 6; i < args.size(); ++i)
                {
                    message += " " + args[i];
                }

                vector<uint8_t> payload(message.begin(), message.end());
                host->sendUDP(destIp, destPort, srcPort, payload);
                ok("Datagram UDP terkirim dari " + host->getName());
            }
            else
            {
                err("Hanya host yang bisa melakukan udp_send.");
            }
        }

        else if (args.size() >= 3 && args[1] == "set-ip")
        {
            if (host)
            {
                // Format: h1 set-ip 192.168.1.10/24
                host->setIpAddress(args[2]);
            }
            else if (router)
            {
                // Format: r1 set-ip 1 192.168.1.1/24
                if (args.size() < 4)
                {
                    err("Penggunaan: <NamaRouter> set-ip <port> <IP/CIDR>");
                    return true;
                }
                int port = stoi(args[2]);
                router->setInterfaceIp(port, args[3]);
            }
            else
            {
                err(args[0] + " tidak mendukung set-ip (hanya host/router).");
            }
        }
        else if (args.size() >= 3 && args[1] == "set-gw")
        {
            if (host)
            {
                host->setDefaultGateway(args[2]);
            }
            else
            {
                err("Hanya host yang mendukung set-gw.");
            }
        }
        else if (args.size() >= 3 && args[1] == "set-dns")
        {
            if (host)
            {
                g_hostDnsIps[host->getName()] = args[2];
                ok("DNS server untuk " + host->getName() + " diset ke " + args[2]);
            }
            else
            {
                err("Hanya host yang mendukung set-dns.");
            }
        }
        else if (args.size() >= 6 && args[1] == "ip" && args[2] == "route")
        {
            if (router)
            {
                string nextHop = (args[4] == "-") ? "" : args[4];
                int outPort = stoi(args[5]);
                router->addRoute(args[3], nextHop, outPort);
                ok("Route berhasil ditambahkan ke " + router->getName());
            }
            else
            {
                err("Perintah ip route hanya untuk router.");
            }
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
        else if (args.size() >= 2 && args[1] == "route")
        {
            if (!router)
            {
                err(args[0] + " bukan router");
                return true;
            }
            router->printRoutingTable();
        }
        else if (args.size() >= 2 && (args[1] == "mac" || args[1] == "mactable"))
        {
            if (!sw)
            {
                err(args[0] + " bukan switch");
                return true;
            }
            sw->printMACTable();
        }
        else if (args.size() >= 2 && args[1] == "mtu")
        {
            if (args.size() < 3)
            {
                err("Penggunaan: <node> mtu <port> <value>");
                return true;
            }
            int port = 1;
            int mtuVal = 1500;
            if (maybeNode)
            {
                if (host || router || sw)
                {
                    if (args.size() >= 4)
                    {
                        port = stoi(args[2]);
                        mtuVal = stoi(args[3]);
                    }
                    else if (args.size() >= 3)
                    {
                        mtuVal = stoi(args[2]);
                    }
                    Interface *ifc = maybeNode->getInterface(port);
                    if (!ifc)
                    {
                        err("Port " + to_string(port) + " tidak ditemukan di " + args[0]);
                        return true;
                    }
                    if (mtuVal < 68)
                    {
                        err("MTU minimum adalah 68 byte (IP minimum)");
                        return true;
                    }
                    if (mtuVal > 9000)
                    {
                        err("MTU maximum adalah 9000 byte (jumbo frame)");
                        return true;
                    }
                    ifc->setMtu(mtuVal);
                    ok("MTU port" + to_string(port) + " " + args[0] + " diset ke " + to_string(mtuVal));
                }
                else
                {
                    err(args[0] + " tidak mendukung mtu.");
                }
            }
        }
        else if (args.size() >= 3 && args[1] == "tcp_listen")
        {
            if (host)
            {
                uint16_t port = stoi(args[2]);
                host->tcpListen(port);
                ok("Host " + host->getName() + " sekarang LISTEN di port " + to_string(port));
            }
            else
            {
                err("Hanya host yang bisa listen port TCP.");
            }
        }
        else if (args.size() >= 4 && args[1] == "tcp_send")
        {
            if (host)
            {
                uint16_t srcPort = static_cast<uint16_t>(stoi(args[2]));
                string message = args[3];
                for (size_t i = 4; i < args.size(); ++i)
                {
                    message += " " + args[i];
                }

                host->tcpSend(srcPort, message);
                ok("Perintah tcp_send dieksekusi.");
            }
            else
            {
                err("Hanya host yang bisa melakukan tcp_send.");
            }
        }
        else if (args.size() >= 3 && args[1] == "tcp_close")
        {
            if (host)
            {
                uint16_t srcPort = static_cast<uint16_t>(stoi(args[2]));
                host->tcpClose(srcPort);
                ok("Connection on port " + to_string(srcPort) + " closed.");
            }
            else
            {
                err("Only hosts can tcp_close.");
            }
        }

        else if (args.size() >= 2 && args[1] == "http_server")
        {
            if (!host)
            {
                err("http_server hanya untuk host.");
                return true;
            }
            if (args.size() >= 3 && args[2] == "start")
            {
                string webRoot = (args.size() >= 4) ? args[3] : "";
                host->startHTTPServer(80, webRoot);
                ok("HTTP server started on port 80");
            }
            else if (args.size() >= 3 && args[2] == "stop")
            {
                // stop: close port 80
                auto &sockets = host->getTcpSockets();
                if (sockets.count(80))
                {
                    sockets[80]->close();
                    delete sockets[80];
                    sockets.erase(80);
                    ok("HTTP server di " + host->getName() + " dihentikan.");
                }
                else
                {
                    err("Tidak ada HTTP server yang berjalan di " + host->getName());
                }
            }
            
            else
            {
                err("Penggunaan: <host> http_server start [file] | stop");
            }
        }

        else if (args.size() >= 3 && args[1] == "http_get")
        {
            if (!host)
            {
                err("http_get hanya untuk host.");
                return true;
            }
            string url = args[2];

            string urlHost, urlPath;
            string s = url;
            if (s.size() >= 7 && s.substr(0, 7) == "http://")
                s = s.substr(7);
            size_t slashPos = s.find('/');
            if (slashPos == string::npos)
            {
                urlHost = s;
                urlPath = "/";
            }
            else
            {
                urlHost = s.substr(0, slashPos);
                urlPath = s.substr(slashPos);
            }

            bool isIp = true;
            for (char c : urlHost)
                if (!isdigit(c) && c != '.') { isIp = false; break; }

            if (!isIp)
            {
                auto dnsIt = g_hostDnsIps.find(host->getName());
                if (dnsIt == g_hostDnsIps.end() || dnsIt->second.empty())
                {
                    err("URL berisi domain tapi DNS belum diset. Gunakan: "
                        + host->getName() + " set-dns <ip>");
                    return true;
                }
                DNSClient dnsClient(host);
                string resolved = dnsClient.resolve(dnsIt->second, urlHost);
                if (resolved.empty())
                {
                    err("DNS resolve gagal untuk: " + urlHost);
                    return true;
                }
                info("DNS: " + urlHost + " -> " + resolved);
                urlHost = resolved;
            }

            // host->httpGet(urlHost, 80, urlPath);
            HTTPClient client(host); 
            string dnsServerIp = (args.size() >= 4) ? args[3] : ""; 
            string result = client.get(url, dnsServerIp); 
        }
        // ── DHCP SERVER ──────────────────────────────────────────────────
        // Format: <host> dhcp_server start [poolStart] [poolEnd] [gw] [prefix]
        else if (args.size() >= 2 && args[1] == "dhcp_server")
        {
            if (!host)
            {
                err("dhcp_server hanya untuk host.");
                return true;
            }
            if (args.size() >= 3 && args[2] == "start")
            {
                if (g_dhcpServers.count(host->getName()))
                {
                    info(host->getName() + " sudah ada DHCP server.");
                }
                else
                {
                    string poolStart = (args.size() >= 4) ? args[3] : "192.168.1.100";
                    string poolEnd = (args.size() >= 5) ? args[4] : "192.168.1.200";
                    string gateway = (args.size() >= 6) ? args[5] : host->getIpAddress();
                    uint8_t prefix = (args.size() >= 7) ? stoi(args[6]) : 24;
                    DHCPServer *srv = new DHCPServer(host, poolStart, poolEnd, gateway, prefix);
                    srv->start();
                    g_dhcpServers[host->getName()] = srv;
                    ok("DHCP server berjalan di " + host->getName());
                }
            }
            else if (args.size() >= 3 && args[2] == "stop")
            {
                if (g_dhcpServers.count(host->getName()))
                {
                    delete g_dhcpServers[host->getName()];
                    g_dhcpServers.erase(host->getName());
                    ok("DHCP server di " + host->getName() + " dihentikan.");
                }
                else
                {
                    err("Tidak ada DHCP server di " + host->getName());
                }
            }
            else
            {
                err("Penggunaan: <host> dhcp_server start [poolStart] [poolEnd] [gw] [prefix]");
            }
        }

        else if (args.size() >= 2 && args[1] == "dhcp_discover")
        {
            if (!host)
            {
                err("dhcp_discover hanya untuk host.");
                return true;
            }
            DHCPClient dhcp(host);
            bool result = dhcp.discover();
            if (result)
            {
                ok("DHCP selesai. IP baru: " + host->getIpAddress() +
                   "  GW: " + host->getDefaultGateway());
            }
            else
            {
                err("DHCP discover gagal — tidak ada response dari server.");
            }
        }
        else if (args.size() >= 2 && args[1] == "dns_server")
        {
            if (!host)
            {
                err("dns_server hanya untuk host.");
                return true;
            }
            if (args.size() >= 3 && args[2] == "start")
            {
                if (g_dnsServers.count(host->getName()))
                {
                    info(host->getName() + " sudah ada DNS server.");
                }
                else
                {
                    DNSServer *srv = new DNSServer(host);
                    srv->start();
                    g_dnsServers[host->getName()] = srv;
                    ok("DNS server berjalan di " + host->getName());
                }
            }
            else if (args.size() >= 5 && args[2] == "add")
            {
                if (!g_dnsServers.count(host->getName()))
                {
                    err("DNS server belum distart di " + host->getName() + ". Jalankan dns_server start dulu.");
                    return true;
                }
                g_dnsServers[host->getName()]->addRecord(args[3], args[4]);
                ok("DNS record: " + args[3] + " -> " + args[4]);
            }
            else if (args.size() >= 3 && args[2] == "records")
            {
                if (!g_dnsServers.count(host->getName()))
                {
                    err("Tidak ada DNS server di " + host->getName());
                    return true;
                }
                g_dnsServers[host->getName()]->printRecords();
            }
            else if (args.size() >= 3 && args[2] == "stop")
            {
                if (g_dnsServers.count(host->getName()))
                {
                    delete g_dnsServers[host->getName()];
                    g_dnsServers.erase(host->getName());
                    ok("DNS server di " + host->getName() + " dihentikan.");
                }
                else
                {
                    err("Tidak ada DNS server di " + host->getName());
                }
            }
            else
            {
                err("Penggunaan:");
                err("  <host> dns_server start");
                err("  <host> dns_server add <domain> <ip>");
                err("  <host> dns_server records");
                err("  <host> dns_server stop");
            }
        }
        else if (args.size() >= 3 && args[1] == "dns_resolve")
        {
            if (!host)
            {
                err("dns_resolve hanya untuk host.");
                return true;
            }
            auto dnsIt = g_hostDnsIps.find(host->getName());
            if (dnsIt == g_hostDnsIps.end() || dnsIt->second.empty())
            {
                err("DNS server belum diset. Gunakan: " + host->getName() + " set-dns <ip>");
                return true;
            }
            DNSClient dnsClient(host);
            string resolved = dnsClient.resolve(dnsIt->second, args[2]);
            if (!resolved.empty())
                ok("Resolve: " + args[2] + " -> " + resolved);
        }
        else if (args.size() >= 3 && args[1] == "acl")
        {
            if (!router)
            {
                err(args[0] + " bukan router.");
                return true;
            }
            if (args[2] == "show")
            {
                router->printACL();
            }
            else if (args[2] == "remove" && args.size() >= 4)
            {
                try
                {
                    router->removeACLRule(stoi(args[3]));
                    ok("ACL rule " + args[3] + " dihapus.");
                }
                catch (const exception &e)
                {
                    err(string("Gagal hapus rule: ") + e.what());
                }
            }
            else if (args[2] == "add" && args.size() >= 9)
            {
                ACLRule rule;
                rule.action = (args[3] == "deny") ? ACLRule::Action::DENY : ACLRule::Action::PERMIT;
                rule.direction = (args[4] == "out") ? ACLRule::Direction::OUT : ACLRule::Direction::IN;
                rule.port = stoi(args[5]);
                rule.srcNet = args[6];
                rule.dstNet = args[7];
                string proto = args[8];
                rule.protocol = (proto == "tcp") ? 6 : (proto == "udp") ? 17
                                                   : (proto == "icmp")  ? 1
                                                                        : 0;
                rule.srcPort = (args.size() > 9) ? stoi(args[9]) : -1;
                rule.dstPort = (args.size() > 10) ? stoi(args[10]) : -1;
                router->addACLRule(rule);
                ok("ACL rule ditambahkan: " + rule.toString());
            }
            else
            {
                err("Penggunaan:");
                err("  <router> acl show");
                err("  <router> acl add <permit|deny> <in|out> <port|-1> <srcNet> <dstNet> <proto> [sport] [dport]");
                err("  <router> acl remove <index>");
            }
        }
        else if (args.size() >= 6 && args[1] == "nat" && args[2] == "enable")
            {
                // Format: R1 nat enable <publicIp> <privatePort> <publicPort>
                // Contoh: R1 nat enable 203.0.113.1 1 2
                if (!router) {
                    err("Hanya router yang bisa enable NAT.");
                } else {
                    std::string publicIp = args[3];
                    int privatePort = stoi(args[4]);
                    int publicPort  = stoi(args[5]);
                    router->enableNAT(publicIp, privatePort, publicPort);
                    ok("NAT aktif di " + router->getName());
                }
            }
            else if (args.size() >= 3 && args[1] == "nat" && args[2] == "table")
            {
                if (!router) {
                    err("Hanya router yang bisa tampilkan NAT table.");
                } else {
                    router->printNATTable();
                }
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
        else if (cmd == "visualize")
        {
            // Bonus: Topology Visualizer
            if (args.size() >= 2 && args[1] == "dot")
            {
                string filename = (args.size() >= 3) ? args[2] : "topology.dot";
                if (TopologyVisualizer::exportDot(sim, filename))
                    ok("DOT file berhasil diekspor ke: " + filename);
                else
                    err("Gagal ekspor DOT file.");
            }
            else
            {
                TopologyVisualizer::printAscii(sim);
            }
        }
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
