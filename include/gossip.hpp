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
    // тип слуха (Ping/Ack)
    MessageType _type;

    Member _owner;
    Member _dest;

    PullTable _table;

public:
    // создание из `Proto::Gossip`
    explicit PullGossip(const Proto::Gossip& protoGossip);
    // `PullGossip` перемещаемый, но некопируемый
    PullGossip(PullGossip&& oth) noexcept;

    MessageType Type() const;

    const Member& Owner() const;
    const Member& Dest() const;

    // перемещает таблицу
    PullTable&& MoveTable();
};

class PushGossip {
private:
    // `Type` задается при создании `Proto::Gossip`
    // `Owner` можно получить из `PushTable`
    // `Dest` задается перед отправкой `Proto::Gossip`
    PushTable _table;

public:
    explicit PushGossip(PushTable&& table);

    // создается `Proto::Gossip` без инициализированного `Dest`
    Proto::Gossip ToProtoType(MessageType type) const;
};


Proto::Gossip MakeProtoGossip(PushTable&& table, MessageType type);

#endif // INCLUDE_GOSSIP_HPP_
