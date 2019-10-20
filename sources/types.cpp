// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <types.hpp>

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


MemberTable::MemberTable()
  : rGenerator_(std::random_device{}())
{}

nlohmann::json MemberTable::ToJSON() const {
    nlohmann::json array = nlohmann::json::array();

    for (const auto& member : set_) {
        nlohmann::json memberJson;

        memberJson["addr"]["IP"] = inet_ntoa(member.Addr.IP);
        memberJson["addr"]["port"] = member.Addr.Port;

        std::string status;
        switch (member.Info.Status) {
            case MemberInfo::State::Alive:
                status = "alive";
                break;
            case MemberInfo::State::Suspicious:
                status = "suspicious";
                break;
            case MemberInfo::State::Dead:
                status = "dead";
                break;
            case MemberInfo::State::Left:
                status = "left";
                break;
        }
        memberJson["info"]["status"] = status;
        memberJson["info"]["incarnation"] = member.Info.Incarnation;

        array.push_back(memberJson);
    }

    return std::move(array);
}

byte* MemberTable::Write(byte* bBegin, byte* bEnd) const {
    if (!(bBegin = WriteNumberToBytes(bBegin, bEnd, Size())))
        return nullptr;

    for (const auto& member : set_) {
        if (!(bBegin = member.Write(bBegin, bEnd)))
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

        Insert(member);
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
    auto found = index_.find(gossip.Owner.Addr);
    if (found == index_.end()) {
        Insert(gossip.Owner);
    } else {
        set_[found->second] = gossip.Owner;
    }

    for (const auto& event : gossip.Events) {
        UpdateRecordIfNewer(event);
    }

    // Unreliable logic
    for (const auto& member : gossip.Table.set_) {
        UpdateRecordIfNewer(member);
    }
}

Member MemberTable::RandomMember() const {
    return set_[rGenerator_() % Size()];
}

MemberTable MemberTable::GetSubset(size_t size) const {
    auto setCopy{set_};
    std::shuffle(setCopy.begin(), setCopy.end(), rGenerator_);

    MemberTable subsetTable;
    for (size_t i = 0; i < size; ++i) {
        subsetTable.Insert(setCopy[i]);
    }

    return subsetTable;
}

bool MemberTable::operator==(const MemberTable& rhs) const {
    for (const auto& pair : index_) {
        auto othFound = rhs.index_.find(pair.first);
        if (othFound == rhs.index_.cend())
            return false;
        if (!(set_[pair.second] == rhs.set_[othFound->second]))
            return false;
    }

    return true;
}


size_t MemberTable::Size() const {
    return set_.size();
}

void MemberTable::Insert(const Member& member) {
    index_.emplace(std::make_pair(member.Addr, set_.size()));
    set_.push_back(member);
}

void MemberTable::UpdateRecordIfNewer(const Member& member) {
    auto found = index_.find(member.Addr);
    if (found == index_.end()) {
        Insert(member);
    } else if (member.Info.LastUpdate.Time < set_[found->second].Info.LastUpdate.Time ||
               member.Info.Incarnation < set_[found->second].Info.Incarnation) {
        set_[found->second].Info = member.Info;
    }
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
