// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <thread>

#include <behavior.hpp>
#include <observer.hpp>
#include <queue.hpp>

int main(int argc, char** argv) {
    if (argc < 3) {
        return 1;
    }

    Config conf{argv[1], argv[2], argv[3]};

    io_service ioService;
    port_t port = (conf.Containerization()) ? conf.DockerPort() : conf.Port();
    std::cout << "Binded on port : " << port << std::endl;
    Socket socket(ioService, port, conf.BufferSize());

    ThreadSaveQueue<PullGossip> pingQ;
    ThreadSaveQueue<PullGossip> ackQ;
    socket.RunGossipCatching(pingQ, ackQ);

    Table table{conf.MoveTable()};
    while (true) {
        /// Updating table with ping-pulls
        {
            auto pings = pingQ.Pop(conf.PingProcessingNum());
            UpdateTable(table, pings);
        }

        // Sending push-acks
        {
            socket.SendAcks(table);
        }

        /// Reacting to ack-pulls
        {
            auto acks = ackQ.Pop(conf.AckProcessingNum());
            AcceptAcks(table, std::move(acks));
        }

        /// Spreading ping-pushes
        static TimeStamp lastSpread{TimeStamp::Now()};

        if (lastSpread.TimeDistance(TimeStamp::Now()) > conf.SpreadRepetition()) {
            socket.Spread(table, conf.SpreadDestinationNum());
            lastSpread = TimeStamp::Now();
        }

        // Detecting failures
        static TimeStamp lastTimeoutCheck{TimeStamp::Now()};

        if (lastTimeoutCheck.TimeDistance(TimeStamp::Now()) > conf.FDRepetition()) {
            table.DetectFailures(conf.FDTimout(), conf.FailuresBeforeDead());
            lastTimeoutCheck = TimeStamp::Now();
        }

        // Notifying observer
        static TimeStamp lastObserve{TimeStamp::Now()};

        if (lastObserve.TimeDistance(TimeStamp::Now()) > conf.ObserveRepetition()) {
            socket.NotifyObserver(conf.ObserverIP(), conf.ObserverPort(), table);
        }
    }
}


