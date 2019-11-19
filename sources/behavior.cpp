// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <behavior.hpp>

Variables gVar;

Variables SetupConfig(const std::string& configPath) {
    std::ifstream configFile{configPath};
    nlohmann::json config = nlohmann::json::parse(configFile);

    Variables vars{};
    vars.Address.FromJson(config["address"]);
    vars.TableLatestUpdatesSize = config["TableLatestUpdatesSize"];
    vars.TableRandomMembersSize = config["TableRandomMembersSize"];
    vars.SpreadNumber = config["SpreadNumber"];
    vars.ReceivingBufferSize = config["ReceivingBufferSize"];
    vars.PingInterval = std::chrono::seconds{config["PingInterval"]};
    vars.UNIXSocketPath = config["UNIXSocketPath"];
    vars.NodeList = config["NodeList"];

    return vars;
}

udpSocket SetupSocket(boost::asio::io_service& ioService, uint16_t port) {
    udpSocket sock(ioService, ip4Endpoint{ip4::any(), port});

    sock.set_option(udpSocket::reuse_address{});

    return sock;
}

void GossipsCatching(udpSocket& sock, ThreadSaveQueue<Gossip>& spread, ThreadSaveQueue<Gossip>& fd) {
    // Max available UDP packet size
    ByteBuffer buffer{gVar.ReceivingBufferSize};
    while (true) {
        ip4Endpoint senderEp{};
        sock.receive_from(boost::asio::buffer(buffer.Begin(), buffer.Size()), senderEp);

        Gossip gossip{};
        // Skips gossip if data unreadable (Read() returns `nullptr`)
        if (!gossip.Read(buffer.Begin(), buffer.End())) {
            std::cout << "+=+=+=+=Gossip is invalid" << std::endl;
            continue;
        }

        std::cout << "[RECV]:";
        if (gossip.Type == Gossip::Ping ||
            gossip.Type == Gossip::Ack) {
            if (gossip.Type == Gossip::Ping) {
                std::cout << "[PING]:";
                std::cout << gossip.Owner.ToJSON().dump() << std::endl;
            } else {
                std::cout << "[ACK ]:";
                std::cout << gossip.Owner.ToJSON().dump() << std::endl;
            }

            fd.Push(gossip);
        } else if (gossip.Type == Gossip::GossipType::Spread) {
            std::cout << "[SPRD]:";
            std::cout << gossip.Owner.ToJSON().dump() << std::endl;

            spread.Push(gossip);
        }
    }
}

void FailureDetection(udpSocket& sock, MemberTable& table, ThreadSaveQueue<Gossip>& fdQueue, ThreadSaveQueue<Member>& conflictQueue) {
    while (table.Size() == 0);

    std::vector<std::pair<Member, TimeStamp>> await;

    std::deque<Member> destQueue;
    while (true) {
        auto gossips = fdQueue.Free();
        conflictQueue.Push(UpdateTable(table, gossips));

        // Receiving of Pings and Acks
        // if Ping : send ack
        // if Ack : remove from awaiting list
        for (const auto& gossip : gossips) {
            if (gossip.Type == Gossip::Ping) {
                auto ack = GenerateGossip(table, Gossip::Ack);
                ack.Dest = gossip.Owner;

                SendGossip(sock, ack);
            } else {
                auto found = std::find_if(await.begin(), await.end(),
                        [&gossip](const auto& record) {
                            return record.first.Addr == gossip.Owner.Addr;
                });

                if (found != await.end()) {
                    await.erase(found);
                }
            }
        }

        // Resolving unanswered pings
        // if Delay > MAX_DELAY : update `Status` and remove from await list
        // else : forward
        for (auto iter = await.begin(); iter != await.end();) {
            // TODO(AndreevSemen) : change for env
            if (iter->second.Delay(TimeStamp::Now()) > std::chrono::seconds{1}) {
                switch (iter->first.Info.Status) {
                    case MemberInfo::Alive :
                        iter->first.Info.Status = MemberInfo::Suspicious;
                        break;
                    case MemberInfo::Suspicious :
                        iter->first.Info.Status = MemberInfo::Dead;
                        break;
                    default:
                        break;
                }
                iter->first.Info.LastUpdate = TimeStamp::Now();
                table.Update(iter->first);

                await.erase(iter);
            } else {
                ++iter;
            }
        }

        auto gossip = GenerateGossip(table, Gossip::Ping);
        do {
            gossip.Dest = table.RandomMember();
        } while (gossip.Dest.Info.Status == MemberInfo::Left);

        std::cout << "[PING]:";
        SendGossip(sock, gossip);
        await.emplace_back(std::make_pair(gossip.Dest, TimeStamp::Now()));

        // Try to ping conflicted member
        auto conflicts = conflictQueue.Free();
        for (const auto& member : conflicts) {
            gossip.Dest = member;

            std::cout << "[CONFL ]:";
            SendGossip(sock, gossip);
            std::cout << member.ToJSON() << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds{gVar.PingInterval});
    }
}


std::deque<Member> UpdateTable(MemberTable& table, const std::deque<Gossip>& gossipQueue) {
    std::deque<Member> conflicts;

    for (const auto& gossip : gossipQueue) {
        if (gossip.Dest.IsStatusWorse(table.Me())) {
            auto newMe = table.Me();
            ++newMe.Info.Incarnation;
            newMe.Info.LastUpdate = TimeStamp::Now();

            table.SetMe(newMe);
        }

        auto owner = gossip.Owner;
        owner.Info.LastUpdate = TimeStamp::Now();
        table.Update(owner);

        auto occurredConflicts = table.TryUpdate(gossip.Table);
        conflicts.insert(conflicts.end(), occurredConflicts.begin(),
                                          occurredConflicts.end());
    }

    return conflicts;
}

Gossip GenerateGossip(MemberTable& table, Gossip::GossipType type) {
    Gossip gossip;

    gossip.Type = type;
    gossip.Owner = table.Me();
    gossip.Table = table.GetSubset();

    return gossip;
}

void SendGossip(udpSocket& sock, const Gossip& gossip) {
    // Creates buffer and write gossip to buffer
    if (gossip.Type == Gossip::Default) {
        std::cout << "+=+=+=+=Gossip type couldn't be default" << std::endl;
        return;
    }

    ByteBuffer buffer{gossip.ByteSize()};
    if (!gossip.Write(buffer.Begin(), buffer.End())) {
        std::cout << "+=+=+=+=Gossip couldn't be written" << std::endl;
        return;
    }

    // Chooses endpoint (dest) and sends gossip
    boost::asio::ip::udp::endpoint destEp{gossip.Dest.Addr.IP, gossip.Dest.Addr.Port};
    sock.send_to(boost::asio::buffer(buffer.Begin(), buffer.Size()), destEp);
    std::cout << "[SENT]:[TO]:" << gossip.Dest.ToJSON() << std::endl;
}

void SpreadGossip(udpSocket& sock, const MemberTable& table, std::deque<Member>& destQueue, Gossip& gossip, size_t destNum) {
    for (size_t i = 0; i < destNum || i < table.Size(); ++i) {
        if (destQueue.empty()) {
            destQueue = table.GetDestQueue();
        }
        gossip.Dest = destQueue.front();
        destQueue.pop_front();

        std::cout << "[SPREAD]:";
        SendGossip(sock, gossip);
    }
}

void AppConnector(const MemberTable& table, uint16_t port) {
    using namespace boost::asio;

    /*io_service ioService;
    ip::tcp::socket sock(ioService, ip::tcp::endpoint{ip::address_v4::any(), port});

    sock.connect(ip::tcp::endpoint{ip::address_v4::from_string("127.0.0.1"), port});

    while (true) {
        sock.send(boost::asio::buffer(table.ToJSON().dump()));

        std::this_thread::sleep_for(std::chrono::seconds{gVar.PingInterval});
    }*/
}
