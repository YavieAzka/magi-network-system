#include "../../include/layer2/Switch.hpp"

Switch::Switch(const std::string &name, int _numPorts)
    : Node(name), numPorts(_numPorts)
{
    for (int i = 1; i <= numPorts; i++)
    {
        portConfigs[i] = PortConfig();
    }
    std::cout << "[" << name << "] Switch dibuat dengan " << numPorts << " port\n";
}

void Switch::configurePort(int port, PortType type, uint16_t pvid, std::vector<int> allowedVlans)
{
    if (!isValidPort(port))
    {
        std::cerr << "[" << getName() << "] ERROR: Port " << port << " tidak valid.\n";
        return;
    }

    portConfigs[port] = PortConfig(type, pvid, allowedVlans);
    std::cout << "[" << getName() << "] Port " << port
              << " -> " << (type == ACCESS ? "ACCESS" : "TRUNK")
              << " PVID=" << pvid;

    if (!allowedVlans.empty())
    {
        std::cout << " allowed=[";
        for (size_t i = 0; i < allowedVlans.size(); ++i)
        {
            std::cout << allowedVlans[i];
            if (i + 1 < allowedVlans.size())
                std::cout << ",";
        }
        std::cout << "]";
    }
    std::cout << "\n";
}

std::string Switch::normalizeMac(const std::string &mac)
{
    std::string result = mac;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

void Switch::learnMAC(uint16_t vlanId, const std::string &mac, int port)
{
    std::string normalizedMac = normalizeMac(mac);

    auto key = std::make_pair(normalizedMac, vlanId);
    auto it = macTable.find(key);
    if (it == macTable.end())
    {
        std::cout << "[" << getName() << "] MAC Learn  : "
                  << mac << " VLAN " << vlanId << " -> Port " << port << "\n";
    }
    else if (it->second.port != port)
    {
        std::cout << "[" << getName() << "] MAC Update : "
                  << mac << " VLAN " << vlanId
                  << " Port " << it->second.port << " -> " << port << "\n";
    }
    macTable[key] = MACEntry{port, std::chrono::steady_clock::now()};
}

int Switch::lookupMAC(uint16_t vlanId, const std::string &mac)
{
    std::string normalizedMac = normalizeMac(mac);
    auto key = std::make_pair(normalizedMac, vlanId);
    auto it = macTable.find(key);
    if (it == macTable.end())
        return -1;
    return it->second.port;
}

void Switch::printMACTable() const
{
    std::cout << "\n[" << getName() << "] MAC Address Table:\n";
    std::cout << std::string(46, '-') << "\n";
    std::cout << std::left
              << std::setw(7) << "VLAN"
              << std::setw(22) << "MAC Address"
              << std::setw(6) << "Port"
              << "\n";
    std::cout << std::string(46, '-') << "\n";

    if (macTable.empty())
    {
        std::cout << "(tabel kosong)\n";
    }
    else
    {
        for (const auto &entry : macTable)
        {
            const std::string &mac = entry.first.first;
            uint16_t vlan = entry.first.second;
            int port = entry.second.port;
            std::cout << std::left
                      << std::setw(7) << vlan
                      << std::setw(22) << mac
                      << std::setw(6) << port
                      << "\n";
        }
    }
    std::cout << std::string(46, '-') << "\n\n";
}

void Switch::handleReceive(Interface *inInterface, const std::vector<uint8_t> &rawBytes)
{
    int ingressPort = inInterface->getPortNumber();

    if (!isValidPort(ingressPort))
    {
        std::cerr << "[" << getName() << "] ERROR: Frame masuk di port tidak valid: " << ingressPort << "\n";
        return;
    }

    EthernetFrame frame;
    try
    {
        frame.deserialize(rawBytes);
    }
    catch (const std::exception &e)
    {
        std::cerr << "[" << getName() << "] ERROR: Gagal parse frame: "
                  << e.what() << "\n";
        return;
    }

    const PortConfig &cfg = portConfigs.at(ingressPort);
    uint16_t ingressVlan;
    if (cfg.type == ACCESS)
    {
        ingressVlan = cfg.pvid;
        if (frame.vlanId != -1)
        {
            std::cout << "[" << getName() << "] WARN: Tagged frame (VLAN " << frame.vlanId
                      << ") di ACCESS port " << ingressPort << ", diabaikan, pakai PVID " << cfg.pvid << "\n";
        }
    }
    else
    {
        if (frame.vlanId != -1)
        {
            ingressVlan = static_cast<uint16_t>(frame.vlanId);
        }
        else
        {
            ingressVlan = cfg.pvid;
            std::cout << "[" << getName() << "] Untagged frame di TRUNK port " << ingressPort
                      << ", pakai native VLAN " << ingressVlan << "\n";
        }
    }
    std::cout << "[" << getName() << "] Frame masuk Port " << ingressPort
              << " VLAN " << ingressVlan << " | " << frame.srcMac << " -> " << frame.destMac << "\n";

    if (normalizeMac(frame.srcMac) != "ff:ff:ff:ff:ff:ff")
    {
        learnMAC(ingressVlan, frame.srcMac, ingressPort);
    }

    if (normalizeMac(frame.destMac) == "ff:ff:ff:ff:ff:ff")
    {
        flood(ingressVlan, frame, ingressPort);
    }
    else
    {
        int targetPort = lookupMAC(ingressVlan, frame.destMac);
        if (targetPort != -1)
        {
            std::cout << "[" << getName() << "] MAC " << frame.destMac << " dikenal -> Forward ke Port " << targetPort << "\n";
            forwardFrame(targetPort, frame, ingressVlan);
        }
        else
        {
            std::cout << "[" << getName() << "] MAC " << frame.destMac << " tidak dikenal -> Flooding\n";
            flood(ingressVlan, frame, ingressPort);
        }
    }
}

std::string Switch::getType() const
{
    return "switch";
}

void Switch::forwardFrame(int port, EthernetFrame &frame, uint16_t ingressVlan)
{
    if (!isValidPort(port))
        return;
    if (!isVlanAllowed(port, ingressVlan))
        return;

    const PortConfig &cfg = portConfigs.at(port);
    if (cfg.type == ACCESS)
    {
        frame.vlanId = -1;
    }
    else
    {
        frame.vlanId = static_cast<int>(ingressVlan);
    }

    std::vector<uint8_t> rawBytes = frame.toBytes();
    Interface *iface = getInterface(port);
    if (iface && iface->isConnected())
    {
        std::cout << "[" << getName() << "] Forward -> Port " << port
                  << " (" << (cfg.type == ACCESS ? "ACCESS" : "TRUNK")
                  << " VLAN " << ingressVlan << ")\n";
        iface->send(rawBytes);
    }
}

void Switch::flood(uint16_t ingressVlan, EthernetFrame &frame, int excludePort)
{
    std::cout << "[" << getName() << "] Flooding VLAN " << ingressVlan
              << " dari Port " << excludePort << "\n";

    for (int p = 1; p <= numPorts; ++p)
    {
        if (p == excludePort)
            continue;
        forwardFrame(p, frame, ingressVlan);
    }
}

bool Switch::isVlanAllowed(int port, uint16_t vlanId) const
{
    auto it = portConfigs.find(port);
    if (it == portConfigs.end())
        return false;

    const PortConfig &cfg = it->second;
    if (cfg.type == ACCESS)
    {
        return cfg.pvid == vlanId;
    }

    if (cfg.allowedVlans.empty())
        return true;

    return std::find(cfg.allowedVlans.begin(), cfg.allowedVlans.end(), vlanId) != cfg.allowedVlans.end();
}

bool Switch::isValidPort(int port) const
{
    return (port >= 1 && port <= numPorts);
}