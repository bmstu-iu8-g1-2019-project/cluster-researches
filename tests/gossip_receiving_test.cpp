// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <types.hpp>
#include <behavior.hpp>
#include <gtest/gtest.h>

class MemberList {
private:
    std::vector<Member> list_;
    mutable std::mt19937 generator_;

public:
    MemberList()
            : generator_(std::random_device{}())
    {
        std::vector<std::string> strIPVec = {
                "127.0.0.1",
                "192.72.1.12",
                "111.111.111.111",
                "217.69.128.44",
                "77.88.8.8",
                "213.180.217.10",
                "213.180.217.219",
                "213.180.217.7",
                "213.180.210.1",
                "213.180.194.138",
                "213.180.194.164",
                "213.180.194.185",
                "213.180.216.233",
                "213.180.216.234",
                "213.180.210.5",
                "213.180.210.9",
                "213.180.210.7",
                "213.180.210.10",
                "213.180.216.30",
                "213.180.216.160",
                "213.180.216.164",
                "213.180.216.165",
                "213.180.216.7"
        };
        std::vector<boost::asio::ip::address> uintIPVec;
        std::vector<uint16_t> portVec;
        std::vector<MemberAddr> addrVec;

        std::vector<MemberInfo::State> statesVec;
        std::vector<uint32_t> incarnationVec;
        std::vector<TimeStamp> timeVec;
        std::vector<MemberInfo> infoVec;

        // initialize loop
        for (size_t i = 0; i < strIPVec.size(); ++i) {
            // IP
            uintIPVec.emplace_back(boost::asio::ip::address::from_string(strIPVec[i]));
            // port
            portVec.push_back(80 + i);
            // addr
            addrVec.emplace_back(MemberAddr{uintIPVec[i],
                                            portVec[i]});

            // state
            statesVec.push_back(MemberInfo::State(generator_()%4));
            // incarnation
            incarnationVec.push_back(static_cast<uint32_t>(generator_()));
            // timestamp
            timeVec.push_back(TimeStamp {static_cast<uint32_t>(generator_())});
            // info
            infoVec.emplace_back(MemberInfo{statesVec[i],
                                            incarnationVec[i],
                                            timeVec[i]});

            // member
            list_.emplace_back(addrVec[i], infoVec[i]);
        }
    }

    const Member& RandomMember() const {
        return list_[generator_() % list_.size()];
    }

    std::vector<Member>& GetList() {
        return list_;
    }
} list;

int main() {
    list.GetList()[0].Addr.Port = 8005;

    MemberTable table;
    for (const auto& member :  list.GetList()) {
        table.DebugInsert(member);
    }


    Gossip gossip;
    gossip.Owner = list.RandomMember();

    gossip.Owner.Addr.IP = boost::asio::ip::address_v4::loopback();
    gossip.Owner.Addr.Port = 8000;

    gossip.Dest = list.RandomMember();
    gossip.Dest.Addr.IP = gossip.Owner.Addr.IP;
    gossip.Dest.Addr.Port = 8005;

    gossip.Table = table;
    gossip.TTL = 10;

    boost::asio::io_service ioService;
    boost::asio::ip::udp::socket sock(ioService, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 8000));

    ByteBuffer bBuffer{gossip.ByteSize()};
    gossip.Write(bBuffer.Begin(), bBuffer.End());

    ThreadSaveGossipQueue receivedGossips;
    std::thread catching{GossipsCatching, std::ref(sock), std::ref(receivedGossips)};
    catching.detach();

    SendGossip(sock, gossip);
    std::cout << "Sent :" << std::endl;
    std::cout << gossip.ToJSON().dump(2) << std::endl;

    std::this_thread::__sleep_for(std::chrono::seconds{2}, std::chrono::nanoseconds{0});

    auto result = receivedGossips.Free();
    if (result.empty()) {
        std::cout << "[=================]PANIC[=================]" << std::endl;
    }
    EXPECT_EQ(result.front().Owner, gossip.Dest);

    return 0;
}