#include <iostream>

#include "arp_message.hh"
#include "debug.hh"
#include "ethernet_frame.hh"
#include "exception.hh"
#include "helpers.hh"
#include "network_interface.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  uint32_t raw_ip = next_hop.ipv4_numeric();
  auto next_address = address_map_.find(raw_ip);
  // next_hop not in the mapping buffer, broadcast ARP request
  if (next_address == address_map_.end()){
    // if arp requested in past 5s, don't request again
    if (pending_datagram_.find(raw_ip) != pending_datagram_.end()){
      pending_datagram_[raw_ip].push(dgram);
      return;
    }
    // construct ARP message
    EthernetHeader header {ETHERNET_BROADCAST, this->ethernet_address_, EthernetHeader::TYPE_ARP};
    ARPMessage arp_message;
    arp_message.opcode = ARPMessage::OPCODE_REQUEST;
    arp_message.sender_ethernet_address = this->ethernet_address_;
    arp_message.sender_ip_address = this->ip_address_.ipv4_numeric();
    arp_message.target_ip_address = raw_ip;
    // serialize ARP message
    Serializer serializer;
    arp_message.serialize(serializer);

    //construct Ethernet frame and send
    EthernetFrame arp_request{header, serializer.finish()};
    transmit(arp_request);
    // manage dgram buffering, map dgram with next_hop
    pending_datagram_[raw_ip].push(dgram);
    address_pending_time_[raw_ip] = time_ms_;
  }else {
    // next_hop is known
    EthernetHeader header {next_address->second, this->ethernet_address_, EthernetHeader::TYPE_IPv4};
    //serialize IPv4 message
    Serializer serializer;
    dgram.serialize(serializer);
    EthernetFrame ipv4_message{header, serializer.finish()};
    transmit(ipv4_message);
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( EthernetFrame frame )
{
  if (frame.header.dst != ETHERNET_BROADCAST && frame.header.dst != this->ethernet_address_){
    // ignore the received frame
    return;
  }
  if (frame.header.type == EthernetHeader::TYPE_ARP){
    // parse the payload
    ARPMessage arp_message;
    if(!parse(arp_message, frame.payload)){
      return;
    }
    EthernetAddress peer_ethernet_address = arp_message.sender_ethernet_address;
    uint32_t peer_ip_address = arp_message.sender_ip_address;
    // insert ip-MAC mapping
    address_map_[peer_ip_address] = peer_ethernet_address;
    ethernet_memory_time_[peer_ip_address] = time_ms_;
    // if this MAC satisfies previous pending datagram, then send it
    auto pending_datagram = pending_datagram_.find(peer_ip_address);
    if (pending_datagram != pending_datagram_.end()){
      std::queue<InternetDatagram> pending_queue = std::move(pending_datagram->second);
      pending_datagram_.erase(peer_ip_address);
      address_pending_time_.erase(peer_ip_address);
      while (!pending_queue.empty()){
        send_datagram(pending_queue.front(), Address::from_ipv4_numeric(peer_ip_address));
        pending_queue.pop();
      }
    }

    if (arp_message.opcode == ARPMessage::OPCODE_REQUEST && arp_message.target_ip_address == this->ip_address_.ipv4_numeric()){
      // response to ARP request
      ARPMessage arp_reply;
      arp_reply.opcode = ARPMessage::OPCODE_REPLY;
      arp_reply.sender_ethernet_address = this->ethernet_address_;
      arp_reply.sender_ip_address = this->ip_address_.ipv4_numeric();
      arp_reply.target_ethernet_address = peer_ethernet_address;
      arp_reply.target_ip_address = peer_ip_address;
      // transmit the ARP reply
      EthernetHeader header {peer_ethernet_address, this->ethernet_address_, EthernetHeader::TYPE_ARP};
      Serializer serializer;
      arp_reply.serialize(serializer);
      EthernetFrame arp_response{header, serializer.finish()};
      transmit(arp_response);
    }
  }else if(frame.header.type == EthernetHeader::TYPE_IPv4){
    // parse the payload
    InternetDatagram received_dgram;
    if(parse(received_dgram, frame.payload)){
      datagrams_received_.push(received_dgram);
    }
  }else {
    return;
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  time_ms_ += ms_since_last_tick;
  // manage expired ARP requests
  for (auto it = address_pending_time_.begin(); it != address_pending_time_.end(); ){
    if (time_ms_ - it->second >= 5000){
      pending_datagram_.erase(it->first);
      it = address_pending_time_.erase(it);
    } else{
      ++it;
    }
  }
  // manage expired ip-MAC memory
  for (auto it = ethernet_memory_time_.begin(); it != ethernet_memory_time_.end(); ){
    if (time_ms_ - it->second >= 30000){
      address_map_.erase(it->first);
      it = ethernet_memory_time_.erase(it);
    } else{
      ++it;
    }
  }
}
