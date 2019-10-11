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

    ThreadSaveGossipQueue threadSaveQueue;
    // TODO(AndreevSemen) : Change listening queue length for env variable
    std::thread threadInput{GossipsCatching, sd, 10, std::ref(threadSaveQueue)};

    MemberTable table;
    while (true) {
        std::deque<Gossip> receivedGossips{threadSaveQueue.Free()};
        UpdateTable(table, receivedGossips);

        auto newGossips = GenerateGossips(table, receivedGossips);

        for (const auto& gossip : newGossips) {
            SendGossip(sd, gossip);
        }
    }
}

