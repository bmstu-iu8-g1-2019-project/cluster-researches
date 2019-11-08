// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <thread>

#include <behavior.hpp>


int main() {
    SetupConfig("config.conf");

    MemberTable table{gVar.TableLatestUpdatesSize, gVar.TableRandomMembersSize};
    table.SetMe(Member{gVar.Address,
                MemberInfo{MemberInfo::State::Alive, 0, TimeStamp{0}}});
    table.FromJson(gVar.jsonTable);
    gVar.jsonTable.clear();

    boost::asio::io_service ioService;
    auto sock = SetupSocket(ioService, gVar.Address.Port);

    ThreadSaveGossipQueue threadSaveQueue;

    std::thread threadInput{GossipsCatching, std::ref(sock), std::ref(threadSaveQueue)};
    threadInput.detach();

    std::thread pingingThread{MemberPinging, std::ref(sock), std::ref(table)};
    pingingThread.detach();

    // std::thread appConnector{AppConnector, std::ref(table)};
    // appConnector.detach();

    while (true) {
        std::deque<Gossip> receivedGossips;
        while (receivedGossips.empty()) {
            receivedGossips = threadSaveQueue.Free();
        }

        auto conflicts = UpdateTable(table, receivedGossips);

        auto newGossip = GenerateGossip(table);

        SpreadGossip(sock, table, newGossip, gVar.SpreadNumber);
    }
}

