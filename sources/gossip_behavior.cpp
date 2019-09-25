// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <gossip_behavior.hpp>

void AppBridging(MemberTable& table) {
    int sd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sd == -1)
        throw std::runtime_error {
                "Could not open socket"
        };

    sockaddr_un addr;
    std::fill(&addr, &addr + sizeof(addr), 0);
    addr.sun_family = AF_UNIX;
    unlink(socket_file.c_str());
    std::copy(socket_file.cbegin(), socket_file.cend(), &addr.sun_path);

    if (bind(sd, (struct sockaddr*) &addr, sizeof(addr)) == -1) {
        close(sd);
        throw std::runtime_error {
                "Could not bind socket"
        };
    }

    while (42) {
        listen(sd, 1);

        int app_sock = accept(sd,nullptr, nullptr);
        if (app_sock == -1) {
            close(sd);
            throw std::runtime_error{
                "Could not accept"
            };
        }

        std::string json{table.ToJson().dump()};
        send(app_sock, json.c_str(), json.size(), 0);

        close(app_sock);
    }
}

void Gossiping(MemberTable& table) {
    
}
