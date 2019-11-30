// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <behavior.hpp>

Config::Config(char* configPath) {
    std::ifstream file{configPath};

    if (!file.is_open()) {
        throw std::runtime_error{"Unable to open config file"};
    }

    auto json = nlohmann::json::parse(file);

    _dockerPort = json["docker-port"];

    _ip = ip_v4::from_string(json["address"]["ip"]);
    _port = json["address"]["port"];

    _bufferSize = json["buffer-size"];

    _latestPartSize = json["latest-part-size"];
    _randomPartSize = json["random-part-size"];

    _pingProcNum = json["ping-processing-num"];
    _ackProcNum = json["ack-processing-num"];

    _spreadDestsNum = json["spread-destination-num"];
    _spreadRepetition = milliseconds{json["spread-repetition"]};

    _fdTimeout = milliseconds{json["fd-timeout"]};
    _fdRepetition = milliseconds{json["fd-repetition"]};
    _fdFailuresBeforeDead = json["fd-failures-before-dead"];

    _observerIP = ip_v4::from_string(json["observer-ip"]);
    _observerPort = json["observer-port"];
    _observeRepetition = milliseconds{json["observe-repetition"]};

    _logRepetition = milliseconds{json["log-repetition"]};

    _pTable = std::make_unique<Table>(MemberAddr{_ip, _port}, _latestPartSize, _randomPartSize);
    for (const auto& jsonMember : json["table"]) {
        auto member = Member{MemberAddr::FromJSON(jsonMember["address"])};
        _pTable->Update(member);
    }

    std::cout << "end of config" << std::endl;
}

port_t Config::DockerPort() const {
    return _dockerPort;
}

const ip_v4& Config::IP() const {
    return _ip;
}

port_t Config::Port() const {
    return _port;
}

size_t Config::BufferSize() const {
    return _bufferSize;
}

size_t Config::LatestPartSize() const {
    return _latestPartSize;
}

size_t Config::RandomPartSize() const {
    return _randomPartSize;
}

size_t Config::PingProcessingNum() const {
    return _pingProcNum;
}

size_t Config::AckProcessingNum() const {
    return _ackProcNum;
}

size_t Config::SpreadDestinationNum() const {
    return _spreadDestsNum;
}

milliseconds Config::SpreadRepetition() const {
    return _spreadRepetition;
}

milliseconds Config::FDTimout() const {
    return _fdTimeout;
}

milliseconds Config::FDRepetition() const {
    return _fdRepetition;
}

size_t Config::FailuresBeforeDead() const {
    return _fdFailuresBeforeDead;
}

const ip_v4& Config::ObserverIP() const {
    return _observerIP;
}

port_t Config::ObserverPort() const {
    return _observerPort;
}

milliseconds Config::ObserveRepetition() const {
    return _observeRepetition;
}

milliseconds Config::LogRepetition() const {
    return _logRepetition;
}

Table&& Config::MoveTable() {
    return std::move(*_pTable);
}


Socket::Socket(io_service& ioService, uint16_t port, size_t bufferSize)
  : _socket{ioService, endpoint{ip_v4{}, port}}
  , _IBuffer(bufferSize, 0)
{}

void Socket::_SendProtoGossip(const Proto::Gossip& protoGossip) {
    endpoint destEndPoint{ip_v4{protoGossip.dest().addr().ip()},
                          static_cast<uint16_t>(protoGossip.dest().addr().port())};

    // запись `Proto::Gossip` в буфер
    std::vector<byte_t> OBuffer(protoGossip.ByteSize(), 0);
    if (!protoGossip.SerializeToArray(OBuffer.data(), OBuffer.size())) {
        return;
    }

    _socket.send_to(boost::asio::buffer(OBuffer.data(), OBuffer.size()), destEndPoint);
}

void Socket::_GossipCatching(ThreadSaveQueue<PullGossip>& pings, ThreadSaveQueue<PullGossip>& acks) {
    while (true) {
        size_t receivedBytes = _socket.receive(boost::asio::buffer(_IBuffer));

        Proto::Gossip protoGossip;
        if (!protoGossip.ParseFromArray(_IBuffer.data(), receivedBytes)) {
            continue;
        }

        PullGossip gossip{protoGossip};
        protoGossip.clear_table();

        switch (gossip.Type()) {
            case MessageType::Ping :
                std::cout << "[PULL][PING]: " << gossip.Owner().ToJSON() << std::endl;
                pings.Push(std::move(gossip));

                break;
            case MessageType::Ack :
                std::cout << "[PULL][ACK ]: " << gossip.Owner().ToJSON() << std::endl;
                acks.Push(std::move(gossip));

                break;
        }
    }
}

void Socket::_JSONCatching(ThreadSaveQueue<nlohmann::json>& jsons) {
    while (true) {
        size_t receivedBytes = _socket.receive(boost::asio::buffer(_IBuffer));

        jsons.Push(nlohmann::json::from_msgpack(_IBuffer.data(), receivedBytes));
    }
}

void Socket::RunGossipCatching(ThreadSaveQueue<PullGossip>& pings, ThreadSaveQueue<PullGossip>& acks) {
    std::thread catchingThread{&Socket::_GossipCatching, this, std::ref(pings), std::ref(acks)};
    catchingThread.detach();
}

void Socket::RunJSONCatching(ThreadSaveQueue<nlohmann::json>& jsons) {
    std::thread catchingThread{&Socket::_JSONCatching, this, std::ref(jsons)};
    catchingThread.detach();
}

void Socket::SendAcks(Table& table) {
    auto destIndexes = table.AckWaiters();

    Proto::Gossip protoGossip = MakeProtoGossip(table.MakePushTable(), MessageType::Ack);

    for (const auto& index : destIndexes) {
        SetDest(protoGossip, table[index]);

        std::cout << "[PUSH][ACK ]: " << table[index].ToJSON() << std::endl;
        _SendProtoGossip(protoGossip);

        table.ResetAckWaiter(index);
    }

    destIndexes.clear();
}

void Socket::Spread(Table& table, size_t destsNum) {
    auto protoGossip = MakeProtoGossip(table.MakePushTable(), MessageType::Ping);

    for (size_t i = 0; i < destsNum && i < table.Size(); ++i) {
        // слухи распростряняются всем равномерно, но с разной очередностью
        static std::vector<size_t> destList;
        if (destList.empty()) {
            destList = table.MakeDestList();
        }

        size_t destIndex = destList.back();
        destList.pop_back();

        SetDest(protoGossip, table[destIndex]);

        std::cout << "[PUSH][PING]: " << table[destIndex].ToJSON() << std::endl;
        _SendProtoGossip(protoGossip);

        table.SetAckWaitingFrom(destIndex);
    }
}

void Socket::NotifyObserver(const ip_v4& observerIP, port_t observerPort, const Table& table) {
    auto json = nlohmann::json::object();

    json["owner"] = table.WhoAmI().ToJSON();

    auto neighbours = nlohmann::json::array();
    for (size_t i = 0; i < table.Size(); ++i) {
        neighbours.push_back(table[i].ToJSON());
    }
    json["neighbours"] = std::move(neighbours);

    endpoint observerEP{ip_v4{observerIP}, observerPort};
    _socket.send_to(boost::asio::buffer(nlohmann::json::to_msgpack(json)), observerEP);
}

std::vector<size_t> GetIndexesToGossipOwners(const Table& table, const std::vector<PullGossip>& gossips) {
    std::vector<size_t> indexes;
    indexes.reserve(gossips.size());

    for (const auto& gossip : gossips) {
        indexes.push_back(table.ToIndex(gossip.Owner().Addr()));
    }

    return std::move(indexes);
}

void UpdateTable(Table& table, std::vector<PullGossip>& gossips) {
    for (auto& gossip : gossips) {
        // обновление себя (может только увеличиться инкарнация)
        table.Update(gossip.Dest());
        table.Update(gossip.Owner());
        table.Update(gossip.MoveTable());

        table.SetAckWaiter(table.ToIndex(gossip.Owner().Addr()));
    }
}

void SetDest(Proto::Gossip& protoGossip, const Member& dest) {
    protoGossip.clear_dest();
    protoGossip.set_allocated_dest(new Proto::Member{dest.ToProtoType()});
}

void AcceptAcks(Table& table, std::vector<PullGossip>&& acks) {
    for (auto& ack : acks) {
        table.Update(ack.Owner());
        table.Update(ack.Dest());
        table.Update(ack.MoveTable());

        table.ResetAckWaitingFrom(table.ToIndex( ack.Owner().Addr() ));
    }

    acks.clear();
}
