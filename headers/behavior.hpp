// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#ifndef HEADERS_BEHAVIOR_HPP_
#define HEADERS_BEHAVIOR_HPP_

#include <stdexcept>
#include <thread>
#include <deque>
#include <iostream>
#include <mutex>

#include <boost/asio.hpp>

#include <types.hpp>
#include <buffer.hpp>

class ThreadSaveGossipQueue {
private:
    std::deque<Gossip> queue_;
    std::mutex mutex_;

public:
    void Push(const Gossip& gossip);
    std::deque<Gossip> Free();
};

boost::asio::ip::udp::socket SetupSocket(boost::asio::io_service& ioService, uint16_t port);
void GossipsCatching(boost::asio::ip::udp::socket& sock, ThreadSaveGossipQueue& queue);
std::deque<Conflict> UpdateTable(MemberTable& table, const std::deque<Gossip>& queue);
std::deque<Gossip> GenerateGossips(MemberTable& table, std::deque<Gossip>& queue); // TODO: complete it
void SendGossip(boost::asio::ip::udp::socket&, const Gossip& gossip);

void AppConnector(const MemberTable& table);

#endif // HEADERS_BEHAVIOR_HPP_
