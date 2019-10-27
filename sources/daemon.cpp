// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <thread>

#include <behavior.hpp>


int main() {
    boost::asio::io_service ioService;
    auto sock = SetupSocket(ioService, 8005);

    ThreadSaveGossipQueue threadSaveQueue;
    
    // TODO(AndreevSemen) : Change listening queue length for env variable
    std::thread threadInput{GossipsCatching, std::ref(sock), std::ref(threadSaveQueue)};
    threadInput.detach();

    MemberTable table;

    //std::thread appConnector{AppConnector, std::ref(table)};
    //appConnector.detach();

    while (true) {
        std::deque<Gossip> receivedGossips = threadSaveQueue.Free();
        auto conflicts = UpdateTable(table, receivedGossips);

        auto newGossips = GenerateGossips(table, receivedGossips);

        for (const auto &gossip : newGossips) {
            SendGossip(sock, gossip);
        }
    }
}

