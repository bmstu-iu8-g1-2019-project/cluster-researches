// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <config.hpp>
#include <behavior.hpp>
#include <gossip.hpp>

int main(int argc, char** argv) {
    Config config{argv[1], argv[2], argv[3]};

    // киллер шлет сообщения от лица обзервера
    Member observer{MemberAddr{config.ObserverIP(), config.ObserverPort()}};

    // инициализация киллер-слуха
    Proto::Gossip protoGossip;
    protoGossip.set_type(Proto::Gossip::Ping);
    protoGossip.set_allocated_owner(new Proto::Member{observer.ToProtoType()});
    protoGossip.set_allocated_table(new Proto::Table{});


    boost::asio::io_service ioService;
    Socket socket(ioService, config.Port(), 0);

    while (true) {
        std::cout << "Sacrifice address :";
        std::string sacrificeAddress;
        std::cin >> sacrificeAddress;

        std::cout << "Sacrifice port :";
        port_t sacrificePort;
        std::cin >> sacrificePort;

        std::cout << std::endl;

        Member dest{MemberAddr{ip_v4::from_string(sacrificeAddress), sacrificePort}};

        SetDest(protoGossip, dest);
        socket.SendProtoGossip(protoGossip);
    }
}

