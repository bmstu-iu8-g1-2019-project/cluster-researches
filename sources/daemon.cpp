// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <thread>

#include <behavior.hpp>


int main() {
    boost::asio::io_service ioService;
    auto sock = SetupSocket(ioService, 8005);

    ThreadSaveGossipQueue threadSaveQueue;

    std::thread threadInput{GossipsCatching, std::ref(sock), std::ref(threadSaveQueue)};
    threadInput.detach();

    // TODO(AndreevSemen): Change this for env var
    MemberTable table{10, 3};

    // std::thread appConnector{AppConnector, std::ref(table)};
    // appConnector.detach();

    while (true) {
        std::deque<Gossip> receivedGossips;
        while (receivedGossips.empty()) {
            receivedGossips = threadSaveQueue.Free();
        }

        auto conflicts = UpdateTable(table, receivedGossips);

        auto newGossip = GenerateGossip(table);

        // TODO(AndreevSemen): Change this for env var
        SpreadGossip(sock, table, newGossip, 100);
    }
}

