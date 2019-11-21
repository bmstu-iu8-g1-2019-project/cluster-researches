// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <member.hpp>

MemberAddr::MemberAddr(ip_v4 ip, port_t port)
  : _IP{std::move(ip)}
  , _port{port}
{}

MemberAddr::MemberAddr(const MemberAddr& oth)
  : MemberAddr(oth._IP, oth._port)
{}

MemberAddr::MemberAddr(MemberAddr&& oth) noexcept
  : _IP{std::move(oth._IP)}
{
    oth._port = 0;
}

MemberAddr::MemberAddr(const Proto::MemberAddr& protoAddr)
  : _IP{ip_v4(protoAddr.ip())}
  , _port{static_cast<uint16_t>(protoAddr.port())}
{}

Proto::MemberAddr MemberAddr::ToProtoType() const {
    Proto::MemberAddr protoAddr;

    protoAddr.set_ip(_IP.to_uint());
    protoAddr.set_port(_port);

    return std::move(protoAddr);
}

MemberAddr& MemberAddr::operator=(const MemberAddr& rhs) {
    if (this != &rhs) {
        _IP = rhs._IP;
        _port = rhs._port;
    }

    return *this;
}

MemberAddr& MemberAddr::operator=(MemberAddr&& rhs) noexcept {
    if (this != &rhs) {
        _IP = std::move(rhs._IP);
        _port = rhs._port;

        rhs._port = 0;
    }

    return *this;
}

const ip_v4& MemberAddr::IP() const {
    return _IP;
}

const port_t& MemberAddr::Port() const {
    return _port;
}

bool MemberAddr::operator==(const MemberAddr& rhs) const {
    return _IP == rhs._IP && _port == rhs._port;
}

size_t MemberAddr::Hasher::operator()(const MemberAddr& key) const {
    return std::hash<uint32_t>{}(key._IP.to_uint()) ^
           (std::hash<uint16_t>{}(key._port) << 1);
}


TimeStamp::TimeStamp()
  : _time_point{}
{}

TimeStamp::TimeStamp(TimeStamp&& oth) noexcept
  : _time_point{oth._time_point}
{
    oth._time_point = {};
}

TimeStamp::TimeStamp(const Proto::TimeStamp& timeStamp)
  : _time_point{milliseconds{timeStamp.time()}}
{}

Proto::TimeStamp TimeStamp::ToProtoType() const {
    using namespace std::chrono;
    Proto::TimeStamp protoTimeStamp;

    protoTimeStamp.set_time(time_point_cast<milliseconds>(_time_point).time_since_epoch().count());

    return std::move(protoTimeStamp);
}

TimeStamp& TimeStamp::operator=(TimeStamp&& rhs) noexcept {
    if (this != &rhs) {
        _time_point = rhs._time_point;
        rhs._time_point = {};
    }

    return *this;
}

bool TimeStamp::OlderThan(const TimeStamp& rhs) const {
    return this->OlderThan(const_cast<const TimeStamp&&>(rhs));
}

bool TimeStamp::OlderThan(const TimeStamp&& rhs) const {
    return _time_point <= rhs._time_point;
}

milliseconds TimeStamp::TimeDistance(const TimeStamp& rhs) const {
    return this->TimeDistance(const_cast<const TimeStamp&&>(rhs));
}

milliseconds TimeStamp::TimeDistance(const TimeStamp&& rhs) const {
    return (_time_point > rhs._time_point) ?
           milliseconds{std::chrono::duration_cast<milliseconds>(_time_point - rhs._time_point)} :
           milliseconds{std::chrono::duration_cast<milliseconds>(rhs._time_point - _time_point)};
}

TimeStamp TimeStamp::Now() noexcept {
    using namespace std::chrono;
    TimeStamp ts;
    ts._time_point = time_point_cast<milliseconds>(system_clock::now());

    return std::move(ts);
}


MemberInfo::MemberInfo()
  : _status{Alive}
  , _incarnation{0}
  , _TS_updated{}
{}

MemberInfo::MemberInfo(MemberInfo&& oth) noexcept
  : _status{oth._status}
  , _incarnation{oth._incarnation}
  , _TS_updated{std::move(oth._TS_updated)}
{
    oth._status = Alive;
    oth._incarnation = 0;
}

MemberInfo::MemberInfo(const Proto::MemberInfo& info) {
    switch (info.status()) {
        case Proto::MemberInfo::ALIVE :
            _status = Alive;
            break;
        case Proto::MemberInfo::SUSPICIOUS :
            _status = Suspicious;
            break;
        case Proto::MemberInfo::DEAD :
            _status = Dead;
            break;
    }

    _incarnation = info.incarnation();
    _TS_updated = TimeStamp{info.time_stamp()};
}

Proto::MemberInfo MemberInfo::ToProtoType() const {
    Proto::MemberInfo protoInfo;

    switch (_status) {
        case Alive :
            protoInfo.set_status(Proto::MemberInfo::ALIVE);
            break;
        case Suspicious :
            protoInfo.set_status(Proto::MemberInfo::SUSPICIOUS);
            break;
        case Dead :
            protoInfo.set_status(Proto::MemberInfo::DEAD);
    }

    protoInfo.set_incarnation(_incarnation);
    protoInfo.set_allocated_time_stamp(new Proto::TimeStamp{_TS_updated.ToProtoType()});

    return std::move(protoInfo);
}

MemberInfo& MemberInfo::operator=(MemberInfo&& rhs) noexcept {
    if (this != &rhs) {
        _status = rhs._status;
        _incarnation = rhs._incarnation;
        _TS_updated = std::move(rhs._TS_updated);

        rhs._status = Alive;
        rhs._incarnation = 0;
    }

    return *this;
}

void MemberInfo::IncreaseIncarnation() {
    ++_incarnation;
    SetNewTimeStamp();
}

void MemberInfo::SetNewTimeStamp() {
    _TS_updated = TimeStamp::Now();
}

bool MemberInfo::IsStatusBetterThan(const MemberInfo& oth) const {
    return _status > oth._status;
}

bool MemberInfo::IsIncarnationMoreThan(const MemberInfo& oth) const {
    return _incarnation > oth._incarnation;
}

const MemberInfo::NodeState& MemberInfo::Status() const {
    return _status;
}

const TimeStamp& MemberInfo::LatestUpdate() const {
    return _TS_updated;
}


Member::Member(MemberAddr addr)
  : _addr{std::move(addr)}
  , _info{}
{}

Member::Member(Member&& oth) noexcept
  : _addr{std::move(oth._addr)}
  , _info{std::move(oth._info)}
{}

Member::Member(const Proto::Member& member)
  : _addr{member.addr()}
  , _info{member.info()}
{}

Proto::Member Member::ToProtoType() const {
    Proto::Member protoMember;

    protoMember.set_allocated_addr(new Proto::MemberAddr{_addr.ToProtoType()});
    protoMember.set_allocated_info(new Proto::MemberInfo{_info.ToProtoType()});

    return std::move(protoMember);
}

const MemberAddr& Member::Addr() const {
    return _addr;
}

const MemberInfo& Member::Info() const {
    return _info;
}

MemberInfo& Member::Info() {
    return const_cast<MemberInfo&>(const_cast<const Member*>(this)->Info());
}
