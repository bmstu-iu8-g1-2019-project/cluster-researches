// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <behavior.hpp>

void AppCommunicator(int sd, MemberTable& table) {
    while (42) {
        listen(sd, 1);

        int app_sd = accept(sd,nullptr, nullptr);
        if (app_sd == -1) {
            close(sd);
            throw std::runtime_error{
                "Could not accept application request"
            };
        }

        std::string json{table.ToJson().dump()};
        send(app_sd, json.c_str(), json.size(), 0);

        close(app_sd);
    }
}

void Gossiping(int sd, MemberTable& table) {
    std::deque<Gossip> discoveredGossips;

    std::thread self{FailureDetection, sd, std::ref(table), std::ref(discoveredGossips)};
    std::thread outside{Distribution, sd, std::ref(table), std::ref(discoveredGossips)};

    self.join();
    outside.join();
}
