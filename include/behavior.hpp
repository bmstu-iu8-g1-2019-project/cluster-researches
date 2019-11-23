// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#ifndef INCLUDE_BEHAVIOR_HPP_
#define INCLUDE_BEHAVIOR_HPP_

#include <gossip.hpp>
#include <queue.hpp>

typedef boost::asio::io_service io_service;
typedef boost::asio::ip::udp::socket socket_type;
typedef boost::asio::ip::udp::endpoint endpoint;

class Socket {
private:
    socket_type _socket;
    std::vector<byte_t> _buffer;

private:
    void _SendProtoGossip(const Proto::Gossip& protoGossip);

    void _GossipCatching(ThreadSaveQueue<PullGossip>& pings, ThreadSaveQueue<PullGossip>& acks);

public:
    explicit Socket(io_service& ioService, uint16_t port, size_t bufferSize);

    void RunGossipCatching(ThreadSaveQueue<PullGossip>& pings, ThreadSaveQueue<PullGossip>& acks);

    void SendAcks(const Table& table, std::vector<size_t>&& destIndexes);

    void Spread(Table& table, size_t destsNum);
};

std::vector<size_t> GetIndexesToGossipOwners(const Table& table, const std::vector<PullGossip>& gossips);

std::vector<size_t> UpdateTable(Table& table, std::vector<PullGossip>& gossips);

void SetDest(Proto::Gossip& protoGossip, const Member& dest);

void AcceptAcks(Table& table, std::vector<PullGossip>&& acks);

void DetectFailures(Table& table, size_t msTimeout, size_t failuresBeforeDead);

#endif // INCLUDE_BEHAVIOR_HPP_
