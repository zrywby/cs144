#pragma once

#include <memory>
#include <optional>
#include <queue>
#include <vector>
#include "exception.hh"
#include "network_interface.hh"

// A wrapper for NetworkInterface that makes the host-side
// interface asynchronous: instead of returning received datagrams
// immediately (from the `recv_frame` method), it stores them for
// later retrieval. Otherwise, behaves identically to the underlying
// implementation of NetworkInterface.
class AsyncNetworkInterface : public NetworkInterface {
    std::queue<InternetDatagram> datagrams_in_{};  // 存储接收到的数据报

public:
    using NetworkInterface::NetworkInterface;

    // Construct from a NetworkInterface
    explicit AsyncNetworkInterface(NetworkInterface &&interface) : NetworkInterface(interface) {}

    // \brief Receives an Ethernet frame and responds appropriately.
    // - If type is IPv4, pushes to the `datagrams_in_` queue for later retrieval by the owner.
    // - If type is ARP request, learn a mapping from the "sender" fields, and send an ARP reply.
    // - If type is ARP reply, learn a mapping from the "target" fields.
    //
    // \param[in] frame the incoming Ethernet frame
    void recv_frame(const EthernetFrame &frame) {
        auto optional_dgram = NetworkInterface::recv_frame(frame);
        if (optional_dgram.has_value()) {
            datagrams_in_.push(std::move(optional_dgram.value()));
        }
    }

    // Access queue of Internet datagrams that have been received
    std::optional<InternetDatagram> maybe_receive() {
        if (datagrams_in_.empty()) {
            return {};
        }
        InternetDatagram datagram = std::move(datagrams_in_.front());
        datagrams_in_.pop();
        return datagram;
    }
};

// A router that has multiple network interfaces and performs
// longest-prefix-match routing between them.
class Router {
    // The router's collection of network interfaces
    std::vector<AsyncNetworkInterface> interfaces_;
    
    // 数据转发表项
    struct Route {
        uint32_t route_prefix;
        uint8_t prefix_length;
        std::optional<Address> next_hop;
        size_t interface_num;
    };

    std::vector<Route> routes_;

public:
    // Add an interface to the router
    // \param[in] interface an already-constructed network interface
    // \returns The index of the interface after it has been added to the router
    size_t add_interface(AsyncNetworkInterface &&interface) {
        interfaces_.push_back(std::move(interface));
        return interfaces_.size() - 1;
    }

    // Access an interface by index
    AsyncNetworkInterface &interface(size_t N) {
        return interfaces_.at(N);
    }

    // Add a route (a forwarding rule)
    void add_route(uint32_t route_prefix,
                   uint8_t prefix_length,
                   std::optional<Address> next_hop,
                   size_t interface_num) {
        routes_.push_back({route_prefix, prefix_length, next_hop, interface_num});
    }

    // Route packets between the interfaces.
    void route() {
        for (auto &interface : interfaces_) {
            // 从接口接收数据包
            while (auto dgram = interface.maybe_receive()) {
                InternetDatagram &datagram = dgram.value();
                uint32_t dest_ip = datagram.header().dst;

                // 查找与目的地址匹配的最长前缀
                size_t best_match_index = -1;
                uint8_t best_match_len = 0;

                for (size_t i = 0; i < routes_.size(); ++i) {
                    const Route &route = routes_[i];
                    uint32_t prefix_mask = (1 << (32 - route.prefix_length)) - 1;
                    uint32_t masked_dest = dest_ip & prefix_mask;

                    if (masked_dest == (route.route_prefix & prefix_mask) && route.prefix_length > best_match_len) {
                        best_match_len = route.prefix_length;
                        best_match_index = i;
                    }
                }

                // 如果找到匹配的路由规则
                if (best_match_index != size_t(-1)) {
                    const Route &route = routes_[best_match_index];

                    if (route.next_hop.has_value()) {
                        // 如果有下一跳，进行相应的处理
                        // 这里可以是直接路由到下一跳，或者直接转发数据包到目标接口
                    }

                    // 将数据包转发到合适的接口
                    interface(route.interface_num).send_datagram(std::move(datagram));
                }
            }
        }
    }
};

