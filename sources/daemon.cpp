// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <thread>

#include <behavior.hpp>
#include <queue.hpp>



int main() {
    io_service ioService;
    // TODO(AndreevSemen) : change it for env var
    Socket socket(ioService, 8000, 1500);

    ThreadSaveQueue<PullGossip> pingQ;
    ThreadSaveQueue<PullGossip> ackQ;
    socket.RunGossipCatching(pingQ, ackQ);

    // TODO(AndreevSemen) : change it for env var
    Table table{MemberAddr{ip_v4::from_string("127.0.0.1"), 8000}, 5, 5};
    while (true) {
        /// Updating table with ping-pulls and sending push-acks
        {
            // TODO(AndreevSemen) : change it for env var
            auto pings = pingQ.Pop(10);

            auto conflicts = UpdateTable(table, pings);
            auto ackWaiters = GetIndexesToGossipOwners(table, pings);
            socket.SendAcks(table, std::move(ackWaiters));
        }

        /// Reacting to ack-pulls
        {
            // TODO(AndreevSemen) : change it for env var
            auto acks = ackQ.Pop(10);
            AcceptAcks(table, std::move(acks));
        }

        /// Spreading ping-pushes
        static TimeStamp lastSpread{TimeStamp::Now()};

        // TODO(AndreevSemen) : change it for env var
        // 5 seconds
        if (lastSpread.TimeDistance(TimeStamp::Now()) > milliseconds{5000}) {
            // TODO(AndreevSemen) : change it for env var
            socket.Spread(table, 10);
        }

        // Detecting failures
        static TimeStamp lastTimeoutCheck{TimeStamp::Now()};

        // TODO(AndreevSemen) : change it for env var
        // 5 seconds
        if (lastTimeoutCheck.TimeDistance(TimeStamp::Now()) > milliseconds{5000}) {
            DetectFailures(table, msTimeout, failuresBeforeDead);
        }
    }
}


