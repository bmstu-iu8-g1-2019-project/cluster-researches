// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <thread>

#include <membertable.hpp>
#include <behavior.hpp>

int SetupUnixSocket(const std::string& path);
int SetupUDPSocket(in_port_t port);

int main() {
    MemberTable table;

    int app_sd = SetupUnixSocket(socket_file);
    int gossip_sd;
    try {
        gossip_sd = SetupUDPSocket(gossip_port);
    } catch (...) {
        close(app_sd);

        throw;
    }

    std::thread application{AppBridging, app_sd, std::ref(table)};
    std::thread gossiping{Gossiping, gossip_sd, std::ref(table)};

    try {
        application.join();
        gossiping.join();
    } catch (...) {
        close(app_sd);
        close(gossip_sd);

        throw;
    }

    close(app_sd);
    close(gossip_sd);

    return 0;
}

int SetupUnixSocket(const std::string& path) {
    int sd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sd == -1)
        throw std::runtime_error {
                "Could not open application socket"
        };

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    unlink(path.c_str());
    std::copy(path.cbegin(), path.cend(), &addr.sun_path);

    if (bind(sd, (sockaddr*) &addr, sizeof(addr)) == -1) {
        close(sd);
        throw std::runtime_error {
                "Could not bind application socket"
        };
    }

    return sd;
}

int SetupUDPSocket(in_port_t port) {
    int sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd == -1)
        throw std::runtime_error{
                "Socket opening error"
        };

    struct sockaddr_in host_addr = {};
    host_addr.sin_family = AF_INET;
    host_addr.sin_addr.s_addr = INADDR_ANY;
    host_addr.sin_port = port;

    if (bind(sd, (const sockaddr*) &host_addr, sizeof(host_addr)) == -1) {
        close(sd);
        throw std::runtime_error{
                "Gossip socket bind error"
        };
    }

    return sd;
}
