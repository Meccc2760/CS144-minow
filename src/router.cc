#include "router.hh"
#include "debug.hh"

#include <iostream>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  route_table_.push_back({route_prefix, prefix_length, next_hop, interface_num});
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  for(const std::shared_ptr<NetworkInterface>& networkinterface : interfaces_){
    std::queue<InternetDatagram>& incoming_datagrams = networkinterface->datagrams_received();

    while(!incoming_datagrams.empty()){
      InternetDatagram datagram = std::move(incoming_datagrams.front());
      incoming_datagrams.pop();
      // routing the datagram
      uint32_t dest_ip = datagram.header.dst;
      std::optional<size_t> outgoing_interface = std::nullopt;
      std::optional<Address> next_ip = std::nullopt;
      uint8_t prev_length = 0;

      for(const auto& route : route_table_){
        uint32_t mask = route.prefix_length == 0 ? 0 : 0xFFFFFFFF << (32 - route.prefix_length);
        if((route.route_prefix & mask) == (dest_ip & mask) && (!outgoing_interface.has_value() || route.prefix_length > prev_length)){
          outgoing_interface = route.interface_num;
          next_ip = route.next_hop;
          prev_length = route.prefix_length;
        }else{
          continue;
        }
      }
      // if no route matched, drop the datagram
      if(!outgoing_interface.has_value()){
        continue;
      }
      // handle the TTL
      if(datagram.header.ttl <= 1){
        continue;
      }else{
        datagram.header.ttl -= 1;
        datagram.header.compute_checksum();
      }
      // if matched and living, send it
      Address dest = next_ip.has_value() ? next_ip.value() : Address::from_ipv4_numeric(dest_ip);
      interface(outgoing_interface.value())->send_datagram(datagram, dest);
    }
  }
}
