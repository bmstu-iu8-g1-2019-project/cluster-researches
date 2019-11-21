// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#ifndef INCLUDE_BEHAVIOR_HPP_
#define INCLUDE_BEHAVIOR_HPP_

#include <gossip.hpp>
#include <queue.hpp>

typedef boost::asio::ip::udp::socket socket_type;


void CatchGossips(socket_type& socket, ThreadSaveQueue<PullGossip>& pings, ThreadSaveQueue<PullGossip>& acks);

void SendGossip(socket_type& socket, const )

#endif // INCLUDE_BEHAVIOR_HPP_
