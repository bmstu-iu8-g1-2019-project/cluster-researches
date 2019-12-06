// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#ifndef INCLUDE_CONFIG_HPP_
#define INCLUDE_CONFIG_HPP_

#include <fstream>

#include <table.hpp>

class Config {
private:
    bool _containerization;

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
    size_t _observeNum;
    milliseconds _observerFDTimeout;

    milliseconds _logRepetition;

    // начальная конфигурация таблицы
    std::unique_ptr<Table> _pTable;

public:
    explicit Config(char* configPath, char* strIP, char* strPort);

    bool Containerization() const;

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
    size_t ObserveNum() const;
    milliseconds ObserverFDTimout() const;

    milliseconds LogRepetition() const;

    Table&& MoveTable();
};

#endif // INCLUDE_CONFIG_HPP_
