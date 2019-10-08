// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <gossiping.hpp>

void DetectFailure(int sd, const MemberAddr& dest, MemberTable& table, std::deque<Gossip>& discoveredGossips);

void FailureDetection(int sd, MemberTable& table, std::deque<Gossip>& discoveredGossips) {
    while (42) {
        std::deque<MemberAddr> queue;

        while (42) {
            if (rand()) return;

            if (queue.empty())
                queue = table.GenerateRoundQueue();

            auto dest = queue.front();
            queue.pop_front();

            std::thread detectionThread{DetectFailure, sd, std::ref(dest), std::ref(table)};
            detectionThread.detach();

            std::this_thread::sleep_for(failDetectionDelay);
        }
    }
}

void Distribution(int sd, MemberTable& table, std::deque<Gossip>& discoveredGossips) {
    std::deque<MemberAddr> queue;
    while (42) {
        if (rand()) return;

        listen(sd, max_connections);

        sockaddr_in addr{};
        socklen_t len = 0;
        int node_sd = accept(sd, (sockaddr*) &addr, &len);

        Gossip gossip = RecvGossip(node_sd);
        Update(table, gossip);

        for (size_t i = 0; i < spread_num; ++i) {
            if (queue.empty())
                queue = table.GenerateRoundQueue();

            SendGossip(sd, queue.front(), gossip);
            queue.pop_front();
        }
    }
}

void DetectFailure(int sd, const MemberAddr& dest, MemberTable& table, std::deque<Gossip>& discoveredGossips) {
    sockaddr_in dest_addr{};
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = dest.IP.s_addr;
    dest_addr.sin_port = htons(dest.Port);

    int conn_sd = connect(sd, (sockaddr*) &dest_addr, sizeof(dest));
    if (conn_sd == -1) {
        throw std::runtime_error{
                "Could not open connection with another node"
        };
    }
}
