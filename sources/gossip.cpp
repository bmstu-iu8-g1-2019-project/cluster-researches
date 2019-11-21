// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <gossip.hpp>

PullGossip::PullGossip(const Proto::Gossip& protoGossip)
  : _owner{protoGossip.owner()}
  , _dest{protoGossip.dest()}
  , _table{protoGossip.table()}
{}

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
  : _table{table}
{}

Proto::Gossip PushGossip::ToProtoType(const Member& dest) const {
    Proto::Gossip protoGossip;
    
    protoGossip.set_allocated_owner(new Proto::Member{_table.WhoAmI().ToProtoType()});
    protoGossip.set_allocated_dest(new Proto::Member{dest.ToProtoType()});
    protoGossip.set_allocated_table(new Proto::Table{_table.ToProtoType()});

    return std::move(protoGossip);
}
