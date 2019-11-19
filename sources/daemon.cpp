// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <thread>

#include <behavior.hpp>

extern Variables gVar;

int main(int argc, char** argv) {
    gVar = SetupConfig(argv[1]);

    MemberTable table{gVar.TableLatestUpdatesSize, gVar.TableRandomMembersSize};
    table.SetMe(Member{gVar.Address,
                MemberInfo{MemberInfo::State::Alive, 0, TimeStamp{}}});

    if (gVar.NodeList.is_array()) {
        for (const auto& address : gVar.NodeList) {
            MemberAddr addr;
            addr.FromJson(address);
            Member member{addr, MemberInfo{}};
            table.Update(member);
        }
    }
    gVar.NodeList.clear();

    boost::asio::io_service ioService;
    auto sock = SetupSocket(ioService, gVar.Address.Port);

    ThreadSaveQueue<Gossip> spreadQueue;
    ThreadSaveQueue<Gossip> fdQueue; // failure detection queue
    ThreadSaveQueue<Member> conflictQueue;

    std::thread inputThread{GossipsCatching, std::ref(sock), std::ref(spreadQueue),
                                                             std::ref(fdQueue)};
    inputThread.detach();

    std::thread fdThread{FailureDetection, std::ref(sock), std::ref(table), std::ref(fdQueue), std::ref(conflictQueue)};
    fdThread.detach();

    std::thread appConnector{AppConnector, std::ref(table), 7999};
    appConnector.detach();

    std::deque<Member> destQueue;
    std::deque<Gossip> receivedGossips;
    while (true) {
        while (spreadQueue.Empty());
        receivedGossips = spreadQueue.Free();

        conflictQueue.Push(UpdateTable(table, receivedGossips));
        receivedGossips.clear();

        auto newGossip = GenerateGossip(table, Gossip::GossipType::Spread);
        SpreadGossip(sock, table, destQueue, newGossip, gVar.SpreadNumber);
    }
}

