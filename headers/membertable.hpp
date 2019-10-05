// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#ifndef SOURCES_MEMBERTABLE_HPP_
#define SOURCES_MEMBERTABLE_HPP_

#include <arpa/inet.h>

#include <algorithm>
#include <deque>
#include <mutex>
#include <random>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

struct MemberAddr {
    in_addr IP;
    in_port_t Port;

    struct Hasher {
        size_t operator()(const MemberAddr& key) const;
    };

    nlohmann::json ToJson() const;
};

bool operator==(const MemberAddr& lhs, const MemberAddr& rhs);

// TODO(AndreevSemen): Incomplete struct
struct MemberInfo {
    std::string State;
    uint32_t Generation;

    nlohmann::json ToJson() const;

    bool operator==(const MemberInfo& rhs) const;
};

struct Member {
    MemberAddr Addr;
    MemberInfo Info;

    nlohmann::json ToJson() const;

    bool operator==(const Member& rhs) const;
};


class MemberTable {
private:
    std::unordered_map<MemberAddr, MemberInfo, MemberAddr::Hasher> table_;
    mutable std::mutex mutex_;

public:
    MemberTable() = default;
    MemberTable(const MemberTable& rhs);

    nlohmann::json ToJson() const;

    std::deque<MemberAddr> GenerateRoundQueue() const;
    MemberTable GetSubset(size_t size) const;

    Member operator[](const MemberAddr& addr) const;
    bool IsExist(const MemberAddr& addr) const;

    void Insert(const Member& member);
    void Update(const Member& member);
    void Erase(const MemberAddr& addr);

    size_t Size() const;
};

#endif // SOURCES_MEMBERTABLE_HPP_
