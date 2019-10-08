// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#ifndef HEADERS_TYPES_HPP_
#define HEADERS_TYPES_HPP_

#include <string>
#include <unistd.h>
#include <chrono>

// It will be env variables in future
std::string socket_file{"/tmp/app.socket"};
in_port_t gossip_port = 80;
size_t spread_num = 4;
size_t max_connections = 10;
std::chrono::seconds failDetectionDelay{1};

struct TimeStamp {
    // TODO(AndreevSemen): incomplete class
};

struct Gossip {
    TimeStamp Stamp;
    Member Member;
};

class Message {
    friend class MemberTable;

public:
    enum MsgType { PING = 0, ACK = 1, GOSSIP = 2 };

private:
    MsgType msgType_;
    Member destMember_;
    Gossip gossip_;
    MemberTable table_;

    bool isValid_;

public:
    Message();
    Message(MsgType type, const Member& dest, const Gossip& gossip, const MemberTable& table);

    bool Parse(void *source, size_t size);

    bool IsValid() const;

    bool IsPing() const;
    bool IsAck() const;
    bool IsGossip() const;
};

#endif // HEADERS_TYPES_HPP_
