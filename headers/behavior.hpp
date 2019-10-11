// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#ifndef HEADERS_BEHAVIOR_HPP_
#define HEADERS_BEHAVIOR_HPP_

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <unistd.h>

#include <stdexcept>
#include <thread>
#include <deque>
#include <mutex>

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

int SetupSocket(in_port_t port);
void GossipsCatching(int sd, size_t listenQueueLength, ThreadSaveGossipQueue& queue);
void UpdateTable(MemberTable& table, const std::deque<Gossip>& queue);
std::deque<Gossip> GenerateGossips(MemberTable& table, std::deque<Gossip>& queue); // TODO: complete it
void SendGossip(int sd, const Gossip& gossip);


#endif // HEADERS_BEHAVIOR_HPP_
