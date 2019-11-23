// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <behavior.hpp>

Socket::Socket(io_service& ioService, uint16_t port, size_t bufferSize)
  : _socket{ioService, endpoint{ip_v4{}, port}}
  , _buffer(bufferSize, 0)
{}

void Socket::_SendProtoGossip(const Proto::Gossip& protoGossip) {
    endpoint destEndPoint{ip_v4{protoGossip.dest().addr().ip()},
                          static_cast<uint16_t>(protoGossip.dest().addr().port())};

    if (!protoGossip.SerializeToArray(_buffer.data(), _buffer.size())) {
        std::cout << "Proto::Gossip send failure" << std::endl;
        return;
    }

    _socket.send_to(boost::asio::buffer(_buffer.data(), protoGossip.ByteSize()), destEndPoint);
}

void Socket::_GossipCatching(ThreadSaveQueue<PullGossip>& pings, ThreadSaveQueue<PullGossip>& acks) {
    while (true) {
        size_t receivedBytes = _socket.receive(boost::asio::buffer(_buffer));

        Proto::Gossip protoGossip;
        if (!protoGossip.ParseFromArray(_buffer.data(), receivedBytes)) {
            continue;
        }

        PullGossip gossip{protoGossip};
        protoGossip.clear_table();

        switch (gossip.Type()) {
            case MessageType::Ping :
                pings.Push(std::move(gossip));
                break;
            case MessageType::Ack :
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
    }

    destIndexes.clear();
}

void Socket::Spread(Table &table, size_t destsNum) {
    static auto destQueue = table.MakeDestList();

    auto protoGossip = MakeProtoGossip(table.MakePushTable(), MessageType::Ping);

    for (size_t i = 0; i < destsNum && i < table.Size(); ++i) {
        if (destQueue.empty()) {
            destQueue = table.MakeDestList();
        }

        size_t destIndex = destQueue.back();
        destQueue.pop_back();

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

void SendProtoGossip(socket_type& socket, const Proto::Gossip& protoGossip) {
    std::vector<byte_t> buffer;
    buffer.resize(protoGossip.ByteSize());

    endpoint destEndPoint{ip_v4{protoGossip.dest().addr().ip()},
                          static_cast<uint16_t>(protoGossip.dest().addr().port())};
    socket.send_to(boost::asio::buffer(buffer), destEndPoint);
}
