// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <thread>

#include <behavior.hpp>
#include <queue.hpp>

int main(int argc, char** argv) {
    Config conf{argv[1]};

    io_service ioService;
    Socket socket(ioService, conf.Port(), conf.BufferSize());

    ThreadSaveQueue<PullGossip> pingQ;
    ThreadSaveQueue<PullGossip> ackQ;
    socket.RunGossipCatching(pingQ, ackQ);

    Table table{conf.MoveTable()};
    while (true) {
        /// Updating table with ping-pulls and sending push-acks
        {
            auto pings = pingQ.Pop(conf.PingProcessingNum());

            auto conflicts = UpdateTable(table, pings);
            auto ackWaiters = GetIndexesToGossipOwners(table, pings);
            socket.SendAcks(table, std::move(ackWaiters));
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
    }
}


