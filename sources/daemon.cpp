// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <thread>

#include <behavior.hpp>


int main() {
    // TODO(AndreevSemen) : Change port for env variable
    int sd = SetupSocket(82);
    if (sd == -1)
        throw std::runtime_error{
            "Unable to open inet socket"
        };

    ThreadSaveGossipQueue threadSaveQueue;
    
    // TODO(AndreevSemen) : Change listening queue length for env variable
    std::thread threadInput{GossipsCatching, sd, 10, std::ref(threadSaveQueue)};
    threadInput.detach();

    MemberTable table;

    std::thread appConnector{AppConnector, std::ref(table)};
    appConnector.detach();

    while (true) {
        std::deque<Gossip> receivedGossips = threadSaveQueue.Free();
        auto conflicts = UpdateTable(table, receivedGossips);

        auto newGossips = GenerateGossips(table, receivedGossips);

        for (const auto &gossip : newGossips) {
            SendGossip(sd, gossip);
        }
    }
}

