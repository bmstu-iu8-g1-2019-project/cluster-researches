// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#ifndef HEADERS_BEHAVIOR_HPP_
#define HEADERS_BEHAVIOR_HPP_

#include <stdexcept>
#include <thread>
#include <deque>
#include <iostream>
#include <mutex>
#include <fstream>

#include <boost/asio.hpp>

#include <types.hpp>
#include <buffer.hpp>

template < typename Type >
class ThreadSaveQueue { // rename
private:
    std::deque<Type> queue_; // -> QUEUE
    mutable std::mutex mutex_;

public:
    void Push(const Type& value) {
        std::lock_guard lock{mutex_};
        queue_.push_back(value);
    }

    void Push(std::deque<Type>&& values) {
        std::lock_guard lock{mutex_};
        queue_.insert(queue_.end(), values.begin(), values.end());
    }

    bool Empty() const {
        std::lock_guard lock{mutex_};
        return queue_.empty();
    }

    std::deque<Type> Free() {
        std::lock_guard lock{mutex_};

        std::deque<Type> bucket{queue_};
        queue_.clear();// move

        return bucket;
    }
};

// try to use 'typedef' or 'using'
Variables SetupConfig(const std::string& configPath);
udpSocket SetupSocket(boost::asio::io_service& ioService, uint16_t port);
void GossipsCatching(udpSocket& sock, ThreadSaveQueue<Gossip>& spread,
                                                         ThreadSaveQueue<Gossip>& fd);
void FailureDetection(udpSocket& sock, MemberTable& table, ThreadSaveQueue<Gossip>& fdQueue,
                                                                              ThreadSaveQueue<Member>& conflictQueue);
std::deque<Member> UpdateTable(MemberTable& table, const std::deque<Gossip>& queue);
Gossip GenerateGossip(MemberTable& table, Gossip::GossipType type=Gossip::GossipType::Default);
void SendGossip(udpSocket&, const Gossip& gossip);
void SpreadGossip(udpSocket& sock, const MemberTable& table, std::deque<Member>& destQueue, Gossip& gossip, size_t destNum);

void AppConnector(const MemberTable& table, uint16_t port);

#endif // HEADERS_BEHAVIOR_HPP_
