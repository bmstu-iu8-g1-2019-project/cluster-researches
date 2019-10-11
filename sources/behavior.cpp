// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <behavior.hpp>

void ThreadSaveGossipQueue::Push(const Gossip& gossip) {
    std::lock_guard lock{mutex_};
    queue_.push_back(gossip);
}

std::deque<Gossip> ThreadSaveGossipQueue::Free() {
    std::lock_guard lock{mutex_};

    std::deque<Gossip> bucket{queue_};
    queue_.clear();

    return std::move(bucket);
}

int SetupSocket(in_port_t port) {
    int sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd == -1)
        return -1;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sd, (sockaddr*) &addr, sizeof(addr)) == -1) {
        return -1;
    }

    return sd;
}

void GossipsCatching(int sd, size_t listenQueueLength, ThreadSaveGossipQueue& queue) {
    // TODO(AndreevSemen): Change this for env var
    ByteBuffer buffer{1500};
    while (true) {
        listen(sd, listenQueueLength);

        size_t msgLength = recvfrom(sd, buffer.Begin(), buffer.Size(),
                                     0, nullptr, nullptr);

        Gossip gossip{};
        // Skips gossip if data unreadable
        if (!gossip.Read(buffer.Begin(), buffer.End())) continue;

        queue.Push(gossip);
    }
}

void UpdateTable(MemberTable& table, const std::deque<Gossip>& queue) {
    Gossip gossip;
    while (!queue.empty()) {
        table.Update(queue.front());
    }
}

std::deque<Gossip> GenerateGossips(MemberTable& table, std::deque<Gossip>& queue) {
    std::deque<Gossip> newGossips;

    for (const auto& gossip : queue) {
        if (queue.front().TTL == 0) {
            queue.pop_front();
            continue;
        }

        Gossip newGossip{};
        newGossip.TTL = gossip.TTL - 1;
        newGossip.Owner = {gossip.Dest.Addr, gossip.Dest.Info};
        newGossip.Dest
        newGossip.Events = gossip.Events;
        // TODO : Change it for env var
        newGossip.Table = table.GetSubset(4);

        newGossips.push_back(newGossip);
    }
}

void SendGossip(int sd, const Gossip& gossip) {
    std::vector<byte> buffer;
    buffer.resize(gossip.ByteSize());

    gossip.Write(buffer.data(), buffer.data() + buffer.size());

    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_addr = gossip.Dest.Addr.IP;
    dest.sin_port = gossip.Dest.Addr.Port;

    sendto(sd, buffer.data(), buffer.size(), 0, (sockaddr*) &dest, sizeof(dest));
}