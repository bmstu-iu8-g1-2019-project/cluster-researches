// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#ifndef INCLUDE_MEMBER_HPP_
#define INCLUDE_MEMBER_HPP_

#include <chrono>

#include <boost/asio.hpp>

#include <protobuf.pb.h>

typedef boost::asio::ip::address_v4 ip_v4;
typedef uint16_t port_t;
typedef size_t incarnation_t;
typedef std::chrono::milliseconds milliseconds;
typedef std::chrono::time_point<std::chrono::system_clock> time_point;

class MemberAddr {
public:
    class Hasher {
    public:
        size_t operator()(const MemberAddr& addr) const;
    };

private:
    ip_v4 _IP;
    port_t _port = 0;

public:
    MemberAddr() = default;
    MemberAddr(ip_v4 ip, port_t port);
    MemberAddr(const MemberAddr& oth);
    MemberAddr(MemberAddr&& oth) noexcept;

    explicit MemberAddr(const gossip::MemberAddr& protoAddr);
    gossip::MemberAddr ToProtoType() const;

    MemberAddr& operator=(const MemberAddr& rhs);
    MemberAddr& operator=(MemberAddr&& rhs) noexcept;

    const ip_v4& IP() const;
    const port_t& Port() const;

    ip_v4& IP();
    port_t& Port();

    bool operator==(const MemberAddr& rhs) const;
};

class TimeStamp {
private:
    time_point _time_point;

public:
    TimeStamp();
    TimeStamp(const TimeStamp& oth) = default;
    TimeStamp(TimeStamp&& oth) noexcept;

    explicit TimeStamp(const gossip::TimeStamp& timeStamp);
    gossip::TimeStamp ToProtoType() const;

    TimeStamp& operator=(const TimeStamp& rhs) = default;
    TimeStamp& operator=(TimeStamp&& rhs) noexcept;

    bool OlderThan(const TimeStamp& rhs) const;
    bool OlderThan(const TimeStamp&& rhs) const;

    milliseconds TimeDistance(const TimeStamp& rhs) const;
    milliseconds TimeDistance(const TimeStamp&& rhs) const;

    static TimeStamp Now() noexcept;
    static TimeStamp Zero() noexcept;
};

class MemberInfo {
public:
    enum NodeState {
        Alive = 0,
        Suspicious = 1,
        Dead = 2
    };

private:
    NodeState _status;
    incarnation_t _incarnation;
    TimeStamp _TS_updated;

public:
    MemberInfo();
    MemberInfo(const MemberInfo& oth) = default;
    MemberInfo(MemberInfo&& oth) noexcept;

    explicit MemberInfo(const gossip::MemberInfo& info);
    gossip::MemberInfo ToProtoType() const;

    MemberInfo& operator=(const MemberInfo& rhs) = default;
    MemberInfo& operator=(MemberInfo&& rhs) noexcept;

    const NodeState& Status() const;
    const TimeStamp& LatestUpdate() const;

    void IncreaseIncarnation();
    void NewTimeStamp();

    bool IsStatusWorthThan(const MemberInfo& oth) const;
};

class Member {
private:
    MemberAddr _addr;
    MemberInfo _info;

public:
    explicit Member(MemberAddr addr);
    Member(const Member& oth) = default;
    Member(Member&& oth) noexcept;

    explicit Member(const gossip::Member& member);
    gossip::Member ToProtoType() const;

    const MemberAddr& Addr() const;
    const MemberInfo& Info() const;

    MemberAddr& Addr();
    MemberInfo& Info();
};

#endif // INCLUDE_MEMBER_HPP_
