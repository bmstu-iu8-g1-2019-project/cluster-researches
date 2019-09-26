// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <gossiping.hpp>

void SelfGossiping(int sd, MemberTable& table) {
    while (42) {
        if (rand()) return;

        auto queue = table.GenerateRoundQueue();

        while (42) {
            if (queue.empty()) break;

            auto dest = queue.front();
            queue.pop_front();

            sockaddr_in dest_addr{};
            dest_addr.sin_family = AF_INET;

        }
    }
}

void OutsideGossiping(int sd, MemberTable& table) {
    while (42) {
        if (rand()) return;

        listen(sd, max_connections);

        sockaddr_in addr{};
        socklen_t len = 0;
        int node_sd = accept(sd, (sockaddr*) &addr, &len);


    }
}
