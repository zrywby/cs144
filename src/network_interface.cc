#include <iostream>
#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

using namespace std;

NetworkInterface::NetworkInterface(string_view name,
                                   shared_ptr<OutputPort> port,
                                   const EthernetAddress& ethernet_address,
                                   const Address& ip_address)
    : name_(name),
      port_(notnull("OutputPort", move(port))),
      ethernet_address_(ethernet_address),
      ip_address_(ip_address),
      ipToEthernetMap(),
      recent_arp_requests(),
      waiting_datagrams_()
{
    cerr << "DEBUG: 网络接口具有以太网地址 " << to_string(ethernet_address) << " 和 IP 地址 " << ip_address.ip() << "\n";
}

void NetworkInterface::send_datagram(const InternetDatagram& dgram, const Address& next_hop)
{   
    uint32_t next_hop_ip = next_hop.ipv4_numeric();

    EthernetFrame frame;    
    if(create_IPv4frame(dgram, next_hop_ip, frame)) {
        transmit(frame);
        return ;
    }

    if (((currentTime - recent_arp_requests[next_hop_ip]) >= 5*1000) || 
        (!recent_arp_requests[next_hop_ip] && !currentTime)) {
        send_arp_request(next_hop_ip);
        recent_arp_requests[next_hop_ip] = currentTime;
    }

    waiting_datagrams_[next_hop_ip] = dgram;
}

void NetworkInterface::recv_frame(const EthernetFrame& frame)
{   
    if(frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST){
            return ;
    }

    if(frame.header.type == frame.header.TYPE_IPv4){
        InternetDatagram dgram;
        if (parse(dgram, frame.payload)) {
            datagrams_received_.push(dgram);
        }
    }            

    if(frame.header.type == frame.header.TYPE_ARP){
        ARPMessage arp_msg;
        if (parse(arp_msg, frame.payload)) {
            ipToEthernetMap[arp_msg.sender_ip_address] = arp_msg.sender_ethernet_address;
            recent_arp_requests[arp_msg.sender_ip_address] = currentTime;
            if (reply_arp_request(arp_msg)) {
                return ;
            }
            if (create_ARPframe(arp_msg)) {
                return ;
            }
        }
   }
}

void NetworkInterface::tick(const size_t ms_since_last_tick)
{   
    currentTime += ms_since_last_tick;
    for (auto it = recent_arp_requests.begin(); it != recent_arp_requests.end(); ) {
        if (it->second + 30*1000 <= currentTime) {
            ipToEthernetMap.erase(it->first);
            it = recent_arp_requests.erase(it);
            currentTime = 0;
        } else {
            ++it;
        }
    }
}

bool NetworkInterface::create_IPv4frame(const InternetDatagram& dgram, uint32_t next_hop, EthernetFrame& frame)
{
    auto it = ipToEthernetMap.find(next_hop);
    if (it != ipToEthernetMap.end()) {
        frame.header.type = EthernetHeader::TYPE_IPv4;
        frame.header.dst = it->second;
        frame.header.src = ethernet_address_;
        frame.payload = serialize(dgram);
        return true;
    }
    return false;
}

bool NetworkInterface::create_ARPframe(const ARPMessage & arp_msg)
{
    if(arp_msg.opcode == ARPMessage::OPCODE_REPLY && arp_msg.target_ip_address == ip_address_.ipv4_numeric()){
        EthernetFrame frame_;
        auto it = waiting_datagrams_.find(arp_msg.sender_ip_address);       
        if (it != waiting_datagrams_.end()) {
            frame_.header.type = EthernetHeader::TYPE_IPv4;
            frame_.header.src = ethernet_address_;
            frame_.header.dst = arp_msg.sender_ethernet_address;
            frame_.payload = serialize(it->second);
            transmit(frame_);   
            waiting_datagrams_.erase(it); 
        }
        return true;
    } else {
        return false;
    }
}

void NetworkInterface::send_arp_request(uint32_t next_hop_ip) {
    ARPMessage arp_request;
    arp_request.opcode = ARPMessage::OPCODE_REQUEST;
    arp_request.sender_ethernet_address = ethernet_address_;
    arp_request.sender_ip_address = ip_address_.ipv4_numeric();
    arp_request.target_ethernet_address = EthernetAddress{0, 0, 0, 0, 0, 0};
    arp_request.target_ip_address = next_hop_ip;

    EthernetFrame frame;
    frame.header.src = ethernet_address_;
    frame.header.dst = ETHERNET_BROADCAST;
    frame.header.type = EthernetHeader::TYPE_ARP;

    Serializer serializer;
    arp_request.serialize(serializer);
    frame.payload = serializer.output();

    transmit(frame);
}

bool NetworkInterface::reply_arp_request(const ARPMessage & arp_msg){
    if(arp_msg.opcode == ARPMessage::OPCODE_REQUEST && arp_msg.target_ip_address == ip_address_.ipv4_numeric()){
        ARPMessage arp_reply;
        arp_reply.opcode = ARPMessage::OPCODE_REPLY;
        arp_reply.sender_ethernet_address = ethernet_address_;
        arp_reply.sender_ip_address = ip_address_.ipv4_numeric();
        arp_reply.target_ethernet_address = arp_msg.sender_ethernet_address;
        arp_reply.target_ip_address = arp_msg.sender_ip_address;

        EthernetFrame reply_frame;
        reply_frame.header.type = EthernetHeader::TYPE_ARP;
        reply_frame.header.src = ethernet_address_;
        reply_frame.header.dst = arp_msg.sender_ethernet_address;
        reply_frame.payload = serialize(arp_reply);  
        transmit(reply_frame);        
        return true;   
    } else {
        return false;
    }
}

void NetworkInterface::prints(){
    cout << "当前所有映射 " << endl;
    for (auto it = ipToEthernetMap.begin(); it != ipToEthernetMap.end(); it++) {
        cout << "IP序列:  " << it->first << endl;
        cout << "MAC地址: " << to_string(it->second) << endl;
        cout << "对应时间" << recent_arp_requests[it->first] << endl;
        cout << "当前时间" << currentTime << endl;
    }
}

