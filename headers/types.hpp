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

// Any class deriving this one could be converted to bytes
struct ByteTranslatable {
    virtual byte* InsertToBytes(byte* bBegin, byte* bEnd) const = 0;
};

struct TimeStamp {
    uint32_t Time;
};

struct MemberAddr {
    in_addr IP;
    in_port_t Port;

    // We need it for `unordered_map` key hashing
    struct Hasher {
        size_t operator()(const MemberAddr& key) const;
    };

    bool operator==(const MemberAddr& rhs) const;
};


struct MemberInfo {
    enum State {
        Alive = 0,
        Onload = 1,
        Inactive = 2,
        Dead = 3
    } Status;
    uint32_t Incarnation;
    TimeStamp LastUpdate;

    bool operator==(const MemberInfo& rhs) const;
};

// Here keeps all information about node
struct Member : public ByteTranslatable {
public:
    MemberAddr Addr;
    MemberInfo Info;

public:
    Member() = default;
    Member(const MemberAddr& addr, const MemberInfo& info);
    explicit Member(const byte* bBegin);

    byte* InsertToBytes(byte* bBegin, byte* bEnd) const override;

    bool operator==(const Member& rhs) const;
};

class MemberTable : public ByteTranslatable {
private:
    std::unordered_map<MemberAddr, MemberInfo, MemberAddr::Hasher> table_;

public:
    MemberTable() = default;
    explicit MemberTable(const byte* bBegin, size_t size);

    byte* InsertToBytes(byte* bBegin, byte* bEnd) const override;

    void Update(const Gossip& gossip);
    size_t Size() const;
};

struct Gossip : public ByteTranslatable {
    Member GossipOwner;
    std::vector<Member> Events;
    MemberTable Table;

    Gossip() = default;

    // Reads table from byte buffer
    bool Read(const byte* bBegin, const byte *bEnd);

    byte* InsertToBytes(byte* bBegin, byte* bEnd) const override;
};

#endif // HEADERS_TYPES_HPP_
