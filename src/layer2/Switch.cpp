#include "../../include/layer2/Switch.hpp"
Switch::Switch(const string& name, int _numPorts): Node(name), numPorts(_numPorts){
    for(int i = 1; i <= numPorts; i++){
        portConfigs[i] = PortConfig();
    }
    cout << "[" << name << "] Swicth dibuat dengan " << numPorts << " numPorts\n";
}

void Switch::configurePort(int port, PortType type, uint16_t pvid, vector<int> allowedVlans){
    if (!isValidPort(port)) {
        cerr << "[" << getName() << "] ERROR: Port " << port << " tidak valid.\n";
        return;
    }

    portConfigs[port] = PortConfig(type, pvid, allowedVlans);
        cout << "[" << getName() << "] Port " << port
              << " -> " << (type == ACCESS ? "ACCESS" : "TRUNK")
              << " PVID=" << pvid;
 
    if (!allowedVlans.empty()) {
        cout << " allowed=[";
        for (size_t i = 0; i < allowedVlans.size(); ++i) {
            cout << allowedVlans[i];
            if (i + 1 < allowedVlans.size()) cout << ",";
        }
        cout << "]";
    }
    cout << "\n";
}
static string normalizeMac(const string& mac){
    string result = mac;
    transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}
void Switch::learnMAC(uint16_t vlanId, const string& mac, int port){
    string normalizedMac = normalizeMac(mac);

    auto key = make_pair(normalizedMac, vlanId);
    auto it = macTable.find(key);
    if (it == macTable.end()){
        cout << "[" << getName() << "] MAC Learn  : "
                    << mac << " VLAN " << vlanId << " -> Port " << port << "\n";
        } else if (it->second.port != port) {
            cout << "[" << getName() << "] MAC Update : "
                    << mac << " VLAN " << vlanId
                    << " Port " << it->second.port << " -> " << port << "\n";
        }  
        macTable[key] = MACEntry{ port, chrono::steady_clock::now() 
    };
}
int Switch::lookupMAC(uint16_t vlanId, const string& mac){
    string normalizedMac = normalizeMac(mac);
    auto key = make_pair(normalizedMac, vlanId);
    auto it = macTable.find(key);
    if (it == macTable.end()) return -1;
    else {
        return it->second.port;
    }
}
void Switch::printMACTable() const {
    cout << "\n[" << getName() << "] MAC Address Table:\n";
    cout << string(46, '-') << "\n";
    cout << left
              << setw(7)  << "VLAN"
              << setw(22) << "MAC Address"
              << setw(6)  << "Port" << "\n";
    cout << string(46, '-') << "\n";
 
    if (macTable.empty()) {
        cout << "(tabel kosong)\n";
    } else {
        for (const auto& entry : macTable) {
            const string& mac = entry.first.first;
            uint16_t vlan = entry.first.second;
            int port = entry.second.port;
            cout << left
                      << setw(7)  << vlan
                      << setw(22) << mac
                      << setw(6)  << port << "\n";
        }
    }
    cout << string(46, '-') << "\n\n";
}
 
void Switch::handleReceive(int ingressPort, const vector<uint8_t>& rawBytes){
    if(!isValidPort(ingressPort)){
        cerr << "[" << getName() << "] ERROR: Frame masuk di port tidak valid: " << ingressPort << "\n";
        return; 
    }

    EthernetFrame frame;
    try{
        frame.deserialize(rawBytes);
    }
    catch (const exception& e){
        cerr << "[" << getName() << "] ERROR: Gagal parse frame: "
                  << e.what() << "\n";
        return;
    }

    const PortConfig& cfg = portConfigs.at(ingressPort);
    uint16_t ingressVlan;
    if (cfg.type == ACCESS){
        ingressVlan = cfg.pvid;

        if (frame.vlanId != -1){
            // frame bertag masuk di access port, warning!
            cout << "[" << getName() << "] WARN: Tagged frame (VLAN " << frame.vlanId << ") di ACCESS port " << ingressPort << ", diabaikan, pakai PVID " << cfg.pvid << "\n";
        }
    }
    else{
        if (frame.vlanId != -1){
            ingressVlan = (uint16_t)frame.vlanId;
        }
        else{
            ingressVlan = cfg.pvid;
            cout << "[" << getName() << "] Untagged frame di TRUNK port " << ingressPort << ", pakai native VLAN " << ingressVlan << "\n";
        }
    }
    cout << "[" << getName() << "] Frame masuk Port " << ingressPort << " VLAN " << ingressVlan << " | " << frame.srcMac << " -> " << frame.destMac << "\n";

    if (normalizeMac(frame.srcMac) != "ff:ff:ff:ff:ff:ff") {
        learnMAC(ingressVlan, frame.srcMac, ingressPort);
    }

    if (normalizeMac(frame.destMac) == "ff:ff:ff:ff:ff:ff") {
        flood(ingressVlan, frame, ingressPort);
 
    } else {
        int targetPort = lookupMAC(ingressVlan, frame.destMac);
 
        if (targetPort != -1) {
            cout << "[" << getName() << "] MAC " << frame.destMac << " dikenal -> Forward ke Port " << targetPort << "\n";
            forwardFrame(targetPort, frame, ingressVlan);
        } else {
            // MAC tidak dikenal -> unknown unicast -> flood
            cout << "[" << getName() << "] MAC " << frame.destMac << " tidak dikenal -> Flooding\n";
            flood(ingressVlan, frame, ingressPort);
        }
    }
}

string Switch::getType() const{
    return "switch";
}

void Switch::forwardFrame(int port, EthernetFrame& frame, uint16_t ingressVlan){
    if (!isValidPort(port)) return;
    if (!isVlanAllowed(port, ingressVlan)) return;

    const PortConfig& cfg = portConfigs.at(port);
    if (cfg.type == ACCESS){
        frame.vlanId = -1;
    }
    else{
        frame.vlanId = (int)ingressVlan;
    }

    vector<uint8_t> rawBytes = frame.toBytes();
    Interface* iface = getInterface(port);
    if (iface && iface->isConnected()) {
        cout   << "[" << getName() << "] Forward -> Port " << port << " (" << (cfg.type == ACCESS ? "ACCESS" : "TRUNK") << " VLAN " << ingressVlan << ")\n";
        iface->send(rawBytes);
    }
        
}
void Switch::flood(uint16_t ingressVlan, EthernetFrame& frame, int excludePort){
    cout << "[" << getName() << "] Flooding VLAN " << ingressVlan << " dari Port " << excludePort << "\n";
 
    for (int p = 1; p <= numPorts; ++p) {
        if (p == excludePort) continue;
        forwardFrame(p, frame, ingressVlan);
    }
}
bool Switch::isVlanAllowed(int port, uint16_t vlanId) const{
    auto it = portConfigs.find(port);
    if (it == portConfigs.end()) return false;

    const PortConfig& cfg = it->second;
    // Kalau access port, cuman frame dengan vlan = pvidnya;
    if (cfg.type == ACCESS){
        return cfg.pvid == vlanId;
    }

    // trunk port, kalau empty boleh semua
    if (cfg.allowedVlans.empty()) return true;

    return find(cfg.allowedVlans.begin(), cfg.allowedVlans.end(), vlanId) != cfg.allowedVlans.end();
}
bool Switch::isValidPort(int port) const{
    return (port >= 1 && port <= numPorts);
}