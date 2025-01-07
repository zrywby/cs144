#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix,
                       const uint8_t prefix_length,
                       const optional<Address> next_hop,
                       const size_t interface_num) {
    cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/"
         << static_cast<int>(prefix_length) << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)")
         << " on interface " << interface_num << "\n";

    routes.push_back({route_prefix, prefix_length, next_hop, interface_num});
}

void Router::route() {
    for (auto &interface : interfaces_) {
        while (true) {
            // 获取当前接口的数据报
            auto ipdata = interface.maybe_receive();
            if (!ipdata.has_value()) {
                break;  // 如果当前接口没有数据包，跳出循环
            }

            auto ipData = *ipdata;  // 拿到接收到的数据包

            if (ipData.header.ttl == 0) {
                continue;  // 如果TTL为0，丢弃数据包
            }

            ipData.header.ttl--;  // 减少TTL
            ipData.header.compute_checksum();  // 重新计算校验和

            int maxx = -1;
            size_t n = routes.size();
            
            // 遍历路由表，选择匹配的最长前缀
            for (size_t i = 0; i < n; ++i) {
                auto &route = routes[i];
                uint8_t len = 32 - route.prefix_length;

                // 将地址与路由前缀进行比较，进行最长前缀匹配
                if ((route.route_prefix >> len) == (ipData.header.dst >> len)) {
                    if (maxx == -1 || routes[maxx].prefix_length < route.prefix_length) {
                        maxx = i;  // 记录匹配的路由
                    }
                }
            }

            if (maxx == -1) {
                continue;  // 如果没有找到匹配的路由，丢弃数据包
            }

            auto nextHop = routes[maxx].next_hop;
            if (!nextHop.has_value()) {
                // 如果路由表没有下一跳，直接将数据包发送到接口
                this->interface(routes[maxx].interface_num)
                    .send_datagram(ipData, Address::from_ipv4_numeric(ipData.header.dst));
            } else {
                // 如果有下一跳，将数据包转发到下一跳
                this->interface(routes[maxx].interface_num).send_datagram(ipData, *nextHop);
            }
        }
    }
}

