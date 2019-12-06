// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#ifndef INCLUDE_MEMBER_HPP_
#define INCLUDE_MEMBER_HPP_

#include <chrono>

#include <nlohmann/json.hpp>
#include <boost/asio.hpp>

#include <protobuf.pb.h>

typedef boost::asio::ip::address_v4 ip_v4;
typedef uint16_t port_t;
typedef size_t incarnation_t;
typedef std::chrono::milliseconds milliseconds;


/*
 * класс представления адреса ноды
 * */
class MemberAddr {
public:
    // функтор хеширования для std::unordered_map
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

    // функции преобразования для `Proto::MemberAddr`
    explicit MemberAddr(const Proto::MemberAddr& protoAddr);
    Proto::MemberAddr ToProtoType() const;

    // функции преобразования для `JSON`
    static MemberAddr FromJSON(const nlohmann::json& json);
    nlohmann::json ToJSON() const;

    MemberAddr& operator=(const MemberAddr& rhs);
    MemberAddr& operator=(MemberAddr&& rhs) noexcept;

    // адрес инициализуется при конструировании и больше не меняется
    const ip_v4& IP() const;
    const port_t& Port() const;

    bool operator==(const MemberAddr& rhs) const;
};

/*
 * класс представления момента времени
 */
class TimeStamp {
    typedef std::chrono::steady_clock steady_clock;

private:
    // время хранится в количестве миллисекундах от нулевого времени
    int64_t _time;

public:
    TimeStamp();
    TimeStamp(milliseconds ms);
    TimeStamp(const TimeStamp& oth) = default;
    TimeStamp(TimeStamp&& oth) noexcept;

    // функции преобразования для `Proto::MemberAddr`
    explicit TimeStamp(const Proto::TimeStamp& timeStamp);
    Proto::TimeStamp ToProtoType() const;

    milliseconds Time() const;

    TimeStamp& operator=(const TimeStamp& rhs) = default;
    TimeStamp& operator=(TimeStamp&& rhs) noexcept;

    // `true`  -- если момент `this` был до `rhs`
    // `false` -- иначе
    bool OlderThan(const TimeStamp& rhs) const;
    bool OlderThan(const TimeStamp&& rhs) const;

    // возвращает промежуток времени от `this` до `rhs`
    milliseconds TimeDistance(const TimeStamp&& rhs) const;

    // создает объект описывающий текущий момент времени
    static TimeStamp Now() noexcept;
};


/*
 * класс представления информации о ноде
 * - состояние (Alive -- все хорошо, нет никаких неполадок
 *              Suspicious -- нода подозрительно себя ведет,
 *                            т.е. находится в пограничном состоянии
 *              Dead -- нода не отвечает)
 * - инкарнация (счетчик, который может увеличить только нода,
 *               которую описывает данная структура;
 *               если счетчик больше, чем известный, значит нода сама
 *               сгенерировала событие и оно однозначно более валидное)
 * - последнее обновление (момент времени, когда была создана информация)
 * */
class MemberInfo {
public:
    enum NodeState {
        Alive = 0,
        Suspicious = 1,
        Dead = 2
    };

private:
    // состояние
    NodeState _status;
    // инкарнация
    incarnation_t _incarnation;
    // последнее обновление
    TimeStamp _TS_updated;

public:
    MemberInfo();
    MemberInfo(const MemberInfo& oth) = default;
    MemberInfo(MemberInfo&& oth) noexcept;

    // функции преобразования для `Proto::MemberInfo`
    explicit MemberInfo(const Proto::MemberInfo& info);
    Proto::MemberInfo ToProtoType() const;

    // функции преобразования для `JSON`
    static MemberInfo FromJSON(const nlohmann::json& json);
    nlohmann::json ToJSON() const;

    MemberInfo& operator=(const MemberInfo& rhs) = default;
    MemberInfo& operator=(MemberInfo&& rhs) noexcept;

    const NodeState& Status() const;
    const TimeStamp& LatestUpdate() const;

    // единственное поле, которое может быть обновлено явно
    NodeState& Status();
    const incarnation_t& Incarnation() const;

    // величивает инкарнацию (может быть применено только к информации о ноде "Me")
    // так же устанавливает новый `TimeStamp`
    void IncreaseIncarnation();
    // устанавливает новый `TimeStamp`
    void SetNewTimeStamp();

    // будем считать, что статус "лучше"если выполняется :
    //   - Alive лучше Suspicious
    //   - Suspicious лучше Dead
    //   - (соотношение транзитивно)
    bool IsStatusBetterThan(const MemberInfo& oth) const;
    // this->_incarnation > oth->_incarnation
    bool IsIncarnationMoreThan(const MemberInfo& oth) const;
};

/*
 * нода может быть представлена двумя стуктурами
 * - адрес (read-only)
 * - состояние
 * */
class Member {
private:
    MemberAddr _addr;
    MemberInfo _info;

public:
    explicit Member(MemberAddr addr);
    Member(const Member& oth) = default;
    Member(Member&& oth) noexcept;

    // функции преобразования для `Proto::MemberInfo`
    explicit Member(const Proto::Member& member);
    Proto::Member ToProtoType() const;

    Member& operator=(const Member& rhs) = default;
    Member& operator=(Member&& rhs) noexcept;

    // функции преобразования для `JSON`
    static Member FromJSON(const nlohmann::json& json);
    nlohmann::json ToJSON() const;

    const MemberAddr& Addr() const;
    const MemberInfo& Info() const;

    // состояние ноды можно менять с помощью методов `Info`
    MemberInfo& Info();
};

#endif // INCLUDE_MEMBER_HPP_
