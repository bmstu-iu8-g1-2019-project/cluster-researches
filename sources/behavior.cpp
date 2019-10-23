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
        throw std::runtime_error{
            "Couldn't bind inet socket"
        };
    }

    return sd;
}

void GossipsCatching(int sd, size_t listenQueueLength, ThreadSaveGossipQueue& queue) {
    // TODO(AndreevSemen): Change this for env var
    ByteBuffer buffer{1500};
    while (true) {
        listen(sd, listenQueueLength);

        recvfrom(sd, buffer.Begin(), buffer.Size(),0, nullptr, nullptr);

        Gossip gossip{};
        // Skips gossip if data unreadable (Read() returns `nullptr`)
        if (!gossip.Read(buffer.Begin(), buffer.End())) continue;

        queue.Push(gossip);

        std::cout << gossip.Owner.ToJSON() << std::endl;
    }
}

std::deque<Conflict> UpdateTable(MemberTable& table, const std::deque<Gossip>& gossipQueue) {
    std::deque<Conflict> conflicts;

    for (const auto& gossip : gossipQueue) {
        table.Update(gossip, conflicts);
    }

    return conflicts;
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
        newGossip.Owner = gossip.Dest;
        newGossip.Dest = table.RandomMember();
        newGossip.Events = gossip.Events;
        // TODO : Change it for env var
        newGossip.Table = table.GetSubset(4);

        newGossips.push_back(newGossip);

        queue.pop_front();
    }

    return newGossips;
}

void SendGossip(int sd, const Gossip& gossip) {
    std::vector<byte> buffer;
    buffer.resize(gossip.ByteSize());

    gossip.Write(buffer.data(), buffer.data() + buffer.size());

    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = htonl(gossip.Dest.Addr.IP.s_addr);
    dest.sin_port = htons(gossip.Dest.Addr.Port);

    if (sendto(sd, buffer.data(), buffer.size(), 0, (sockaddr*) &dest, sizeof(dest)) == -1)
        throw std::runtime_error{
            "Couldn't send gossip"
        };
}

void AppConnector(const MemberTable& table) {
    int sd = socket(AF_UNIX, SOCK_STREAM, 0);

    sockaddr_un path{};
    path.sun_family = AF_UNIX;
    // TODO : Change it for env var
    std::string str{"./socket.sock"};
    std::copy(str.cbegin(), str.cend(), path.sun_path);

    if (bind(sd, (sockaddr*) &path, sizeof(path)) == -1) {
        throw std::runtime_error{
            "Unable to open UNIX-socket"
        };
    }

    std::string json = table.ToJSON().dump();
    while (true) {
        std::this_thread::__sleep_for(std::chrono::seconds{5}, std::chrono::nanoseconds{0});
        send(sd, json.c_str(), json.size(), 0);
    }
}
