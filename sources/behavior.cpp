// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <behavior.hpp>

int SetupSocket(in_port_t port) {
    int sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd == -1)
        return -1;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sd, (sockaddr*) &addr, sizeof(addr)) == -1) {
        return -1;
    }

    return sd;
}

void GossipsCatching(int sd, size_t listenQueueLength, GossipQueue& queue) {
    size_t buffSize = 1500;
    byte inBuff[1500]{};
    while (true) {
        listen(sd, listenQueueLength);

        sockaddr_in addr{};
        size_t msgLength = recvfrom(sd, inBuff, buffSize, 0, (sockaddr*) &addr, nullptr);



    }
}
