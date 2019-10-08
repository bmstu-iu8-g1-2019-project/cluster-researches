// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#ifndef HEADERS_GOSSIPING_HPP_
#define HEADERS_GOSSIPING_HPP_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include <types.hpp>
#include <membertable.hpp>
#include <behavior.hpp>

void FailureDetection(int sd, MemberTable& table, std::deque<Gossip>& discoveredGossips);

void Distribution(int sd, MemberTable& table, std::deque<Gossip>& discoveredGossips);

#endif // HEADERS_GOSSIPING_HPP_
