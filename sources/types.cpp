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

    return object;
}

void MemberAddr::FromJson(const nlohmann::json& json) {
    IP = boost::asio::ip::address_v4::from_string(std::string{json["IP"]});
    Port = json["port"];
}

bool MemberAddr::operator==(const MemberAddr &rhs) const {
    return IP == rhs.IP && Port == rhs.Port;
}


bool TimeStamp::IsLatestThen(TimeStamp rhs) const {
    return Time >= rhs.Time;
}

std::chrono::seconds TimeStamp::Delay(TimeStamp rhs) const {
    return std::chrono::duration_cast<std::chrono::seconds>(rhs.Time - Time);
}

TimeStamp TimeStamp::Now() {
    return TimeStamp{std::chrono::system_clock::now()};
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
    // object["timestamp"] = LastUpdate.Time;

    return object;
}

void MemberInfo::FromJson(const nlohmann::json& json) {
    std::string status{json["status"]};

    if (status == "alive") {
        Status = MemberInfo::State::Alive;
    } else if (status == "suspicious") {
        Status = MemberInfo::State::Suspicious;
    } else if (status == "dead") {
        Status = MemberInfo::State::Dead;
    } else if (status == "left") {
        Status = MemberInfo::State::Left;
    } else {
        Status = MemberInfo::State::Unknown;
    }

    Incarnation = json["incarnation"];
    // LastUpdate.Time = json["timestamp"];
}

int MemberInfo::IncarnationDiff(const MemberInfo& oth) const {
    // Необходимо преобразование типов
    return static_cast<int>(this->Incarnation) - oth.Incarnation;
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

    return object;
}

void Member::FromJson(const nlohmann::json& json) {
    Addr.FromJson(json["address"]);
    Info.FromJson(json["info"]);
}

bool Member::operator==(const Member &rhs) const {
    return Addr == rhs.Addr && Info == rhs.Info;
}

bool Member::IsLatestUpdatedThen(const Member& rhs) const {
    return Info.LastUpdate.IsLatestThen(rhs.Info.LastUpdate);
}

bool Member::IsStatusWorse(const Member& rhs) const {
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

    return array;
}

void MemberTable::FromJson(const nlohmann::json& json) {
    std::lock_guard<std::mutex> lock{mutex_};

    for (const auto& jsonMember : json) {
        Member member{};
        member.FromJson(jsonMember);
        Insert_(member);
    }
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

    // Если до этого member был неизвестен
    if (found == index_.end()) {
        Insert_(member);
        return true;
    }

    // If new incarnation
    if (set_[found->second].Info.IncarnationDiff(member.Info) < 0) {
        Update_(member);
        return true;
    }

    // If `member.Info` older
    if (set_[found->second].IsLatestUpdatedThen(member)) {
        return true;
    }

    // If `member.Status` better (we have: "dead", `member` have :"alive")
    // We need check truth
    if (set_[found->second].IsStatusWorse(member)) {
        suspicion = set_[found->second];
        return false;
    }

    // If other reliable case `member.Info`
    Update_(member);
    return true;
}

std::vector<Member> MemberTable::TryUpdate(const MemberTable& table) {
    std::lock_guard<std::mutex> lock{mutex_};

    return TryUpdate_(table);
}

std::vector<Member> MemberTable::TryUpdate_(const MemberTable& table) {
    std::vector<Member> conflicts;

    Member conflict{};
    for (const auto& member : table.set_) {
        if (member.Addr == me_.Addr) {
            continue;
        }

        if (!TryUpdate_(member, conflict)) {
            conflicts.push_back(conflict);
        }
    }

    return conflicts;
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

    // Создаем вектор vec = {0, 1, ..., size -1}, из которого уберем индексы последних обновлений
    // В итоге получим индексы всех остальных (самых старых обновлений)
    std::vector<size_t> earliestUpdates{};
    earliestUpdates.resize(Size_(), 0);
    std::iota(earliestUpdates.begin(), earliestUpdates.end(), 0);

    MemberTable subsetTable{};
    for (const auto& latestIndex : latestUpdates_) {
        subsetTable.Insert_(set_[latestIndex]);

        // Делаем MemberTable
        earliestUpdates.erase(std::find(earliestUpdates.begin(),
                                        earliestUpdates.end(),
                                        latestIndex));
    }

    std::shuffle(earliestUpdates.begin(), earliestUpdates.end(), rGenerator_);

    /*
    size_t insertedRandomly = 0;
    for (size_t i = 0; i < Size_(); ++i) {
        auto member = RandomMember_();
        auto foundInIndex = index_.find(member.Addr);

        if (std::find(latestUpdates_.begin(), latestUpdates_.end(),
                                                foundInIndex->second) ==
            latestUpdates_.end()) {
            subsetTable.Insert_(member);
            ++insertedRandomly;

            if (insertedRandomly == randomSubsetSize_) {
                break;
            }
        }
    }
    */

    return subsetTable;
}

std::deque<Member> MemberTable::GetDestQueue() const {
    std::lock_guard lock{mutex_};

    std::deque<Member> queue;
    for (const auto& member : set_) {
        queue.push_back(member);
    }

    std::shuffle(queue.begin(), queue.end(), rGenerator_);

    return queue;
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

    return Size_();
}

size_t MemberTable::Size_() const {
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
        return;
    }

    // Самая старая запись должна оказаться вверху
    auto comp = [=](size_t lhs, size_t rhs) {
        return !set_[lhs].IsLatestUpdatedThen(set_[rhs]);
    };

    if (latestUpdates_.size() < latestUpdatesSize_) { // Добавляем в конец, если не достигли макс. длины массива
        latestUpdates_.push_back(index);
    } else { // спускаем самый старый элемент и ставим на его место новый
        std::pop_heap(latestUpdates_.begin(), latestUpdates_.end(), comp);
        latestUpdates_.back() = index;
    }

    // Запихивает новый элемент на нужное место
    std::push_heap(latestUpdates_.begin(), latestUpdates_.end(), comp);
}


const byte* Gossip::Read(const byte *bBegin, const byte* bEnd) {
    if (!(bBegin = ReadNumberFromBytes(bBegin, bEnd, Type)))
        return nullptr;
    if (!(bBegin = Owner.Read(bBegin, bEnd)))
        return nullptr;
    if (!(bBegin = Dest.Read(bBegin, bEnd)))
        return nullptr;

    return Table.Read(bBegin, bEnd);
}


byte* Gossip::Write(byte *bBegin, const byte *bEnd) const {
    if (!(bBegin = WriteNumberToBytes(bBegin, bEnd, Type)))
        return nullptr;
    if (!(bBegin = Owner.Write(bBegin, bEnd)))
        return nullptr;
    if (!(bBegin = Dest.Write(bBegin, bEnd)))
        return nullptr;

    return Table.Write(bBegin, bEnd);
}

size_t Gossip::ByteSize() const {
    return sizeof(Type) +
           Owner.ByteSize() +
           Dest.ByteSize() +
           Table.ByteSize();
}

nlohmann::json Gossip::ToJSON() const {
    auto object = nlohmann::json::object();

    std::string strType;
    switch (Type) {
        case GossipType::Ping :
            strType = "ping";
            break;
        case GossipType::Ack :
            strType = "ack";
            break;
        case GossipType::Spread :
            strType = "spread";
            break;
        default :
            strType = "default";
    }
    object["Type"] = strType;
    object["owner"] = Owner.ToJSON();
    object["dest"] = Dest.ToJSON();
    object["table"] = Table.ToJSON();

    return object;
}

void Gossip::FromJson(const nlohmann::json& json) {
    if (json["type"] == "ping") {
        Type = GossipType::Ping;
    } else if (json["type"] == "ack") {
        Type = GossipType::Ack;
    } else if (json["type"] == "spread") {
        Type = GossipType::Spread;
    } else {
        Type = GossipType::Default;
    }
    Owner.FromJson(json["owner"]);
    Dest.FromJson(json["dest"]);
    Table.FromJson(json["table"]);
}

bool Gossip::operator==(const Gossip& rhs) const {
    return Type == rhs.Type &&
           Owner == rhs.Owner &&
           Dest == rhs.Dest &&
           Table == rhs.Table;
}
