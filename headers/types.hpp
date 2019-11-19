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
#include <boost/asio.hpp>

#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>


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
using udpSocket = boost::asio::ip::udp::socket;
using ip4 = boost::asio::ip::address_v4;
using ip4Endpoint = boost::asio::ip::udp::endpoint;

struct MemberAddr;
struct MemberInfo;
struct TimeStamp;
struct Member;
struct MemberTable;
struct Gossip;


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


struct MemberAddr {
    boost::asio::ip::address IP;
    uint16_t Port = 0;

    MemberAddr() = default;
    MemberAddr(boost::asio::ip::address addr, in_port_t port);

    // We need it for `unordered_map` key hashing
    struct Hasher {
        size_t operator()(const MemberAddr& key) const;
    };

    byte* Write(byte* bBegin, const byte* bEnd) const;
    const byte* Read(const byte* bBegin, const byte* bEnd);
    size_t ByteSize() const;

    nlohmann::json ToJSON() const;
    void FromJson(const nlohmann::json& json);

    bool operator==(const MemberAddr& rhs) const;
};


struct TimeStamp {
    std::chrono::system_clock::time_point Time;

    bool IsLatestThen(TimeStamp rhs) const;
    std::chrono::seconds Delay(TimeStamp rhs) const;

    static TimeStamp Now();
};


struct MemberInfo {
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

    byte* Write(byte* bBegin, const byte* bEnd) const;
    const byte* Read(const byte* bBegin, const byte* bEnd);
    size_t ByteSize() const;

    nlohmann::json ToJSON() const;
    void FromJson(const nlohmann::json& json);

    // Возвращает (this->Incarnation - oth.Incarnation)
    int IncarnationDiff(const MemberInfo& oth) const;

    bool operator==(const MemberInfo& rhs) const;
};


// Here keeps all information about node
struct Member {
public:
    MemberAddr Addr;
    MemberInfo Info;

public:
    Member() = default;
    Member(const MemberAddr& addr, const MemberInfo& info);

    byte* Write(byte* bBegin, const byte* bEnd) const;
    const byte* Read(const byte *bBegin, const byte *bEnd);
    size_t ByteSize() const;

    nlohmann::json ToJSON() const;
    void FromJson(const nlohmann::json& json);

    bool IsLatestUpdatedThen(const Member& rhs) const;
    bool IsStatusWorse(const Member& rhs) const;

    bool operator==(const Member& rhs) const;
};


class MemberTable {
private:
    Member me_;

    std::unordered_map<MemberAddr, size_t, MemberAddr::Hasher> index_;
    std::vector<Member> set_;

    // Массив с самыми поздними обновлениями (сортируется по TimeStamp)
    std::vector<size_t> latestUpdates_; // THINK ABOUT 'HEAP'

    // Количество последних обновлений попадающих в подтаблицу (subset)
    size_t latestUpdatesSize_;
    // Количество рандомных записей попадающих в подтаблицу (subset)
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

    byte* Write(byte* bBegin, const byte* bEnd) const;
    const byte* Read(const byte *bBegin, const byte *bEnd);
    size_t ByteSize() const;

    nlohmann::json ToJSON() const;
    void FromJson(const nlohmann::json& json);

    size_t Size() const;

    // Update обновляет запись в таблице без всяких проверок
    void Update(const Member& member);
    // TryUpdate обновляет новой таблице (сщ всем проверками)
    std::vector<Member> TryUpdate(const MemberTable& table);

    Member RandomMember() const;
    MemberTable GetSubset() const;

    // Возвращает перемешанный список всех членов
    std::deque<Member> GetDestQueue() const;

    bool operator==(const MemberTable& rhs) const;

private:
    // `Func_` means that function is private

    size_t Size_() const;
    void Update_(const Member& member);

    //
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


struct Gossip {
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

    const byte* Read(const byte* bBegin, const byte *bEnd);
    byte* Write(byte* bBegin, const byte* bEnd) const;
    size_t ByteSize() const;

    nlohmann::json ToJSON() const;
    void FromJson(const nlohmann::json& json);

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
