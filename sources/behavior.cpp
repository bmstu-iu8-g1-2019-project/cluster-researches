// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <behavior.hpp>

void CatchGossips(socket_type& socket, ThreadSaveQueue<PullGossip>& pings, ThreadSaveQueue<PullGossip>& acks) {
    std::vector<byte_t> buffer;
    buffer.resize(1500); // TODO(AndreevSemen) : change it for env var

    while (true) {
        size_t receivedBytes = socket.receive(boost::asio::buffer(buffer));

        Proto::Gossip protoGossip;
        if (!protoGossip.ParseFromArray(buffer.data(), receivedBytes)) {
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

std::vector<size_t> UpdateTable(Table& table, std::queue<PullGossip>& queue) {
    std::vector<size_t> allConflicts;

    while (!queue.empty()) {
        auto gossip = std::move(queue.front());

        std::vector<size_t> conflicts = table.Update(gossip.MoveTable());

        table.Update(gossip.Dest());

        if (!table.Update(gossip.Owner())) {
            conflicts.push_back(table.ToIndex(gossip.Owner().Addr()));
        }

        for (auto& conflict : conflicts) {
            allConflicts.push_back(conflict);
        }

        queue.pop();
    }

    return std::move(allConflicts);
}
