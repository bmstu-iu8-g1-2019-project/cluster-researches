// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <types.hpp>

void AddToMapIfNewer(std::unordered_map<MemberAddr, MemberInfo, MemberAddr::Hasher>& map,
                     const Member& member);


size_t MemberAddr::Hasher::operator()(const MemberAddr& key) const {
    return std::hash<uint32_t>{}(key.IP.s_addr) ^
           (std::hash<uint16_t>{}(key.Port) << 1);
}

bool MemberAddr::operator==(const MemberAddr &rhs) const {
    return IP.s_addr == rhs.IP.s_addr && Port == rhs.Port;
}


bool MemberInfo::operator==(const MemberInfo &rhs) const {
    return Status == rhs.Status && Incarnation == rhs.Incarnation;
}


Member::Member(const MemberAddr& addr, const MemberInfo& info)
  : Addr(addr)
  , Info(info)
{}

Member::Member(const byte* bBegin)
  : Addr(*reinterpret_cast<const MemberAddr*>(bBegin))
  , Info(*reinterpret_cast<const MemberInfo*>(bBegin + sizeof(MemberAddr)))
{}

bool Member::operator==(const Member &rhs) const {
    return Addr == rhs.Addr && Info == rhs.Info;
}

byte* Member::InsertToBytes(byte* bBegin, byte* bEnd) const {
    if (std::distance(bEnd, bBegin) < sizeof(Member))
        return nullptr;

    std::copy((byte*)this, (byte*)this + sizeof(Member), bBegin);
    return bBegin + sizeof(Member);
}


MemberTable::MemberTable(const byte* bBegin, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        Member member{bBegin};
        bBegin += sizeof(Member);

        table_.insert(std::make_pair(member.Addr, member.Info));
    }
}

byte* MemberTable::InsertToBytes(byte* bBegin, byte* bEnd) const {
    if (std::distance(bEnd, bBegin) < sizeof(Member)*Size() + sizeof(size_t))
        return nullptr;

    *reinterpret_cast<size_t *>(bBegin) = Size();
    bBegin += sizeof(size_t);

    for (const auto& pair : table_) {
        Member member{pair.first, pair.second};
        member.InsertToBytes(bBegin, bBegin + sizeof(Member));
        bBegin += sizeof(Member);
    }

    return bBegin;
}


// TODO(AndreevSemen): here will be gossip-update logic
// TODO              : for example, here might be solved timestamps diffs
// TODO              : or statuses' conflict problems
void MemberTable::Update(const Gossip& gossip) {
    table_[gossip.Owner.Addr] = gossip.Owner.Info;

    for (const auto& event : gossip.Events) {
        AddToMapIfNewer(table_, event);
    }

    // Unreliable logic
    for (const auto& member : gossip.Table.table_) {
        AddToMapIfNewer(table_, Member{member.first, member.second});
    }
}


size_t MemberTable::Size() const {
    return table_.size();
}


bool Gossip::Read(const byte *bBegin, const byte* bEnd) {
    if (std::distance(bEnd, bBegin) < sizeof(uint16_t))
        return false;
    TTL = *reinterpret_cast<const uint16_t*>(bBegin);
    bBegin += sizeof(uint16_t);

    if (std::distance(bEnd, bBegin) < sizeof(Member))
        return false;
    Owner = Member{bBegin};
    bBegin += sizeof(Member);

    if (std::distance(bEnd, bBegin) < sizeof(Member))
        return false;
    Dest = Member{bBegin};
    bBegin += sizeof(Member);

    if (std::distance(bEnd, bBegin) < sizeof(size_t))
        return false;
    size_t size = *reinterpret_cast<const size_t*>(bBegin);
    bBegin += sizeof(size_t);

    if (std::distance(bEnd, bBegin) < sizeof(Member)*size)
        return false;
    for (size_t i = 0; i < size; ++i) {
        Events.emplace_back(Member(bBegin));
        bBegin += sizeof(Member);
    }

    if (std::distance(bEnd, bBegin) < sizeof(size_t))
        return false;
    size = *reinterpret_cast<const size_t*>(bBegin);
    bBegin += sizeof(size_t);

    Table = MemberTable{bBegin, size};

    return true;
}


byte* Gossip::InsertToBytes(byte *bBegin, byte *bEnd) const {
    // `TTL` insertion
    if (!(bBegin = InsertNumberToBytes(bBegin, bEnd, TTL)))
        return nullptr;

    // `Owner` insertion
    if (!(bBegin = Owner.InsertToBytes(bBegin, bEnd)))
        return nullptr;

    // `Dest` insertion
    if (!(bBegin = Dest.InsertToBytes(bBegin, bEnd)))
        return nullptr;

    // `Events` size insertion
    if (!(bBegin = InsertNumberToBytes(bBegin, bEnd, Events.size())))
        return nullptr;

    // Insertion of every event form `Event`
    for (const auto& member : Events)
        if (!(bBegin = member.InsertToBytes(bBegin, bEnd)))
            return nullptr;

    // `Table` size insertion
    if (!(bBegin = InsertNumberToBytes(bBegin, bEnd, Table.Size())))
        return nullptr;

    // `Table` insertion
    bBegin = Table.InsertToBytes(bBegin, bEnd);
    return bBegin;
}

size_t Gossip::ByteSize() const {
    return sizeof(TTL) +
           sizeof(Owner) +
           sizeof(Dest) +
           sizeof(size_t) + sizeof(Member)*Events.size() +
           sizeof(size_t) + sizeof(Member)*Table.Size();
}


void AddToMapIfNewer(std::unordered_map<MemberAddr, MemberInfo, MemberAddr::Hasher>& map,
                     const Member& member) {
    auto found = map.find(member.Addr);

    if (found == map.cend() ||
        found->second.LastUpdate.Time < member.Info.LastUpdate.Time)
        map[found->first] = member.Info;
}
