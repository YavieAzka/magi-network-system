# Milestone 2 WBS 

## 5 people wbs divison 

P1 
| task | deliverables | 
| ---- | ------------ | 
| NetUtils | stringToIP(), ipToString(), etc. | 
| refactor interface, MAC from string to vector<uin8_t> | self explanatory | 
| update addInterface() | accept vector instead of string | 
| generateMac() | return vector instead of string | 
| update cmakelist | self explanatory | 

P2
| task | deliverables | 
| ---- | ------------ | 
| vlanId to ethernetFrame | hpp file change | 
| EthernetFrame::toBytes() | if vlanId >= 0; insert TPID 0x8100 + TCI bytes | 
| EthernetFrame::deserialize() | extract VID from TCI, set vlan Id | 
| FCS computation | calculate_and_set_fcs() and verify_fcs() | 
| add convenience constructors | ethernetFrame with vlanId parameter | 



P3
| task | deliverables | 
| ---- | ------------ | 
| create ARPCache | do your research ~~~ | 
| ARP request builder | ARPPacket with opcode, sender_mac, sender_ip, target_mac, target_ip | 
| ARP reply builder | ARPPacket with opcode for reply | 
| ARP receive handler | if request -> send reply, etc. | 


P4
| task | deliverables | 
| ---- | ------------ | 
| Switch class | extension of Node | 
| MAC learning | update macTable entry for all switch X -> learn MAC xx on port N VLAN V | 
| MAV lookup | lookup dst_mac | 
| forwarding | port forwarding | 
| flooding | | 
| handleReceive() | deserialize -> ethernetframe; determine ingress VLAN; learn src MAC; if broadcast -> flood, else lookup -> forward or floow | 
| access/trunk port | assign pvid, etc. | 
| getType() | ya gitu lah | 

P5
| task | deliverables | 
| ---- | ------------ | 
| host class | extension of node | 
| sendLayer3Packet | kayak praktikum | 
| send ARP request | kayak praktikum | 
| sendARP reply | kayak praktikum | 
| handleReceive | deserialize -> ethernetFrame, if ethertype=0x0806 then arp processing, else if ethertype=0x0800 -> print received IP packet info | 
| CLI for createNode() switch, host, . build from config() too | construcotrsssS|
| integration | | 

remember to write unit testing ~~~. 


