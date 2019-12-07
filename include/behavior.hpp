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


class Socket {
private:
    socket_type _socket;

    // IBuffer (receiving)
    std::vector<byte_t> _IBuffer;

private:
    // бесконечная функций, ловящая слухи и складывающая их в соответствующую очередь
    void _GossipCatching(ThreadSaveQueue<PullGossip>& pings,
                         ThreadSaveQueue<PullGossip>& acks,
                         ThreadSaveQueue<PullGossip>& observerCommands,
                         MemberAddr observerAddress);

    void _Observing(ThreadSaveQueue<PullGossip>& pulls);

    void _Killer(Member observer);

public:
    explicit Socket(io_service& ioService, uint16_t port, size_t bufferSize);

    // запускает `_GossipCatching()` в отдельном потоке
    void RunGossipCatching(ThreadSaveQueue<PullGossip>& pings,
                           ThreadSaveQueue<PullGossip>& acks,
                           ThreadSaveQueue<PullGossip>& observerCommands,
                           MemberAddr observerAddress);

    void RunObserving(ThreadSaveQueue<PullGossip>& pulls);

    void RunKiller(Member observer);

    // отправка ЗАПОЛНЕННОГО `Proto::Gossip`
    void SendProtoGossip(const Proto::Gossip& protoGossip);

    // генерирует акки и отправляет нодам из `destIndexes`
    void SendAcks(Table& table);

    // генерирует слух и расспространяет его `destsNum` нодам (если столько есть)
    void Spread(Table& table, size_t destsNum);

    // отправляет UDP-дейтаграмму `observer`
    void NotifyObserver(const ip_v4& observerIP, port_t observerPort, const Table& table);
};

void ExecuteObserverCommands(ThreadSaveQueue<PullGossip>& observerCommands);

bool UpdateTable(Table& table, std::vector<PullGossip>& gossips);

void SetDest(Proto::Gossip& protoGossip, const Member& dest);

// вносит в таблицу информацию о том, что пришел акк
void AcceptAcks(Table& table, std::vector<PullGossip>&& acks);

#endif // INCLUDE_BEHAVIOR_HPP_
