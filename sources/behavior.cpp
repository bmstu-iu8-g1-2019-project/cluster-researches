// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <behavior.hpp>

Config::Config(const std::string& path) {
    std::ifstream file{path};

    if (!file.is_open()) {
        return;
    }

    auto json = nlohmann::json::parse(file);

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


    _pTable = std::make_unique<Table>(MemberAddr{_ip, _port}, _latestPartSize, _randomPartSize);
    for (const auto& jsonMember : json["table"]) {
        auto member = Member{MemberAddr::FromJSON(jsonMember["address"])};
        _pTable->Update(member);
    }
}

const ip_v4& Config::IP() const {
    return _ip;
}

uint16_t Config::Port() const {
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

Table&& Config::MoveTable() {
    return std::move(*_pTable);
}


Socket::Socket(io_service& ioService, uint16_t port, size_t bufferSize)
  : _socket{ioService, endpoint{ip_v4{}, port}}
  , _IBuffer(bufferSize, 0)
  , _OBuffer(bufferSize, 0)
{}

void Socket::_SendProtoGossip(const Proto::Gossip& protoGossip) {
    endpoint destEndPoint{ip_v4{protoGossip.dest().addr().ip()},
                          static_cast<uint16_t>(protoGossip.dest().addr().port())};

    if (!protoGossip.SerializeToArray(_OBuffer.data(), _OBuffer.size())) {
        std::cout << "Proto::Gossip send failure" << std::endl;
        return;
    }

    _socket.send_to(boost::asio::buffer(_OBuffer.data(), protoGossip.ByteSize()), destEndPoint);
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
                std::cout << "[PULL]:[PING]: " << gossip.Owner().ToJSON().dump() << std::endl;

                pings.Push(std::move(gossip));

                break;
            case MessageType::Ack :
                std::cout << "[PULL]:[ACK ]: " << gossip.Owner().ToJSON().dump() << std::endl;

                acks.Push(std::move(gossip));

                break;
        }
    }
}

void Socket::RunGossipCatching(ThreadSaveQueue<PullGossip>& pings, ThreadSaveQueue<PullGossip>& acks) {
    std::thread catchingThread{&Socket::_GossipCatching, this, std::ref(pings), std::ref(acks)};
    catchingThread.detach();
}

void Socket::SendAcks(const Table& table, std::vector<size_t>&& destIndexes) {
    Proto::Gossip protoGossip = MakeProtoGossip(table.MakePushTable(), MessageType::Ack);

    for (const auto& index : destIndexes) {
        SetDest(protoGossip, table[index]);

        _SendProtoGossip(protoGossip);

        protoGossip.clear_dest();

        std::cout << "[PUSH]:[ACK ]: " << table[index].ToJSON().dump() << std::endl;
    }

    destIndexes.clear();
}

void Socket::Spread(Table& table, size_t destsNum) {
    static auto destQueue = table.MakeDestList();

    auto protoGossip = MakeProtoGossip(table.MakePushTable(), MessageType::Ping);

    for (size_t i = 0; i < destsNum && i < table.Size(); ++i) {
        if (destQueue.empty()) {
            destQueue = table.MakeDestList();
        }

        size_t destIndex = destQueue.back();
        destQueue.pop_back();

        std::cout << "[PUSH]:[PING]: " << table[destIndex].ToJSON().dump() << std::endl;

        SetDest(protoGossip, table[destIndex]);

        _SendProtoGossip(protoGossip);

        table.SetAckWaiting(destIndex);
    }
}

std::vector<size_t> GetIndexesToGossipOwners(const Table& table, const std::vector<PullGossip>& gossips) {
    std::vector<size_t> indexes;
    indexes.reserve(gossips.size());

    for (const auto& gossip : gossips) {
        indexes.push_back(table.ToIndex(gossip.Owner().Addr()));
    }

    return std::move(indexes);
}

std::vector<size_t> UpdateTable(Table& table, std::vector<PullGossip>& gossips) {
    std::vector<size_t> allConflicts;

    for (auto& gossip : gossips) {
        std::vector<size_t> conflicts = table.Update(gossip.MoveTable());

        table.Update(gossip.Dest());

        if (!table.Update(gossip.Owner())) {
            conflicts.push_back(table.ToIndex(gossip.Owner().Addr()));
        }

        for (auto& conflict : conflicts) {
            allConflicts.push_back(conflict);
        }
    }

    return std::move(allConflicts);
}

void SetDest(Proto::Gossip& protoGossip, const Member& dest) {
    protoGossip.clear_dest();
    protoGossip.set_allocated_dest(new Proto::Member{dest.ToProtoType()});
}

void AcceptAcks(Table& table, std::vector<PullGossip>&& acks) {
    for (const auto& ack : acks) {
        table.ResetAckWaiting(table.ToIndex( ack.Owner().Addr() ));
    }

    acks.clear();
}
