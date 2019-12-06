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
  , _port{oth._port}
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

MemberAddr MemberAddr::FromJSON(const nlohmann::json& json) {
    auto ip = ip_v4::from_string(json["ip"]);
    uint16_t port = json["port"];

    return std::move(MemberAddr{ip, port});
}

nlohmann::json MemberAddr::ToJSON() const {
    auto json = nlohmann::json::object();

    json["ip"] = _IP.to_string();
    json["port"] = _port;

    return std::move(json);
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
  : _time{std::chrono::duration_cast<milliseconds>(
              steady_clock::now().time_since_epoch()
          ).count()}
{}

TimeStamp::TimeStamp(milliseconds ms)
  : _time{ms.count()}
{}

TimeStamp::TimeStamp(TimeStamp&& oth) noexcept
  : _time{oth._time}
{
    oth._time = 0;
}

TimeStamp::TimeStamp(const Proto::TimeStamp& timeStamp)
  : _time{timeStamp.time()}
{}

Proto::TimeStamp TimeStamp::ToProtoType() const {
    Proto::TimeStamp protoTimeStamp;
    protoTimeStamp.set_time(_time);

    return std::move(protoTimeStamp);
}

milliseconds TimeStamp::Time() const {
    return milliseconds{_time};
}

TimeStamp& TimeStamp::operator=(TimeStamp&& rhs) noexcept {
    if (this != &rhs) {
        _time = rhs._time;

        rhs._time = 0;
    }

    return *this;
}

bool TimeStamp::OlderThan(const TimeStamp& rhs) const {
    return this->OlderThan(const_cast<const TimeStamp&&>(rhs));
}

bool TimeStamp::OlderThan(const TimeStamp&& rhs) const {
    return _time <= rhs._time;
}

milliseconds TimeStamp::TimeDistance(const TimeStamp&& rhs) const {
    return milliseconds{std::abs(_time - rhs._time)};
}

TimeStamp TimeStamp::Now() noexcept {
    return std::move(TimeStamp{});
}


MemberInfo::MemberInfo()
  : _status{Alive}
  , _incarnation{0}
  , _TS_updated{milliseconds{0}}
{}

MemberInfo::MemberInfo(MemberInfo&& oth) noexcept
  : _status{oth._status}
  , _incarnation{oth._incarnation}
  , _TS_updated{std::move(oth._TS_updated)}
{
    oth._status = Alive;
    oth._incarnation = 0;
}

MemberInfo::MemberInfo(const Proto::MemberInfo& info)
  : _incarnation{info.incarnation()}
  , _TS_updated{info.time_stamp()}
{
    NodeState status = Alive;
    switch (info.status()) {
        case Proto::MemberInfo::ALIVE :
            status = Alive;
            break;
        case Proto::MemberInfo::SUSPICIOUS :
            status = Suspicious;
            break;
        case Proto::MemberInfo::DEAD :
            status = Dead;
            break;
    }
    _status = status;
}

Proto::MemberInfo MemberInfo::ToProtoType() const {
    Proto::MemberInfo protoInfo;

    Proto::MemberInfo::State status = Proto::MemberInfo::ALIVE;
    switch (_status) {
        case Alive :
            status = Proto::MemberInfo::ALIVE;
            break;
        case Suspicious :
            status = Proto::MemberInfo::SUSPICIOUS;
            break;
        case Dead :
            status = Proto::MemberInfo::DEAD;
            break;
    }
    protoInfo.set_status(status);

    protoInfo.set_incarnation(_incarnation);
    protoInfo.set_allocated_time_stamp(new Proto::TimeStamp{_TS_updated.ToProtoType()});

    return std::move(protoInfo);
}

MemberInfo MemberInfo::FromJSON(const nlohmann::json& json) {
    MemberInfo info;

    NodeState status;
    if (json["status"] == "alive") {
        status = Alive;
    } else if (json["status"] == "suspicious") {
        status = Suspicious;
    } else if (json["status"] == "dead") {
        status = Dead;
    }
    info._status = status;
    info._incarnation = json["incarnation"];
    info._TS_updated = milliseconds{json["latest-update"]};

    return std::move(info);
}

nlohmann::json MemberInfo::ToJSON() const {
    auto json = nlohmann::json::object();

    std::string status;
    switch (_status) {
        case Alive :
            status = "alive";
            break;
        case Suspicious :
            status = "suspicious";
            break;
        case Dead :
            status = "dead";
            break;
    }
    json["status"] = status;

    json["incarnation"] = _incarnation;
    json["latest-update"] = _TS_updated.Time().count();

    return std::move(json);
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
    return _status < oth._status;
}

bool MemberInfo::IsIncarnationMoreThan(const MemberInfo& oth) const {
    return _incarnation > oth._incarnation;
}

const MemberInfo::NodeState& MemberInfo::Status() const {
    return _status;
}

MemberInfo::NodeState& MemberInfo::Status() {
    return const_cast<NodeState&>(const_cast<const MemberInfo*>(this)->Status());
}

const incarnation_t& MemberInfo::Incarnation() const {
    return _incarnation;
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

Member& Member::operator=(Member&& rhs) noexcept {
    if (this != & rhs) {
        _addr = std::move(rhs._addr);
        _info = std::move(rhs._info);
    }

    return *this;
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

Member Member::FromJSON(const nlohmann::json& json) {
    Member member{MemberAddr::FromJSON(json["address"])};
    member._info = MemberInfo::FromJSON(json["info"]);

    return std::move(member);
}

nlohmann::json Member::ToJSON() const {
    auto json = nlohmann::json::object();

    json["address"] = _addr.ToJSON();
    json["info"] = _info.ToJSON();

    return std::move(json);
}