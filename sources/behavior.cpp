// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <behavior.hpp>

void AppBridging(MemberTable& table) {
    int sd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sd == -1)
        throw std::runtime_error {
                "Could not open application socket"
        };

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    unlink(socket_file.c_str());
    std::copy(socket_file.cbegin(), socket_file.cend(), &addr.sun_path);

    if (bind(sd, (sockaddr*) &addr, sizeof(addr)) == -1) {
        close(sd);
        throw std::runtime_error {
                "Could not bind application socket"
        };
    }

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
    // Make here container with relevant self-gossips

    std::thread self{SelfGossiping, sd, std::ref(table)};
    std::thread outside{OutsideGossiping, sd, std::ref(table)};

    self.join();
    outside.join();
}
