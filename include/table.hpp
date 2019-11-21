// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#ifndef INCLUDE_TABLE_HPP_
#define INCLUDE_TABLE_HPP_

#include <numeric>
#include <random>

#include <boost/asio.hpp>

#include <protobuf.pb.h>

#include <member.hpp>

typedef boost::asio::ip::udp::socket socket_type;
typedef uint8_t byte_t;

/*
 * основная таблица, на которую ссылаются PushTable's
 * */
class Table;

/*
 * будет два типа слухов : push and pull
 * push -- те, которые мы отправляем. в них лежит адресат и адресант и
 *         PushTable (внутри лежит указатель на основную таблицу и массив,
 *                    включенных в PushTable членов)
 *
 * pull -- те, которые мы принимаем. в них лежит адресат и адресант и
 *         PullTable (внутри лежит просто вектор членов)
 * */
class PullTable;
class PushTable;


class Table {
private:
    static std::mt19937 _rGenerator;

    size_t _latestPartSize;
    size_t _randomPartSize;

    mutable Member _me;

    std::unordered_map<MemberAddr, size_t, MemberAddr::Hasher> _indexes;
    std::vector<Member> _members;

    std::vector<size_t> _latestUpdates;

public:
    Table(const MemberAddr& addr, size_t latestPartSize, size_t randomPartSize);

    // возвращает `Member` текущей ноды кластера
    const Member& WhoAmI() const;

    // если возник конфликт, то вернет `false`, иначе `true`
    bool Update(const Member& member);

    // возвращает список индексов на конфликтные члены
    std::vector<size_t> Update(PullTable&& table);

    PushTable MakePushTable() const; // Size -- latest.size() + random.size()

    std::vector<size_t> MakeDestList() const;

    const Member& operator[](const MemberAddr& addr) const;
    const Member& operator[](size_t index) const;
};


class PullTable {
private:
    std::vector<Member> _members;

public:
    explicit PullTable(const Proto::Table& table);
    PullTable(PullTable&& oth) noexcept;

    size_t Size() const;
    const Member& operator[](size_t index) const;
};


class PushTable {
private:
    std::shared_ptr<const Table> _pTable;
    std::vector<size_t> _indexes;

public:
    PushTable(std::shared_ptr<const Table> pTable, std::vector<size_t>&& indexes);
    Proto::Table ToProtoType() const;

    const Member& WhoAmI() const;
};

#endif // INCLUDE_TABLE_HPP_
