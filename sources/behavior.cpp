// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <behavior.hpp>

void AppBridging(int sd, MemberTable& table) {
    while (42) {
        listen(sd, 1);

        int app_sd = accept(sd,nullptr, nullptr);
        if (app_sd == -1) {
            close(sd);
            throw std::runtime_error{
                "Could not accept application"
            };
        }

        std::string json{table.ToJson().dump()};
        send(app_sd, json.c_str(), json.size(), 0);

        close(app_sd);
    }
}

void Gossiping(int sd, MemberTable& table) {
    // TODO(AndreevSemen): Make here thread-shared container with relevant self-gossips

    std::thread self{FailureDetection, sd, std::ref(table)};
    std::thread outside{Distribution, sd, std::ref(table)};

    self.join();
    outside.join();
}
