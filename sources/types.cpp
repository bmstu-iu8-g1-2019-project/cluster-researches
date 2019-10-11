// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <types.hpp>

void AddToMapIfNewer(std::unordered_map<MemberAddr, MemberInfo, MemberAddr::Hasher>& map,
                     const Member& member);

MemberAddr::MemberAddr(in_addr addr, in_port_t port)
  : IP{addr}
  , Port{port}
{}

size_t MemberAddr::Hasher::operator()(const MemberAddr& key) const {
    return std::hash<uint32_t>{}(key.IP.s_addr) ^
           (std::hash<uint16_t>{}(key.Port) << 1);
}

byte* MemberAddr::Write(byte *bBegin, byte *bEnd) const {
    if (bEnd - bBegin <
        sizeof(MemberAddr::IP) + sizeof(MemberAddr::Port))
        return nullptr;

    *reinterpret_cast<in_addr*>(bBegin) = IP;
    bBegin += sizeof(in_addr);

    *reinterpret_cast<in_port_t*>(bBegin) = Port;
    return bBegin + sizeof(in_port_t);
}

const byte* MemberAddr::Read(const byte *bBegin, const byte *bEnd) {
    if (bEnd - bBegin < ByteSize())
        return nullptr;

    IP = *reinterpret_cast<const in_addr*>(bBegin);
    bBegin += sizeof(in_addr);

    Port = *reinterpret_cast<const in_port_t*>(bBegin);
    return bBegin + sizeof(in_port_t);
}

size_t MemberAddr::ByteSize() const {
    return sizeof(MemberAddr::IP) + sizeof(MemberAddr::Port);
}

bool MemberAddr::operator==(const MemberAddr &rhs) const {
    return IP.s_addr == rhs.IP.s_addr && Port == rhs.Port;
}


MemberInfo::MemberInfo(MemberInfo::State status, uint32_t incarnation, TimeStamp time)
  : Status{status}
  , Incarnation{incarnation}
  , LastUpdate{time}
{}

byte* MemberInfo::Write(byte *bBegin, byte *bEnd) const {
    if (bEnd - bBegin < ByteSize())
        return nullptr;

    *reinterpret_cast<State*>(bBegin) = Status;
    bBegin += sizeof(Status);

    *reinterpret_cast<uint32_t*>(bBegin) = Incarnation;
    bBegin += sizeof(Incarnation);

    *reinterpret_cast<TimeStamp*>(bBegin) = LastUpdate;
    return bBegin + sizeof(LastUpdate);
}

const byte* MemberInfo::Read(const byte *bBegin, const byte *bEnd) {
    if (bEnd - bBegin < ByteSize())
        return nullptr;

    Status = *reinterpret_cast<const State*>(bBegin);
    bBegin += sizeof(Status);

    Incarnation = *reinterpret_cast<const uint32_t*>(bBegin);
    bBegin += sizeof(Incarnation);

    LastUpdate = *reinterpret_cast<const TimeStamp*>(bBegin);
    return bBegin + sizeof(LastUpdate);
}

size_t MemberInfo::ByteSize() const {
    return sizeof(Status) + sizeof(Incarnation) + sizeof(LastUpdate);
}

bool MemberInfo::operator==(const MemberInfo &rhs) const {
    return Status == rhs.Status && Incarnation == rhs.Incarnation;
}

Member::Member()
  : Addr{}
  , Info{}
{}

Member::Member(const MemberAddr& addr, const MemberInfo& info)
  : Addr{addr}
  , Info{info}
{}

bool Member::operator==(const Member &rhs) const {
    return Addr == rhs.Addr && Info == rhs.Info;
}

byte* Member::Write(byte* bBegin, byte* bEnd) const {
    if (!(bBegin = Addr.Write(bBegin, bEnd)))
        return nullptr;

    return Info.Write(bBegin, bEnd);
}

const byte* Member::Read(const byte *bBegin, const byte *bEnd) {
    if (!(bBegin = Addr.Read(bBegin, bEnd)))
        return nullptr;

    return Info.Read(bBegin, bEnd);
}

size_t Member::ByteSize() const {
    return Addr.ByteSize() + Info.ByteSize();
}

byte* MemberTable::Write(byte* bBegin, byte* bEnd) const {
    if (!(bBegin = WriteNumberToBytes(bBegin, bEnd, Size())))
        return nullptr;

    for (const auto& pair : table_) {
        if (!(bBegin = Member{pair.first, pair.second}.Write(bBegin, bEnd)))
            return nullptr;
    }

    return bBegin;
}

const byte* MemberTable::Read(const byte *bBegin, const byte *bEnd) {
    size_t size = 0;
    if (!(bBegin = ReadNumberFromBytes(bBegin, bEnd, size)))
        return nullptr;

    for (size_t i = 0; i < size; ++i) {
        Member member{};
        if (!(bBegin = member.Read(bBegin, bEnd)))
            return nullptr;
        table_.insert(std::make_pair(member.Addr, member.Info));
    }

    return bBegin;
}

size_t MemberTable::ByteSize() const {
    return sizeof(size_t) + Size()*Member{}.ByteSize();
}


// TODO(AndreevSemen): here will be gossip-update logic
// TODO              : for example, here might be solved timestamps diffs
// TODO              : or statuses' conflicts
void MemberTable::Update(const Gossip& gossip) {
    table_[gossip.Owner.Addr] = gossip.Owner.Info;

    for (const auto& event : gossip.Events) {
        AddToMapIfNewer(table_, event);
    }

    // Unreliable logic
    for (const auto& member : gossip.Table.table_) {
        AddToMapIfNewer(table_, Member{member.first,
                                                  member.second});
    }
}

MemberTable MemberTable::GetSubset(size_t size) const {
    for (const auto& pair : table_) {

    }
}

bool MemberTable::operator==(const MemberTable& rhs) const {
    return table_ == rhs.table_;
}


size_t MemberTable::Size() const {
    return table_.size();
}


const byte* Gossip::Read(const byte *bBegin, const byte* bEnd) {
    size_t size = 0;
    if (!(bBegin = ReadNumberFromBytes(bBegin, bEnd, TTL)))
        return nullptr;
    if (!(bBegin = Owner.Read(bBegin, bEnd)))
        return nullptr;
    if (!(bBegin = Dest.Read(bBegin, bEnd)))
        return nullptr;
    if (!(bBegin = ReadNumberFromBytes(bBegin, bEnd, size)))
        return nullptr;

    for (size_t i = 0; i < size; ++i) {
        Member member{};
        if (!(bBegin = member.Read(bBegin, bEnd)))
            return nullptr;
        Events.push_back(member);
    }

    return Table.Read(bBegin, bEnd);
}


byte* Gossip::Write(byte *bBegin, byte *bEnd) const {
    if (!(bBegin = WriteNumberToBytes(bBegin, bEnd, TTL)))
        return nullptr;
    if (!(bBegin = Owner.Write(bBegin, bEnd)))
        return nullptr;
    if (!(bBegin = Dest.Write(bBegin, bEnd)))
        return nullptr;
    if (!(bBegin = WriteNumberToBytes(bBegin, bEnd, Events.size())))
        return nullptr;

    for (const auto& member : Events)
        if (!(bBegin = member.Write(bBegin, bEnd)))
            return nullptr;

    return Table.Write(bBegin, bEnd);
}

size_t Gossip::ByteSize() const {
    return sizeof(TTL) +
           Owner.ByteSize() +
           Dest.ByteSize() +
           sizeof(size_t) + Owner.ByteSize()*Events.size() +
           Table.ByteSize();
}

bool Gossip::operator==(const Gossip& rhs) const {
    return TTL == rhs.TTL &&
           Owner == rhs.Owner &&
           Dest == rhs.Dest &&
           Events == rhs.Events &&
           Table == rhs.Table;
}


void AddToMapIfNewer(std::unordered_map<MemberAddr, MemberInfo, MemberAddr::Hasher>& map,
                     const Member& member) {
    auto found = map.find(member.Addr);

    if (found == map.cend() ||
        found->second.LastUpdate.Time < member.Info.LastUpdate.Time)
        map[found->first] = member.Info;
}
