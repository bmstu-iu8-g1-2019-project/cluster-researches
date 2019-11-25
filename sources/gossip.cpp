// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <gossip.hpp>

PullGossip::PullGossip(const Proto::Gossip& protoGossip)
  : _type{(protoGossip.type() == Proto::Gossip::Ping) ? Ping : Ack}
  , _owner{protoGossip.owner()}
  , _dest{protoGossip.dest()}
  , _table{protoGossip.table()}
{}

PullGossip::PullGossip(PullGossip&& oth) noexcept
  : _type{oth._type}
  , _owner{std::move(oth._owner)}
  , _dest{std::move(oth._dest)}
  , _table{std::move(oth._table)}
{}

MessageType PullGossip::Type() const {
    return _type;
}

const Member& PullGossip::Owner() const {
    return _owner;
}

const Member& PullGossip::Dest() const {
    return _dest;
}

PullTable&& PullGossip::MoveTable() {
    return std::move(_table);
}


PushGossip::PushGossip(PushTable&& table)
  : _table{std::move(table)}
{}

Proto::Gossip PushGossip::ToProtoType(MessageType type) const {
    Proto::Gossip protoGossip;

    switch (type) {
        case MessageType::Ping :
            protoGossip.set_type(Proto::Gossip::Ping);
            break;
        case MessageType ::Ack :
            protoGossip.set_type(Proto::Gossip::Ack);
            break;
    }

    protoGossip.set_allocated_owner(new Proto::Member{_table.WhoAmI().ToProtoType()});
    protoGossip.set_allocated_table(new Proto::Table{_table.ToProtoType()});

    return std::move(protoGossip);
}

Proto::Gossip MakeProtoGossip(PushTable&& table, MessageType type) {
    Proto::Gossip protoGossip = PushGossip{std::move(table)}.ToProtoType(type);
    return std::move(protoGossip);
}
