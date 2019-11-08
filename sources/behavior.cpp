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

boost::asio::ip::udp::socket SetupSocket(boost::asio::io_service& ioService, uint16_t port) {
    using namespace boost::asio;
    ip::udp::socket sock(ioService, ip::udp::endpoint{ip::address_v4::any(), port});

    sock.set_option(boost::asio::ip::udp::socket::reuse_address{});

    return std::move(sock);
}

void GossipsCatching(boost::asio::ip::udp::socket& sock, ThreadSaveGossipQueue& queue) {
    // TODO(AndreevSemen): Change this for env var
    // Max available UDP packet size
    ByteBuffer buffer{1500};
    while (true) {
        std::cout << "Gossip-catching began" << std::endl;

        boost::asio::ip::udp::endpoint senderEp{};
        std::cout << "Boost sock received : " << sock.receive_from(boost::asio::buffer(buffer.Begin(), buffer.Size()), senderEp) << std::endl;

        Gossip gossip{};
        // Skips gossip if data unreadable (Read() returns `nullptr`)
        if (!gossip.Read(buffer.Begin(), buffer.End())) {
            std::cout << "Gossip is invalid" << std::endl;
            continue;
        }

        queue.Push(gossip);

        std::cout << gossip.ToJSON().dump(2) << std::endl;
    }
}

std::deque<Member> UpdateTable(MemberTable& table, const std::deque<Gossip>& gossipQueue) {
    std::deque<Member> conflicts;

    for (const auto& gossip : gossipQueue) {
        if (table.Size() == 0) {
            table.SetMe(gossip.Dest);
        }

        table.Update(gossip.Owner);
        auto occurredConflicts = table.TryUpdate(gossip.Table);
        conflicts.insert(conflicts.end(), occurredConflicts.begin(),
                                          occurredConflicts.end());
    }

    return conflicts;
}

Gossip GenerateGossip(MemberTable& table) {
    Gossip gossip;

    gossip.TTL = 1;
    gossip.Owner = table.Me();
    gossip.Dest = table.RandomMember();
    gossip.Table = table.GetSubset();

    return gossip;
}

void SendGossip(boost::asio::ip::udp::socket& sock, const Gossip& gossip) {
    // Creates buffer and write gossip to buffer
    ByteBuffer buffer{gossip.ByteSize()};

    if (!gossip.Write(buffer.Begin(), buffer.End())) {
        std::cout << "Gossip couldn't be written" << std::endl;
        return;
    }

    // Chooses endpoint (dest) and sends gossip
    boost::asio::ip::udp::endpoint destEp{gossip.Dest.Addr.IP, gossip.Dest.Addr.Port};
    sock.send_to(boost::asio::buffer(buffer.Begin(), buffer.Size()), destEp);
}

void SpreadGossip(boost::asio::ip::udp::socket& sock, const MemberTable& table, Gossip& gossip, size_t destNum) {
    for (size_t i = 0; i < destNum; ++i) {
        if (table.Size() != 0) {
            gossip.Dest = table.RandomMember();
        }
        SendGossip(sock, gossip);
    }
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
