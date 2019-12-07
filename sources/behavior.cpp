// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <behavior.hpp>

Socket::Socket(io_service& ioService, uint16_t port, size_t bufferSize)
  : _socket{ioService, endpoint{ip_v4{}, port}}
  , _IBuffer(bufferSize, 0)
{}

void Socket::SendProtoGossip(const Proto::Gossip& protoGossip) {
    endpoint destEndPoint{ip_v4{protoGossip.dest().addr().ip()},
                          static_cast<uint16_t>(protoGossip.dest().addr().port())};

    // запись `Proto::Gossip` в буфер
    std::vector<byte_t> OBuffer(protoGossip.ByteSize(), 0);
    if (!protoGossip.SerializeToArray(OBuffer.data(), OBuffer.size())) {
        return;
    }

    _socket.send_to(boost::asio::buffer(OBuffer.data(), OBuffer.size()), destEndPoint);
}

void Socket::_GossipCatching(ThreadSaveQueue<PullGossip>& pings,
                             ThreadSaveQueue<PullGossip>& acks,
                             ThreadSaveQueue<PullGossip>& observerCommands,
                             MemberAddr observerAddress) {
    while (true) {
        size_t receivedBytes = _socket.receive(boost::asio::buffer(_IBuffer));

        Proto::Gossip protoGossip;
        if (!protoGossip.ParseFromArray(_IBuffer.data(), receivedBytes)) {
            continue;
        }

        PullGossip gossip{protoGossip};

        if (gossip.Owner().Addr() == observerAddress) {
            observerCommands.Push(std::move(gossip));
            continue;
        }

        switch (gossip.Type()) {
            case MessageType::Ping :
                std::cout << "[PULL][PING]: " << "(size : " << receivedBytes << " )"
                          << gossip.Owner().ToJSON() << std::endl;
                pings.Push(std::move(gossip));

                break;
            case MessageType::Ack :
                std::cout << "[PULL][ACK ]: " << "(size : " << receivedBytes << " )"
                          << gossip.Owner().ToJSON() << std::endl;
                acks.Push(std::move(gossip));

                break;
        }
    }
}

void Socket::_Observing(ThreadSaveQueue<PullGossip>& pulls) {
    while (true) {
        size_t receivedBytes = _socket.receive(boost::asio::buffer(_IBuffer));

        Proto::Gossip protoGossip;
        if (!protoGossip.ParseFromArray(_IBuffer.data(), receivedBytes)) {
            continue;
        }

        PullGossip gossip{protoGossip};

        pulls.Push(std::move(gossip));
    }
}

void Socket::_Killer(Member observer) {
    Proto::Gossip protoGossip;
    protoGossip.set_type(Proto::Gossip::Ping);
    protoGossip.set_allocated_owner(new Proto::Member{observer.ToProtoType()});
    protoGossip.set_allocated_table(new Proto::Table{});

    while (true) {
        std::cout << "Sacrifice address : ";
        std::string sacrificeAddress;
        std::cin >> sacrificeAddress;

        std::cout << "Sacrifice port : ";
        port_t sacrificePort;
        std::cin >> sacrificePort;

        Member dest{MemberAddr{ip_v4::from_string(sacrificeAddress), sacrificePort}};

        SetDest(protoGossip, dest);
        SendProtoGossip(protoGossip);

        std::cout << "Sent command at : " << TimeStamp::Now().Time().count()
                  << std::endl
                  << std::endl;
    }
}

void Socket::RunGossipCatching(ThreadSaveQueue<PullGossip>& pings,
                               ThreadSaveQueue<PullGossip>& acks,
                               ThreadSaveQueue<PullGossip>& observerCommands,
                               MemberAddr observerAddress) {
    std::thread catchingThread{&Socket::_GossipCatching, this, std::ref(pings),
                                                               std::ref(acks),
                                                               std::ref(observerCommands),
                                                               std::move(observerAddress)};
    catchingThread.detach();
}

void Socket::RunObserving(ThreadSaveQueue<PullGossip>& pulls) {
    std::thread catchingThread{&Socket::_Observing, this, std::ref(pulls)};
    catchingThread.detach();
}

void Socket::RunKiller(Member observer) {
    std::thread killerThread{&Socket::_Killer, this, observer};
    killerThread.detach();
}

void Socket::SendAcks(Table& table) {
    auto destIndexes = table.AckWaiters();

    Proto::Gossip protoGossip = MakeProtoGossip(table.MakePushTable(), MessageType::Ack);

    for (const auto& index : destIndexes) {
        SetDest(protoGossip, table[index]);

        std::cout << "[PUSH][ACK ]: " << "(size : " << protoGossip.ByteSize() << " )"
                  << table[index].ToJSON() << std::endl;
        SendProtoGossip(protoGossip);

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

        std::cout << "[PUSH][PING]: " << "(size : " << protoGossip.ByteSize() << " )"
                  << table[destIndex].ToJSON() << std::endl;
        SendProtoGossip(protoGossip);

        table.SetAckWaitingFrom(destIndex);
    }
}

void Socket::NotifyObserver(const ip_v4& observerIP, port_t observerPort, const Table& table) {
    std::vector<size_t> indexes(table.Size(), 0);
    std::iota(indexes.begin(), indexes.end(), 0);

    Proto::Gossip protoGossip = MakeProtoGossip(PushTable(table, std::move(indexes)), MessageType::Ping);

    SetDest(protoGossip, Member{MemberAddr{observerIP, observerPort}});
    SendProtoGossip(protoGossip);
}

void ExecuteObserverCommands(ThreadSaveQueue<PullGossip>& observerCommands) {
    // если данная нода получила от обзервера
    // команду, что нужно уснуть, то флаг поднят
    bool isNodeMustSleep = false;

    // пока нужно спать или не все команды обработаны,
    // то данная нода находится в бесконечном сне
    while (isNodeMustSleep || observerCommands.Size() != 0) {
        if (observerCommands.Size() != 0) {
            // полученная команда инвертирует состояние ноды
            observerCommands.Pop(1);
            isNodeMustSleep = !isNodeMustSleep;
        }
    }
}

bool UpdateTable(Table& table, std::vector<PullGossip>& gossips) {
    bool isUpdated = false;
    for (auto& gossip : gossips) {
        // обновление себя (может только увеличиться инкарнация)
        isUpdated = table.Update(gossip.Dest()) || isUpdated;
        isUpdated = table.Update(gossip.Owner()) || isUpdated;
        isUpdated = table.Update(gossip.MoveTable()) || isUpdated;

        table.SetAckWaiter(table.ToIndex(gossip.Owner().Addr()));
    }

    return isUpdated;
}

void SetDest(Proto::Gossip& protoGossip, const Member& dest) {
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
