// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <behavior.hpp>

void GossipQueue::Push(const Gossip& gossip) {
    std::lock_guard lock{mutex_};
    queue_.push(gossip);
}

bool GossipQueue::SafetyPop(Gossip& gossip) {
    std::lock_guard lock{mutex_};

    if (queue_.empty())
        return false;

    gossip = queue_.front();
    queue_.pop();

    return true;
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

void GossipsCatching(int sd, size_t listenQueueLength, GossipQueue& queue) {
    size_t buffSize = 1500;
    byte inBuff[1500]{};
    while (true) {
        listen(sd, listenQueueLength);

        sockaddr_in addr{};
        size_t msgLength = recvfrom(sd, inBuff, buffSize, 0, (sockaddr*) &addr, nullptr);

        Gossip gossip{};
        // Skips gossip if data unreadable
        if (!gossip.Read(inBuff, inBuff + buffSize)) continue;
        --gossip.TTL;

        queue.Push(gossip);
    }
}

void UpdateTable(MemberTable& table, GossipQueue& queue) {
    Gossip gossip;
    while (queue.SafetyPop(gossip)) {
        table.Update(gossip);
    }
}

void SendGossip(int sd, const Gossip& gossip) {
    size_t size = gossip.ByteSize();

    byte* buff = new byte[size];
    gossip.InsertToBytes(buff, buff + size);

    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_addr = gossip.Dest.Addr.IP;
    dest.sin_port = gossip.Dest.Addr.Port;

    sendto(sd, buff, size, 0, (sockaddr*) &dest, sizeof(dest));

    delete[] buff;
}