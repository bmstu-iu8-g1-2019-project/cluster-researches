// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <thread>

#include <membertable.hpp>
#include <behavior.hpp>

int main() {
    MemberTable table;

    int sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd == -1)
        throw std::runtime_error{
            "Socket opening error"
        };

    struct sockaddr_in host_addr = {};
    host_addr.sin_family = AF_INET;
    host_addr.sin_addr.s_addr = INADDR_ANY;
    host_addr.sin_port = gossip_port;

    if (bind(sd, (const sockaddr*) &host_addr, sizeof(host_addr)) == -1) {
        close(sd);
        throw std::runtime_error{
            "Gossip socket bind error"
        };
    }

    std::thread application{AppBridging, std::ref(table)};
    std::thread gossiping{Gossiping, sd, std::ref(table)};

    application.join();
    gossiping.join();

    close(sd);

    return 0;
}
