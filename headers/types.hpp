// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#ifndef HEADERS_TYPES_HPP_
#define HEADERS_TYPES_HPP_

#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <random>
#include <deque>
#include <mutex>

#include <nlohmann/json.hpp>

#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <boost/asio/ip/address.hpp>


/* Member  ------------------------> 6 + 12 = 18 B
 * |
 * |__Addr (MemberAddr)   -> 4 + 2 = 6 B
 * |  |
 * |  |__IP   (in_addr)   -> 4 B
 * |  |__Port (in_port_t) -> 2 B
 * |
 * |__Info (MemberInfo)           -> 4 + 4 + 4 = 12 B
 *    |
 *    |__Status      (enum State) -> 4 B
 *    |__Incarnation (uint32_t)   -> 4 B
 *    |__LastUpdate  (TimeStamp)  -> 4 = 4 B
 *       |
 *       |__Time (uint32_t)  -> 4 B
 * */

using byte = uint8_t;

struct MemberAddr;
struct MemberInfo;
struct TimeStamp;
struct Member;
struct MemberTable;
struct Gossip;


// Any class deriving this could be able converted to bytes
struct Translatable {
    virtual byte* Write(byte* bBegin, const byte* bEnd) const = 0;
    virtual const byte* Read(const byte* bBegin, const byte* bEnd) = 0;
    virtual size_t ByteSize() const = 0;

    virtual nlohmann::json ToJSON() const = 0;
    virtual void FromJson(const nlohmann::json& json) = 0;
};


template < typename Type >
byte* WriteNumberToBytes(byte* bBegin, const byte* bEnd, Type number) {
    if (bEnd - bBegin  < sizeof(number))
        return nullptr;

    *reinterpret_cast<Type*>(bBegin) = number;

    return bBegin + sizeof(Type);
}

template < typename Type >
const byte* ReadNumberFromBytes(const byte* bBegin, const byte* bEnd, Type& number) {
    if (bEnd - bBegin < sizeof(Type))
        return nullptr;

    number = *reinterpret_cast<const Type*>(bBegin);

    return bBegin + sizeof(Type);
}


struct MemberAddr : public Translatable {
    boost::asio::ip::address IP;
    uint16_t Port = 0;

    MemberAddr() = default;
    MemberAddr(boost::asio::ip::address addr, in_port_t port);

    // We need it for `unordered_map` key hashing
    struct Hasher {
        size_t operator()(const MemberAddr& key) const;
    };

    byte* Write(byte* bBegin, const byte* bEnd) const override ;
    const byte* Read(const byte* bBegin, const byte* bEnd) override;
    size_t ByteSize() const override;

    nlohmann::json ToJSON() const override;
    void FromJson(const nlohmann::json& json) override;

    bool operator==(const MemberAddr& rhs) const;
};


struct TimeStamp {
    std::chrono::system_clock::time_point Time;

    bool IsLatestThen(TimeStamp rhs) const;
    std::chrono::seconds Delay(TimeStamp rhs) const;

    static TimeStamp Now();
};


struct MemberInfo : public Translatable {
    enum State {
        Alive = 0,
        Suspicious = 1,
        Dead = 2,
        Left = 3,
        Unknown = -1
    } Status = Alive;
    uint32_t Incarnation = 0;
    TimeStamp LastUpdate;

    MemberInfo() = default;
    MemberInfo(State status, uint32_t incarnation, TimeStamp time);

    byte* Write(byte* bBegin, const byte* bEnd) const override ;
    const byte* Read(const byte* bBegin, const byte* bEnd) override;
    size_t ByteSize() const override;

    nlohmann::json ToJSON() const override;
    void FromJson(const nlohmann::json& json) override;

    bool operator==(const MemberInfo& rhs) const;
};


// Here keeps all information about node
struct Member : public Translatable {
public:
    MemberAddr Addr;
    MemberInfo Info;

public:
    Member() = default;
    Member(const MemberAddr& addr, const MemberInfo& info);

    byte* Write(byte* bBegin, const byte* bEnd) const override;
    const byte* Read(const byte *bBegin, const byte *bEnd) override;
    size_t ByteSize() const override;

    nlohmann::json ToJSON() const override;
    void FromJson(const nlohmann::json& json) override;

    bool IsLatestUpdatedThen(const Member& rhs) const;
    bool IsStatusWorse(const Member& rhs) const;

    bool operator==(const Member& rhs) const;
};


class MemberTable : public Translatable {
private:
    Member me_;

    std::unordered_map<MemberAddr, size_t, MemberAddr::Hasher> index_;
    std::vector<Member> set_;

    std::vector<size_t> latestUpdates_;
    // Size of subset part with latest updates
    size_t latestUpdatesSize_;
    // Size of subset part with random members
    size_t randomSubsetSize_;

    mutable std::mutex mutex_;

    mutable std::mt19937 rGenerator_;

public:
    MemberTable();
    explicit MemberTable(size_t subTableSize, size_t randomSubsetSize);

    MemberTable(const MemberTable& other);
    MemberTable& operator=(const MemberTable& rhs);

    void SetMe(const Member& member);
    const Member& Me() const;

    byte* Write(byte* bBegin, const byte* bEnd) const override;
    const byte* Read(const byte *bBegin, const byte *bEnd) override;
    size_t ByteSize() const override;

    nlohmann::json ToJSON() const override;
    void FromJson(const nlohmann::json& json) override;

    size_t Size() const;

    // Just updates member in table without checks
    void Update(const Member& member);
    std::vector<Member> TryUpdate(const MemberTable& table);

    Member RandomMember() const;
    MemberTable GetSubset() const;

    std::deque<Member> GetDestQueue() const;

    bool operator==(const MemberTable& rhs) const;

    void DebugInsert(const Member& member);
    bool DebugIsExists(const Member& member) const;

private:
    // `Func_` means that function is private

    size_t Size_() const;
    void Update_(const Member& member);
    // If there's status conflict, `suspicion` contains worst of statuses
    bool TryUpdate_(const Member& member, Member& suspicion);
    std::vector<Member> TryUpdate_(const MemberTable& table);

    void Insert_(const Member& member);
    void InsertInLatest_(size_t index);

    Member RandomMember_() const;
};


/* Gossip  -------------------------> 2 + 18 * (1 + 1 + EventsSize + TableSize)
 * |
 * |__TTL   (uint16_t)             -> 2 B
 * |__Owner (Member)               -> 18 B
 * |__Dest  (Member)               -> 18 B
 * |
 * |__Events (std::vector<Member>) -> 18 B * EventsSize
 * |  |
 * |  |__Member[0]
 * |  |__Member[1]
 * |   ......
 * |   ......
 * |  |__Member[size - 2]
 * |  |__Member[size - 1]
 * |
 * |__Table (MemberTable)          -> 18 B * TableSize
 *    |_______________________________
 *    | MemberAddr[0] | MemberInfo[0] |
 *    | MemberAddr[0] | MemberInfo[0] |
 *              .............
 *              .............
 *              .............
 *    | MemberAddr[0] | MemberInfo[0] |
 *    | MemberAddr[0] | MemberInfo[0] |
 *    |_______________________________|
 *
 * */


struct Gossip : public Translatable {
    enum GossipType {
        Spread = 0,
        Ping = 1,
        Ack = 2,
        Default = -1
    } Type = Default;

    Member Owner;
    Member Dest;
    MemberTable Table;

    Gossip() = default;

    const byte* Read(const byte* bBegin, const byte *bEnd) override;
    byte* Write(byte* bBegin, const byte* bEnd) const override;
    size_t ByteSize() const override;

    nlohmann::json ToJSON() const override;
    void FromJson(const nlohmann::json& json) override;

    bool operator==(const Gossip& rhs) const;
};


struct Variables {
    MemberAddr Address;
    size_t TableLatestUpdatesSize;
    size_t TableRandomMembersSize;
    size_t SpreadNumber;
    size_t ReceivingBufferSize;
    std::chrono::seconds PingInterval;
    std::string UNIXSocketPath;

    nlohmann::json NodeList;
};

#endif // HEADERS_TYPES_HPP_
