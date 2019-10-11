// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#ifndef HEADERS_TYPES_HPP_
#define HEADERS_TYPES_HPP_

#include <string>
#include <unordered_map>
#include <vector>

#include <unistd.h>
#include <netinet/in.h>


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

struct ByteTranslatable;

struct MemberAddr;
struct MemberInfo;
struct TimeStamp;
struct Member;
struct MemberTable;
struct Gossip;


// Any class deriving this could be able converted to bytes
struct ByteTranslatable {
    virtual byte* Write(byte* bBegin, byte* bEnd) const = 0;
    virtual const byte* Read(const byte* bBegin, const byte* bEnd) = 0;
    virtual size_t ByteSize() const = 0;
};

template < typename Type >
byte* WriteNumberToBytes(byte* bBegin, byte* bEnd, Type number) {
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


struct TimeStamp {
    uint32_t Time;
};


struct MemberAddr : public ByteTranslatable {
    in_addr IP;
    in_port_t Port;

    MemberAddr() = default;
    MemberAddr(in_addr addr, in_port_t port);

    // We need it for `unordered_map` key hashing
    struct Hasher {
        size_t operator()(const MemberAddr& key) const;
    };

    byte* Write(byte* bBegin, byte* bEnd) const override ;
    const byte* Read(const byte* bBegin, const byte* bEnd) override;
    size_t ByteSize() const override;

    bool operator==(const MemberAddr& rhs) const;
};


struct MemberInfo : public ByteTranslatable {
    enum State {
        Alive = 0,
        Suspicious = 1,
        Dead = 2,
        Left = 3
    } Status;
    uint32_t Incarnation;
    TimeStamp LastUpdate;

    MemberInfo() = default;
    MemberInfo(State status, uint32_t incarnation, TimeStamp time);

    byte* Write(byte* bBegin, byte* bEnd) const override ;
    const byte* Read(const byte* bBegin, const byte* bEnd) override;
    size_t ByteSize() const override;

    bool operator==(const MemberInfo& rhs) const;
};


// Here keeps all information about node
struct Member : public ByteTranslatable {
public:
    MemberAddr Addr;
    MemberInfo Info;

public:
    Member();
    Member(const MemberAddr& addr, const MemberInfo& info);

    byte* Write(byte* bBegin, byte* bEnd) const override;
    const byte* Read(const byte *bBegin, const byte *bEnd) override;
    size_t ByteSize() const override;

    bool operator==(const Member& rhs) const;
};


class MemberTable : public ByteTranslatable {
private:
    std::unordered_map<MemberAddr, MemberInfo, MemberAddr::Hasher> table_;

public:
    MemberTable() = default;

    byte* Write(byte* bBegin, byte* bEnd) const override;
    const byte* Read(const byte *bBegin, const byte *bEnd) override;
    size_t ByteSize() const override;

    size_t Size() const;

    void Update(const Gossip& gossip);

    MemberTable GetSubset(size_t size) const;

    bool operator==(const MemberTable& rhs) const;

    void DebugInsert(const Member& member);
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


struct Gossip : public ByteTranslatable {
    uint16_t TTL = 0;
    Member Owner;
    Member Dest;
    std::vector<Member> Events;
    MemberTable Table;

    Gossip() = default;

    const byte* Read(const byte* bBegin, const byte *bEnd) override;
    byte* Write(byte* bBegin, byte* bEnd) const override;
    size_t ByteSize() const override;

    bool operator==(const Gossip& rhs) const;
};

#endif // HEADERS_TYPES_HPP_
