// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#ifndef INCLUDE_GOSSIP_HPP_
#define INCLUDE_GOSSIP_HPP_

#include <protobuf.pb.h>

#include <member.hpp>
#include <table.hpp>

enum MessageType {
    Ping = 0,
    Ack = 1
};

class PullGossip {
private:
    MessageType _type;

    Member _owner;
    Member _dest;

    PullTable _table;

public:
    explicit PullGossip(const Proto::Gossip& protoGossip);
    PullGossip(PullGossip&& oth) noexcept;

    MessageType Type() const;

    const Member& Owner() const;
    const Member& Dest() const;

    PullTable&& MoveTable();
};

class PushGossip {
private:
    PushTable _table;

public:
    explicit PushGossip(PushTable&& table);

    Proto::Gossip ToProtoType(const Member& dest, MessageType type) const;
};

#endif // INCLUDE_GOSSIP_HPP_
