// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#ifndef INCLUDE_TABLE_HPP_
#define INCLUDE_TABLE_HPP_

#include <boost/asio.hpp>


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

class PullTable {
private:
    std::vector<Member> _members;

public:
    explicit PullTable(const gossip::Table& table);
    PullTable(PullTable&& oth) noexcept;

    size_t Size() const;
    const Member& operator[](size_t index) const;
};

class PushTable {
private:
    std::shared_ptr<Table> _pTable;
    std::vector<size_t> _indexes;

public:
    PushTable(std::shared_ptr<Table> pTable, std::vector<size_t>&& indexes);
    gossip::Table ToProtoType() const;
};

class Table {
private:
    //  TimeStamp::Zero() -- все хорошо, ничего не ожидаем
    //  TimeStamp:: -- должны пропинговать, как только сможем
    //
    enum FDState {
        OK = 0,
        NeedPing = 1,

    };
private:
    Member _me;

    std::unordered_map<MemberAddr, size_t, MemberAddr::Hasher> _indexes;
    std::vector<Member> _members;

    std::vector<size_t> _latestUpdates;

public:
    // void Insert(const Member& member);
    void Update(PullTable&& table);

    // Size -- latest.size() + random.size()
    PushTable MakePushTable() const;

    std::vector<size_t> MakeDestQ() const;

    const Member& operator[](const MemberAddr& addr) const;
    const Member& operator[](size_t index) const;
};

#endif // INCLUDE_TABLE_HPP_
