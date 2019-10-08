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
#include <queue>

#include <types.hpp>

constexpr size_t UDPHeaderLength = 8;

using GossipQueue = std::queue<Gossip>;

int SetupSocket(in_port_t port);
void GossipsCatching(int sd, size_t listenQueueLength, GossipQueue& queue);
void UpdateTable(MemberTable& table, GossipQueue& queue);
std::vector<Gossip> GenerateGossips(MemberTable& table);
void SendGossip(int sd, const Gossip& gossip);


#endif // HEADERS_BEHAVIOR_HPP_
