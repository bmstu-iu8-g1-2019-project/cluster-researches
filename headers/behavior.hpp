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

Variables SetupConfig(const std::string& configPath);
boost::asio::ip::udp::socket SetupSocket(boost::asio::io_service& ioService, uint16_t port);
void GossipsCatching(boost::asio::ip::udp::socket& sock, ThreadSaveGossipQueue& queue);
void MemberPinging(boost::asio::ip::udp::socket& sock, MemberTable& table);
std::deque<Member> UpdateTable(MemberTable& table, const std::deque<Gossip>& queue);
Gossip GenerateGossip(MemberTable& table); // TODO: complete it
void SendGossip(boost::asio::ip::udp::socket&, const Gossip& gossip);
void SpreadGossip(boost::asio::ip::udp::socket& sock, const MemberTable& table, Gossip& gossip, size_t destNum);
/*
void ResolveConflicts(const std::dequ);
*/

void AppConnector(const MemberTable& table);

#endif // HEADERS_BEHAVIOR_HPP_
