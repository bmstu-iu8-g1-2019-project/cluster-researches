// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <membertable.hpp>


nlohmann::json MemberAddr::ToJson() const {
    nlohmann::json json;
    json["ip"] = inet_ntoa(IP);
    json["port"] = Port;

    return json;
}

size_t MemberAddr::Hasher::operator()(const MemberAddr& key) const {
    return std::hash<uint32_t>{}(key.IP.s_addr) ^
           (std::hash<uint16_t>{}(key.Port) << 1);
}
bool operator==(const MemberAddr& lhs, const MemberAddr& rhs) {
    return lhs.IP.s_addr == rhs.IP.s_addr && lhs.Port == rhs.Port;
}

nlohmann::json MemberInfo::ToJson() const {
    nlohmann::json json;
    json["state"] = State;
    json["generation"] = Generation;

    return json;
}

bool MemberInfo::operator==(const MemberInfo &rhs) const {
    return State == rhs.State && Generation == rhs.Generation;
}

nlohmann::json Member::ToJson() const {
    nlohmann::json json;
    json["addr"] = Addr.ToJson();
    json["info"] = Info.ToJson();

    return json;
}

bool Member::operator==(const Member& rhs) const {
    return Addr == rhs.Addr && Info == rhs.Info;
}

MemberTable::MemberTable(const MemberTable& rhs)
  : table_{rhs.table_}
  , mutex_{std::mutex{}}
{}

nlohmann::json MemberTable::ToJson() const {
    std::lock_guard<std::mutex> lock{mutex_};

    nlohmann::json json = nlohmann::json::array();
    for (const auto& pair : table_)
        json.push_back(Member{pair.first, pair.second}.ToJson());

    return json;
}

std::deque<MemberAddr> MemberTable::GenerateRoundQueue() const {
    std::deque<MemberAddr> deque;

    std::unique_lock<std::mutex> lock(mutex_);
    for (const auto &member : table_) {
        deque.push_back(member.first);
    }
    lock.unlock();

    std::random_device rng{};
    std::mt19937 generator{rng()};
    std::shuffle(deque.begin(), deque.end(), generator);

    return deque;
}

MemberTable MemberTable::GetSubset(size_t size) const {
    std::deque<MemberAddr> randomQueue;
    randomQueue = GenerateRoundQueue();

    MemberTable subset;
    for (size_t i = 0; i < size && i < Size(); ++i)
        subset.Insert(Member{randomQueue[i], table_.find(randomQueue[i])->second});

    return subset;
}

Member MemberTable::operator[](const MemberAddr& addr) const {
    std::lock_guard<std::mutex> lock{mutex_};
    return Member{addr, table_.find(addr)->second};
}

bool MemberTable::IsExist(const MemberAddr& addr) const {
    std::lock_guard<std::mutex> lock{mutex_};
    return table_.find(addr) != table_.end();
}

void MemberTable::Insert(const Member& member) {
    std::lock_guard<std::mutex> lock{mutex_};
    table_.insert(std::make_pair(member.Addr, member.Info));
}

void MemberTable::Update(const Member& member) {
    std::lock_guard<std::mutex> lock{mutex_};
    table_.find(member.Addr)->second = member.Info;
}

void MemberTable::Erase(const MemberAddr& addr) {
    std::lock_guard<std::mutex> lock{mutex_};
    table_.erase(addr);
}

size_t MemberTable::Size() const {
    std::lock_guard<std::mutex> lock{mutex_};
    return table_.size();
}
