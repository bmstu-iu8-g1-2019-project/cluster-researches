// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <types.hpp>
#include <behavior.hpp>

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

    const std::vector<Member>& GetList() const {
        return list_;
    }
} list;

int main() {
    Gossip gossip;
    gossip.Owner = list.RandomMember();

    gossip.Owner.Addr.IP = boost::asio::ip::address_v4::loopback();
    gossip.Owner.Addr.Port = 83;

    gossip.Dest = list.RandomMember();
    gossip.Dest.Addr.IP = gossip.Owner.Addr.IP;
    gossip.Dest.Addr.Port = 80;

    gossip.Events = {};
    gossip.TTL = 10;

    boost::asio::io_service ioService;
    boost::asio::ip::udp::socket sock(ioService, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 8000));

    ByteBuffer bBuffer{gossip.ByteSize()};
    gossip.Write(bBuffer.Begin(), bBuffer.End());

    std::cout << "Gossip content is:" << std::endl;

    std::cout << "Gossip real byte size : " << gossip.ByteSize() << std::endl;
    auto ptr = (bBuffer.Begin());
    for (size_t i = 0; i < bBuffer.Size(); ++i) {
        std::cout << (int) *ptr << " ";
        ++ptr;
        ++i;
    }
    std::cout << std::endl;

    boost::asio::ip::udp::endpoint ep(boost::asio::ip::address::from_string("127.0.0.1"), 8005);
    sock.send_to(boost::asio::buffer(bBuffer.Begin(), bBuffer.Size()), ep);

    SendGossip(sock, gossip);

    return 0;
}