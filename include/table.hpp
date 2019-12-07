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
    class FDStatus {
    private:
        // `true`, если был пинг и ждем на него акк
        bool _isWaitForAck;
        // время последнего пинга
        TimeStamp _lastPing;

        // количество пингов, не принятных нодой
        uint32_t _failures;

    public:
        FDStatus();
        // ставится ожидание акка от ноды
        void SetAckWaiting();
        // сбрасывается ожидание акка от ноды
        void ResetAckWaiting();

        // проверяет заподозрен ли выход из строя данной ноды
        // возвращает:
        // Alive      -- если нет изменения текущего статуса
        // Suspicious -- если нода ведет себя подозрительно
        // Dead       -- если однозначно ясно, что нода умерла
        MemberInfo::NodeState DetectFailure(milliseconds timeout, size_t failuresBeforeDead);

        bool IsWaitForAck() const;
    };

private:
    static std::mt19937 _rGenerator;

    // размер списка последних (самых новых) событий
    size_t _latestPartSize;
    // размер части подтаблицы `PushTable` со случайными событиями
    size_t _randomPartSize;

    // информация о ноде, которую представляет данная таблица
    mutable Member _me;

    // хранит key-value, где key -- адрес ноды, value -- порядковый номер в списке всех нод
    std::unordered_map<MemberAddr, size_t, MemberAddr::Hasher> _indexes;
    // список всех нод
    std::vector<Member> _members;

    // список с Failure Detection данными о нодах (порядок соответсвует порядку `_members`)
    std::vector<FDStatus> _fd;

    // список флагов для отправки акков (если `true`, то нода нашего ждет акка)
    std::vector<bool> _ackWaiters;

    // сортированный список индесков на ноды, с которыми произошли последине события
    // список не может превзойти по размеру `_latestPartSize`
    // самые новые обновления находятся вначале массива
    std::vector<size_t> _latestUpdates;

public:
    // таблица создается для ноды которую она представляет
    // `latestPartSize` и `randomPartSize` -- конфигурацинные параметры таблицы
    Table(const MemberAddr& addr, size_t latestPartSize, size_t randomPartSize);
    Table(Table&& oth) noexcept;

    size_t Size() const;

    // возвращает `Member` текущей ноды кластера
    const Member& WhoAmI() const;

    // обновляет таблицу информацией об одной ноде
    bool Update(const Member& member);
    // возвращает список индексов на конфликтные члены
    bool Update(PullTable&& table);

    // установка ожидания акка и его сброс
    void SetAckWaitingFrom(size_t index);
    void ResetAckWaitingFrom(size_t index);

    // проверяет все ноды из `_members` на наличие FD-ситуаций и выявляет их
    // если такие были выявлены, то обновляет ноду и ставит соответствующий ситуации статус
    void DetectFailures(milliseconds msTimeout, size_t failuresBeforeDead);

    // формирует и возвращает `PushTable`
    PushTable MakePushTable() const; // Size -- latest.size() + random.size()

    // создает "перемешанный" список индексов на ноды
    // он нужнен для ранодомизации распространения слухов
    std::vector<size_t> MakeDestList() const;

    // установка и сброс флага о том, что нода ожидает от нас акка
    void SetAckWaiter(size_t index);
    void ResetAckWaiter(size_t index);

    // список всех, кто ждет от нас акк
    std::vector<size_t> AckWaiters() const;

    // переводит структуру `MemberAddr` в соответствующий ей индекс из `_members`
    size_t ToIndex(const MemberAddr& addr) const;

    // доступ к нодам из `_members`
    const Member& operator[](const MemberAddr& addr) const;
    const Member& operator[](size_t index) const;

private:
    // создает новое событие для ноды `index`
    void _NewEvent(size_t index, MemberInfo::NodeState status);
    // вставляет в таблицу `member` как новую ноду
    void _Insert(const Member& member);
    // вставляет в последние обновления ниформацию о ноде
    // (может не вставить, если информация достаточно старая)
    void _InsertInLatest(size_t index);
};


class PullTable {
private:
    // хранит информацию о сведений из приходящих из вне слухов
    std::vector<Member> _members;

public:
    // может быть сконструированна только из приходящей из вне структуры `Proto::Table`
    explicit PullTable(const Proto::Table& table);
    // только перемещаемый объект
    PullTable(PullTable&& oth) noexcept;

    size_t Size() const;
    const Member& operator[](size_t index) const;
};


class PushTable {
private:
    // ссылка на таблицу из которой берутся все данные о нодах
    const Table& _refTable;
    // индексы на входящие в `PushTable` ноды
    std::vector<size_t> _indexes;

public:
    PushTable(const Table& table, std::vector<size_t>&& indexes);
    PushTable(PushTable&& oth) noexcept;
    // переод в структкру `Proto::Table`
    Proto::Table ToProtoType() const;

    // метод, необходимый для того, чтобы создавать из таблицы слух (нужен для поля `Owner`)
    const Member& WhoAmI() const;
};

#endif // INCLUDE_TABLE_HPP_
