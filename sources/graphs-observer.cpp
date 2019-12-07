// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <config.hpp>
#include <behavior.hpp>
#include <matrix.hpp>

int main(int argc, char** argv) {
    Config config{argv[1], argv[2], argv[3]};

    boost::asio::io_service ioService;
    Socket socket{ioService, config.ObserverPort(), config.BufferSize()};

    ThreadSaveQueue<PullGossip> pullQ;
    socket.RunObserving(pullQ);

    socket.RunKiller(Member{MemberAddr(config.ObserverIP(), config.ObserverPort())});

    MemberMatrix matrix;
    while (true) {
        auto pulls = pullQ.Pop(config.ObserveNum());

        for (auto& pull : pulls) {
            matrix.Push(std::move(pull));
        }

        matrix.SearchAbsolutelyDead();
    }
}

