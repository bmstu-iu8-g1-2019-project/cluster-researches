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

MemberAddr::MemberAddr(const gossip::MemberAddr& protoAddr)
  : _IP{ip_v4(protoAddr.ip())}
  , _port{static_cast<uint16_t>(protoAddr.port())}
{}

gossip::MemberAddr MemberAddr::ToProtoType() const {
    gossip::MemberAddr protoAddr;

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

ip_v4& MemberAddr::IP() {
    return const_cast<ip_v4&>(const_cast<const MemberAddr*>(this)->IP());
}

port_t& MemberAddr::Port() {
    return const_cast<port_t&>(const_cast<const MemberAddr*>(this)->Port());
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

TimeStamp::TimeStamp(const gossip::TimeStamp& timeStamp)
  : _time_point{milliseconds{timeStamp.time()}}
{}

gossip::TimeStamp TimeStamp::ToProtoType() const {
    using namespace std::chrono;
    gossip::TimeStamp protoTimeStamp;

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

TimeStamp TimeStamp::Zero() noexcept {
    using namespace std::chrono;
    TimeStamp ts;
    ts._time_point = time_point_cast<milliseconds>(system_clock::from_time_t(0));

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

MemberInfo::MemberInfo(const gossip::MemberInfo& info) {
    switch (info.status()) {
        case gossip::MemberInfo::ALIVE :
            _status = Alive;
            break;
        case gossip::MemberInfo::SUSPICIOUS :
            _status = Suspicious;
            break;
        case gossip::MemberInfo::DEAD :
            _status = Dead;
            break;
    }

    _incarnation = info.incarnation();
    _TS_updated = TimeStamp{info.time_stamp()};
}

gossip::MemberInfo MemberInfo::ToProtoType() const {
    gossip::MemberInfo protoInfo;

    switch (_status) {
        case Alive :
            protoInfo.set_status(gossip::MemberInfo::ALIVE);
            break;
        case Suspicious :
            protoInfo.set_status(gossip::MemberInfo::SUSPICIOUS);
            break;
        case Dead :
            protoInfo.set_status(gossip::MemberInfo::DEAD);
    }

    protoInfo.set_incarnation(_incarnation);
    protoInfo.set_allocated_time_stamp(new gossip::TimeStamp{_TS_updated.ToProtoType()});

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
    NewTimeStamp();
}

void MemberInfo::NewTimeStamp() {
    _TS_updated = TimeStamp::Now();
}

bool MemberInfo::IsStatusWorthThan(const MemberInfo& oth) const {
    return _status > oth._status;
}

const MemberInfo::NodeState& MemberInfo::Status() const {
    return _status;
}

/*const incarnation_t& MemberInfo::Incarnation() const {
    return _incarnation;
}*/

const TimeStamp& MemberInfo::LatestUpdate() const {
    return _TS_updated;
}

/*MemberInfo::NodeState& MemberInfo::Status() {
    return const_cast<MemberInfo::NodeState&>(const_cast<const MemberInfo*>(this)->Status());
}

incarnation_t& MemberInfo::Incarnation() {
    return const_cast<incarnation_t&>(const_cast<const MemberInfo*>(this)->Incarnation());
}

TimeStamp& MemberInfo::LatestUpdate() {
    return const_cast<TimeStamp&>(const_cast<const MemberInfo*>(this)->LatestUpdate());
}*/


Member::Member(MemberAddr addr)
  : _addr{std::move(addr)}
  , _info{}
{}

Member::Member(Member&& oth) noexcept
  : _addr{std::move(oth._addr)}
  , _info{std::move(oth._info)}
{}

Member::Member(const gossip::Member& member)
  : _addr{member.addr()}
  , _info{member.info()}
{}

gossip::Member Member::ToProtoType() const {
    gossip::Member protoMember;

    protoMember.set_allocated_addr(new gossip::MemberAddr{_addr.ToProtoType()});
    protoMember.set_allocated_info(new gossip::MemberInfo{_info.ToProtoType()});

    return std::move(protoMember);
}

/*void Member::IncreaseIncarnation() {
    _info.IncreaseIncarnation();
}

void Member::NewTimeStamp() {
    _info.NewTimeStamp();
}

bool Member::IsStatusWorthThan(const Member& oth) const {
    return _info.IsStatusWorthThan(oth._info);
}

bool Member::IsSameNode(const Member& oth) const {
    return _addr == oth._addr;*/
}

