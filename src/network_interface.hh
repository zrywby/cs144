#pragma once
#include <chrono>
#include <unordered_map>
#include <queue>
#include <memory>
#include "address.hh"
#include "ethernet_frame.hh"
#include "ipv4_datagram.hh"

class NetworkInterface
{
public:
  class OutputPort
  {
  public:
    virtual void transmit(const NetworkInterface& sender, const EthernetFrame& frame) = 0;
    virtual ~OutputPort() = default;
  };

  NetworkInterface(std::string_view name,
                   std::shared_ptr<OutputPort> port,
                   const EthernetAddress& ethernet_address,
                   const Address& ip_address);

  void send_datagram(const InternetDatagram& dgram, const Address& next_hop);
  void recv_frame(const EthernetFrame& frame);
  void tick(size_t ms_since_last_tick);

  const std::string& name() const { return name_; }
  const OutputPort& output() const { return *port_; }
  OutputPort& output() { return *port_; }
  std::queue<InternetDatagram>& datagrams_received() { return datagrams_received_; }

private:
  bool create_IPv4frame(const InternetDatagram& dgram, uint32_t next_hop, EthernetFrame& frame);
  bool create_ARPframe(const ARPMessage & arp_msg);
  void send_arp_request(uint32_t next_hop_ip);
  bool reply_arp_request(const ARPMessage & arp_msg);
  void prints();

  std::string name_;
  std::shared_ptr<OutputPort> port_;
  void transmit(const EthernetFrame& frame) const { port_->transmit(*this, frame); }
  EthernetAddress ethernet_address_;
  Address ip_address_;
  std::queue<InternetDatagram> datagrams_received_;
  std::unordered_map<uint32_t,EthernetAddress> ipToEthernetMap;
  std::unordered_map<uint32_t,uint64_t> recent_arp_requests;
  std::unordered_map<uint32_t,InternetDatagram> waiting_datagrams_;
  uint64_t currentTime = 0;
};

