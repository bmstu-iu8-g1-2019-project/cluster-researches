// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <types.hpp>
#include <deque>

MemberAddr::MemberAddr(boost::asio::ip::address addr, uint16_t port)
  : IP{std::move(addr)}
  , Port{port}
{}

size_t MemberAddr::Hasher::operator()(const MemberAddr& key) const {
    return std::hash<uint32_t>{}(key.IP.to_v4().to_uint()) ^
           (std::hash<uint16_t>{}(key.Port) << 1);
}

byte* MemberAddr::Write(byte *bBegin, const byte *bEnd) const {
    if (bEnd - bBegin < ByteSize())
        return nullptr;

    *reinterpret_cast<uint32_t*>(bBegin) = IP.to_v4().to_uint();
    bBegin += sizeof(uint32_t);

    *reinterpret_cast<uint16_t*>(bBegin) = Port;
    return bBegin + sizeof(uint16_t);
}

const byte* MemberAddr::Read(const byte *bBegin, const byte *bEnd) {
    if (bEnd - bBegin < ByteSize())
        return nullptr;

    IP = boost::asio::ip::address_v4{*reinterpret_cast<const uint32_t*>(bBegin)};
    bBegin += sizeof(uint32_t);

    Port = *reinterpret_cast<const uint16_t*>(bBegin);
    return bBegin + sizeof(uint16_t);
}

size_t MemberAddr::ByteSize() const {
    return sizeof(uint32_t) + sizeof(uint16_t);
}

nlohmann::json MemberAddr::ToJSON() const {
    auto object = nlohmann::json::object();

    object["IP"] = IP.to_string();
    object["port"] = Port;

    return std::move(object);
}

bool MemberAddr::operator==(const MemberAddr &rhs) const {
    return IP == rhs.IP && Port == rhs.Port;
}


bool TimeStamp::IsLatestThen(TimeStamp rhs) const {
    return Time >= rhs.Time;
}


MemberInfo::MemberInfo(MemberInfo::State status, uint32_t incarnation, TimeStamp time)
  : Status{status}
  , Incarnation{incarnation}
  , LastUpdate{time}
{}

byte* MemberInfo::Write(byte *bBegin, const byte *bEnd) const {
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

nlohmann::json MemberInfo::ToJSON() const {
    auto object = nlohmann::json::object();

    std::string status;
    switch (Status) {
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
        default:
            status = "unknown status";
    }
    object["status"] = status;
    object["incarnation"] = Incarnation;
    object["timestamp"] = LastUpdate.Time;

    return std::move(object);
}

bool MemberInfo::operator==(const MemberInfo &rhs) const {
    return Status == rhs.Status && Incarnation == rhs.Incarnation;
}

Member::Member(const MemberAddr& addr, const MemberInfo& info)
  : Addr{addr}
  , Info{info}
{}

byte* Member::Write(byte* bBegin, const byte* bEnd) const {
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

nlohmann::json Member::ToJSON() const {
    auto object = nlohmann::json::object();

    object["address"] = Addr.ToJSON();
    object["info"] = Info.ToJSON();

    return std::move(object);
}

bool Member::operator==(const Member &rhs) const {
    return Addr == rhs.Addr && Info == rhs.Info;
}

bool Member::IsLatestUpdatedThen(const Member& rhs) const {
    return Info.LastUpdate.IsLatestThen(rhs.Info.LastUpdate);
}

bool Member::IsStatusWorse(const Member& rhs) {
    return Info.Status > rhs.Info.Status;
}


MemberTable::MemberTable()
  : latestUpdatesSize_{0}
  , randomSubsetSize_{0}
  , rGenerator_{std::random_device{}()}
{}

MemberTable::MemberTable(size_t latestUpdatesSize, size_t randomSubsetSize)
  : MemberTable{}
{
    latestUpdatesSize_ = latestUpdatesSize;
    randomSubsetSize_ = randomSubsetSize;
}

MemberTable::MemberTable(const MemberTable& other)
  : me_{other.me_}
  , index_{other.index_}
  , set_{other.set_}
  , latestUpdates_{other.latestUpdates_}
  , latestUpdatesSize_{other.latestUpdatesSize_}
  , randomSubsetSize_{other.randomSubsetSize_}
  , mutex_{}
  , rGenerator_{other.rGenerator_}
{}

MemberTable& MemberTable::operator=(const MemberTable& other) {
    if (this != &other) {
        me_ = other.me_;
        index_ = other.index_;
        set_ = other.set_;
        latestUpdates_ = other.latestUpdates_;
        latestUpdatesSize_ = other.latestUpdatesSize_;
        randomSubsetSize_ = other.randomSubsetSize_;
    }

    return *this;
}

void MemberTable::SetMe(const Member& member) {
    std::lock_guard<std::mutex> lock{mutex_};

    me_ = member;
}

const Member& MemberTable::Me() const {
    std::lock_guard<std::mutex> lock{mutex_};

    return me_;
}

byte* MemberTable::Write(byte* bBegin, const byte* bEnd) const {
    std::lock_guard<std::mutex> lock{mutex_};

    if (!(bBegin = WriteNumberToBytes(bBegin, bEnd, set_.size())))
        return nullptr;

    for (const auto& member : set_) {
        if (!(bBegin = member.Write(bBegin, bEnd)))
            return nullptr;
    }

    return bBegin;
}

const byte* MemberTable::Read(const byte *bBegin, const byte *bEnd) {
    std::lock_guard<std::mutex> lock{mutex_};

    size_t size = 0;
    if (!(bBegin = ReadNumberFromBytes(bBegin, bEnd, size)))
        return nullptr;

    for (size_t i = 0; i < size; ++i) {
        Member member{};
        if (!(bBegin = member.Read(bBegin, bEnd)))
            return nullptr;

        Insert_(member);
    }

    return bBegin;
}

size_t MemberTable::ByteSize() const {
    std::lock_guard<std::mutex> lock{mutex_};

    return sizeof(size_t) + set_.size()*Member{}.ByteSize();
}

nlohmann::json MemberTable::ToJSON() const {
    std::lock_guard<std::mutex> lock{mutex_};

    auto array = nlohmann::json::array();

    for (const auto& member : set_) {
        array.push_back(member.ToJSON());
    }

    return std::move(array);
}

void MemberTable::Update(const Member& member) {
    std::lock_guard<std::mutex> lock{mutex_};

    Update_(member);
}

void MemberTable::Update_(const Member& member) {
    auto found = index_.find(member.Addr);
    if (found == index_.end()) {
        Insert_(member);
        return;
    }

    set_[found->second] = member;
}

// TODO(AndreevSemen): here will be gossip-update logic
// TODO              : for example, here might be solved timestamps diffs
// TODO              : or statuses' conflicts
bool MemberTable::TryUpdate_(const Member& member, Member& suspicion) {
    auto found = index_.find(member.Addr);

    if (found == index_.end()) {
        Insert_(member);
        return true;
    }

    // If new incarnation
    if (set_[found->second].Info.Incarnation < member.Info.Incarnation) {
        Update_(member);
        return true;
    }

    // If `member.Info` older
    if (set_[found->second].IsLatestUpdatedThen(member)) {
        return true;
    }

    // If `member.Status` better (we have: "dead", `member` have :"alive")
    if (set_[found->second].IsStatusWorse(member)) {
        suspicion = set_[found->second];
        return false;
    }

    // If `member.Info` is reliable
    Update_(member);
    return true;
}

std::vector<Member> MemberTable::TryUpdate(const MemberTable& table) {
    std::lock_guard<std::mutex> lock{mutex_};

    return std::move(TryUpdate_(table));
}

std::vector<Member> MemberTable::TryUpdate_(const MemberTable& table) {
    std::vector<Member> conflicts;

    Member conflict{};
    for (const auto& member : table.set_) {
        if (member == Me()) {
            continue;
        }

        if (!TryUpdate_(member, conflict)) {
            conflicts.push_back(conflict);
        }
    }

    return std::move(conflicts);
}

Member MemberTable::RandomMember() const {
    std::lock_guard<std::mutex> lock{mutex_};

    return RandomMember_();

}

Member MemberTable::RandomMember_() const {
    return set_[rGenerator_() % set_.size()];
}

MemberTable MemberTable::GetSubset() const {
    std::lock_guard<std::mutex> lock{mutex_};

    MemberTable subsetTable{};
    for (const auto& latestIndex : latestUpdates_) {
        subsetTable.Insert_(set_[latestIndex]);
    }

    for (size_t i = 0; i < randomSubsetSize_ || i < Size() - latestUpdatesSize_;) {
        auto member = RandomMember_();
        auto foundInIndex = index_.find(member.Addr);

        if (std::find(latestUpdates_.begin(), latestUpdates_.end(),
                                                foundInIndex->second) ==
            latestUpdates_.end()) {
            subsetTable.Insert_(member);
            ++i;
        }
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
    std::lock_guard<std::mutex> lock{mutex_};

    return set_.size();
}

void MemberTable::Insert_(const Member& member) {
    index_.emplace(std::make_pair(member.Addr, set_.size()));
    set_.push_back(member);
    InsertInLatest_(set_.size() - 1);
}

void MemberTable::InsertInLatest_(size_t index) {
    if (latestUpdates_.empty()) {
        latestUpdates_.push_back(index);
    }

    auto iter = latestUpdates_.begin();
    for (; iter != latestUpdates_.end(); ++iter) {
        if (set_[index].IsLatestUpdatedThen(set_[*iter])) {
            break;
        }
    }

    if (iter != latestUpdates_.end()) {
        latestUpdates_.insert(iter, index);

        if (latestUpdates_.size() > latestUpdatesSize_) {
            latestUpdates_.pop_back();
        }
    }
}

void MemberTable::DebugInsert(const Member& member) {
    Insert_(member);
}

bool MemberTable::DebugIsExists(const Member& member) const {
    return index_.find(member.Addr) != index_.end();
}


const byte* Gossip::Read(const byte *bBegin, const byte* bEnd) {
    size_t size = 0;
    if (!(bBegin = ReadNumberFromBytes(bBegin, bEnd, TTL)))
        return nullptr;
    if (!(bBegin = Owner.Read(bBegin, bEnd)))
        return nullptr;
    if (!(bBegin = Dest.Read(bBegin, bEnd)))
        return nullptr;

    return Table.Read(bBegin, bEnd);
}


byte* Gossip::Write(byte *bBegin, const byte *bEnd) const {
    if (!(bBegin = WriteNumberToBytes(bBegin, bEnd, TTL)))
        return nullptr;
    if (!(bBegin = Owner.Write(bBegin, bEnd)))
        return nullptr;
    if (!(bBegin = Dest.Write(bBegin, bEnd)))
        return nullptr;

    return Table.Write(bBegin, bEnd);
}

size_t Gossip::ByteSize() const {
    return sizeof(TTL) +
           Owner.ByteSize() +
           Dest.ByteSize() +
           sizeof(size_t) +
           Table.ByteSize();
}

nlohmann::json Gossip::ToJSON() const {
    auto object = nlohmann::json::object();

    object["TTL"] = TTL;
    object["owner"] = Owner.ToJSON();
    object["dest"] = Dest.ToJSON();
    object["table"] = Table.ToJSON();

    return std::move(object);
}

bool Gossip::operator==(const Gossip& rhs) const {
    return TTL == rhs.TTL &&
           Owner == rhs.Owner &&
           Dest == rhs.Dest &&
           Table == rhs.Table;
}
