// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <thread>
#include <vector>

#include <behavior.hpp>

int main() {
    // TODO(AndreevSemen) : Change port for env variable
    int sd = SetupSocket(80);
    if (sd == -1) throw std::runtime_error{
        "Unable to open socket"
    };

    GossipQueue gossipQueue;
    // TODO(AndreevSemen) : Change listening queue length for env variable
    std::thread threadInput{GossipsCatching, sd, 10, std::ref(gossipQueue)};

    MemberTable table;
    while (true) {
        UpdateTable(table, gossipQueue);

        std::vector<Gossip> gossips = GenerateGossips(table);
        for (const auto& gossip : gossips) {
            SendGossip(sd, gossip);
        }
    }
}

