// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#ifndef INCLUDE_BEHAVIOR_HPP_
#define INCLUDE_BEHAVIOR_HPP_

#include <fstream>

#include <nlohmann/json.hpp>

#include <gossip.hpp>
#include <queue.hpp>

typedef boost::asio::io_service io_service;
typedef boost::asio::ip::udp::socket socket_type;
typedef boost::asio::ip::udp::endpoint endpoint;

class Config {
private:
    ip_v4 _ip;
    uint16_t _port;

    size_t _bufferSize;

    size_t _latestPartSize;
    size_t _randomPartSize;

    size_t _pingProcNum;
    size_t _ackProcNum;

    size_t _spreadDestsNum;
    milliseconds _spreadRepetition;

    milliseconds _fdTimeout;
    milliseconds _fdRepetition;
    size_t _fdFailuresBeforeDead;

    std::unique_ptr<Table> _pTable;

public:
    Config(const std::string& path);

    const ip_v4& IP() const;
    uint16_t Port() const;

    size_t BufferSize() const;

    size_t LatestPartSize() const;
    size_t RandomPartSize() const;

    size_t PingProcessingNum() const;
    size_t AckProcessingNum() const;

    size_t SpreadDestinationNum() const;
    milliseconds SpreadRepetition() const;

    milliseconds FDTimout() const;
    milliseconds FDRepetition() const;

    size_t FailuresBeforeDead() const;

    Table&& MoveTable();
};

class Socket {
private:
    socket_type _socket;
    std::vector<byte_t> _IBuffer;
    std::vector<byte_t> _OBuffer;

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

#endif // INCLUDE_BEHAVIOR_HPP_
