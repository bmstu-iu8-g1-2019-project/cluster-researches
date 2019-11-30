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
    port_t _dockerPort;

    ip_v4 _ip;
    port_t _port;

    size_t _bufferSize;

    // количество последних и случайных событий в слухе
    size_t _latestPartSize;
    size_t _randomPartSize;

    // количество одновременно обрабатываемых пингов
    size_t _pingProcNum;
    // количество одновременно обрабатываемых акков
    size_t _ackProcNum;

    // количество нод, которым мы отправляем слух
    size_t _spreadDestsNum;
    // частота отправки слухов
    milliseconds _spreadRepetition;

    milliseconds _fdTimeout;
    // частота отправки failure detection
    milliseconds _fdRepetition;
    // количество возможных неполученых ответов на пинг
    size_t _fdFailuresBeforeDead;

    ip_v4 _observerIP;
    port_t _observerPort;
    milliseconds _observeRepetition;

    milliseconds _logRepetition;

    // начальная конфигурация таблицы
    std::unique_ptr<Table> _pTable;

public:
    explicit Config(char* configPath);

    port_t DockerPort() const;

    const ip_v4& IP() const;
    port_t Port() const;

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

    const ip_v4& ObserverIP() const;
    port_t ObserverPort() const;
    milliseconds ObserveRepetition() const;

    milliseconds LogRepetition() const;

    Table&& MoveTable();
};

class Socket {
private:
    socket_type _socket;

    // IBuffer (receiving)
    std::vector<byte_t> _IBuffer;

private:
    // отправка ЗАПОЛНЕННОГО `Proto::Gossip`
    void _SendProtoGossip(const Proto::Gossip& protoGossip);

    // бесконечная функций, ловящая слухи и складывающая их в соответствующую очередь
    void _GossipCatching(ThreadSaveQueue<PullGossip>& pings, ThreadSaveQueue<PullGossip>& acks);

    void _JSONCatching(ThreadSaveQueue<nlohmann::json>& jsons);

public:
    explicit Socket(io_service& ioService, uint16_t port, size_t bufferSize);

    // запускает `_GossipCatching()` в отдельном потоке
    void RunGossipCatching(ThreadSaveQueue<PullGossip>& pings, ThreadSaveQueue<PullGossip>& acks);

    void RunJSONCatching(ThreadSaveQueue<nlohmann::json>& jsons);

    // генерирует акки и отправляет нодам из `destIndexes`
    void SendAcks(Table& table);

    // генерирует слух и расспространяет его `destsNum` нодам (если столько есть)
    void Spread(Table& table, size_t destsNum);

    // отправляет UDP-дейтаграмму `observer`
    void NotifyObserver(const ip_v4& observerIP, port_t observerPort, const Table& table);
};

std::vector<size_t> GetIndexesToGossipOwners(const Table& table, const std::vector<PullGossip>& gossips);

void UpdateTable(Table& table, std::vector<PullGossip>& gossips);

void SetDest(Proto::Gossip& protoGossip, const Member& dest);

// вносит в таблицу информацию о том, что пришел акк
void AcceptAcks(Table& table, std::vector<PullGossip>&& acks);

#endif // INCLUDE_BEHAVIOR_HPP_
