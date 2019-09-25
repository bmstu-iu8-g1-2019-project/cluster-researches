// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <thread>

#include <membertable.hpp>
#include <gossip_behavior.hpp>

int main() {
    MemberTable table;

    std::thread gossip(Gossiping, std::ref(table));
    std::thread application{AppBridging, std::ref(table)};

    gossip.join();
    application.join();

    return 0;
}
