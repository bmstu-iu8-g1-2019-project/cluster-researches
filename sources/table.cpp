// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <table.hpp>

std::mt19937 Table::_rGenerator{42};

Table::FDStatus::FDStatus()
  : _isWaitForAck{false}
  , _lastPing{TimeStamp::Now()}
  , _failures{0}
{}

size_t Table::Size() const {
    return _members.size();
}

void Table::FDStatus::SetAckWaiting() {
    _isWaitForAck = true;
    _lastPing = TimeStamp::Now();
}

void Table::FDStatus::ResetAckWaiting() {
    _isWaitForAck = false;
    _failures = 0;
}

MemberInfo::NodeState Table::FDStatus::DetectFailure(milliseconds timeout, size_t failuresBeforeDead) {
    MemberInfo::NodeState detectedState = MemberInfo::Alive;

    if (IsWaitForAck() && _lastPing.TimeDistance(TimeStamp::Now()) > timeout) {
        if (_failures >= failuresBeforeDead) {
            detectedState = MemberInfo::Dead;
        } else {
            detectedState = MemberInfo::Suspicious;
        }

        ++_failures; // увеличиваем, потому что произошел failure detection
        _isWaitForAck = false; // не ждем акка до следующего пинга (если будет акк, то ничего страшного не произойдет)
    }

    return detectedState;
}

bool Table::FDStatus::IsWaitForAck() const {
    return _isWaitForAck;
}

Table::Table(const MemberAddr& addr, size_t latestPartSize, size_t randomPartSize)
  : _me{addr}
  , _latestPartSize{latestPartSize}
  , _randomPartSize{randomPartSize}
{}

Table::Table(Table&& oth) noexcept
  : _latestPartSize{oth._latestPartSize}
  , _randomPartSize{oth._randomPartSize}
  , _me{std::move(oth._me)}
  , _indexes{std::move(oth._indexes)}
  , _members{std::move(oth._members)}
  , _fd{std::move(oth._fd)}
  , _ackWaiters{std::move(oth._ackWaiters)}
  , _latestUpdates{std::move(oth._latestUpdates)}
{
    oth._latestPartSize = 0;
    oth._randomPartSize = 0;
}

const Member& Table::WhoAmI() const {
    return _me;
}

bool Table::Update(const Member& member) {
    // если известно, что другая нода думает, что мы мертвы или
    // подозрительны, то необходимо увеличить инкарнацию
    if (member.Addr() == WhoAmI().Addr()) {
        if ((member.Info().Status() == MemberInfo::Suspicious ||
            member.Info().Status() == MemberInfo::Dead) &&
            !_me.Info().IsIncarnationMoreThan(member.Info())) {
            _me.Info().IncreaseIncarnation();
        }

        return true;
    }

    // поиск ноды в `_members`
    auto found = _indexes.find(member.Addr());

    // если не найдена, то ее нужно добавить как новую
    if (found == _indexes.end()) {
        _Insert(member);
        _InsertInLatest(ToIndex(member.Addr()));

        return true;
    }

    // `knownMember` -- информация о `member` из `this->_members`
    // переменная для читабильности
    auto& knownMember = _members[found->second];

    // если у `member` новая инкарнация
    if (member.Info(). IsIncarnationMoreThan (knownMember.Info())) {
        knownMember.Info() = member.Info();
        _InsertInLatest(ToIndex(member.Addr()));

        return true;
    }

    // если информация страее, чем наша, то мы игнорируем ее
    if (member.Info().LatestUpdate(). OlderThan (knownMember.Info().LatestUpdate())) {
        return false;
    }

    // если конфликт состояний члена кластера
    // она возникает, если `member->Status()` лучше, чем `knownMember->Status()`
    // не верим данному слуху
    if (member.Info(). IsStatusBetterThan (knownMember.Info())) {
        return false;
    }

    // все остальные случаи являются валидными, ими нужно обновить таблицу
    knownMember.Info() = member.Info();
    _InsertInLatest(ToIndex(member.Addr()));

    return true;
}

bool Table::Update(PullTable&& pullTable) {
    bool isUpdated = false;
    for (size_t i = 0; i < pullTable.Size(); ++i) {
        if (Update(pullTable[i])) {
            isUpdated = true;
        }
    }

    return isUpdated;
}

void Table::SetAckWaitingFrom(size_t index) {
    _fd[index].SetAckWaiting();
}

void Table::ResetAckWaitingFrom(size_t index) {
    _fd[index].ResetAckWaiting();
}

void Table::DetectFailures(milliseconds msTimeout, size_t failuresBeforeDead) {
    for (size_t index = 0; index < _members.size(); ++index) {
        switch (_fd[index].DetectFailure(msTimeout, failuresBeforeDead)) {
            // состояние не поменялось
            case MemberInfo::Alive :
                break;
            // поменялось на Suspicious
            case MemberInfo::Suspicious :
                // мы не можем улучшить состояние ноды,
                // пока она сама не установит новую инкарнацию
                if (_members[index].Info().Status() < MemberInfo::Suspicious) {
                    _NewEvent(index, MemberInfo::Suspicious);
                }
                break;
            // поменялось на Dead
            case MemberInfo::Dead :
                if (_members[index].Info().Status() < MemberInfo::Dead) {
                    _NewEvent(index, MemberInfo::Dead);
                }
                break;
        }
    }
}

void Table::_NewEvent(size_t index, MemberInfo::NodeState status) {
    _members[index].Info().Status() = status;
    _members[index].Info().SetNewTimeStamp();
    _InsertInLatest(index);
}

PushTable Table::MakePushTable() const {
    // в подтаблицу должны быть занесены все последние обновления
    // и по возможности случайные не последние обновления (максимально `_randomPartSize` событий)
    // (размер последних обновлений может быть меньше, чем `_latestPartSize`)
    std::vector<size_t> indexes;
    size_t expectedPushTableSize = _latestUpdates.size() + _randomPartSize;
    indexes.reserve((expectedPushTableSize < _members.size()) ?
                    expectedPushTableSize :
                    _members.size());

    // добавляем все последние обновления
    for (const auto& latest : _latestUpdates) {
        indexes.push_back(latest);
    }

    // добавляем случайные обновления
    // (общий размер подтаблицы должен быть меньше размера `_members`)
    for (size_t i = 0; i < _randomPartSize &&
                       i + _latestUpdates.size() < _members.size();) {
        size_t randomIndex = _rGenerator() % _members.size();

        // если событие уже было занесено
        if (std::find(indexes.begin(), indexes.end(), randomIndex) == indexes.end()) {
            indexes.push_back(randomIndex);
            ++i;
        }
    }

    PushTable pushTable{*this, std::move(indexes)};

    return std::move(pushTable);
}

std::vector<size_t> Table::MakeDestList() const {
    std::vector<size_t> destIndexes;
    destIndexes.resize(_members.size());

    // создает последоватеьность от 0 до size
    std::iota(destIndexes.begin(), destIndexes.end(), 0);

    // перемешиваем
    std::shuffle(destIndexes.begin(), destIndexes.end(), _rGenerator);

    return std::move(destIndexes);
}

void Table::SetAckWaiter(size_t index) {
    _ackWaiters[index] = true;
}

void Table::ResetAckWaiter(size_t index) {
    _ackWaiters[index] = false;
}

std::vector<size_t> Table::AckWaiters() const {
    std::vector<size_t> waiters;
    for (size_t index = 0; index < _members.size(); ++index) {
        if (_ackWaiters[index]) {
            waiters.push_back(index);
        }
    }

    return std::move(waiters);
}

size_t Table::ToIndex(const MemberAddr& addr) const {
    return _indexes.find(addr)->second;
}

const Member& Table::operator[](const MemberAddr& addr) const {
    return _members[_indexes.find(addr)->second];
}

const Member& Table::operator[](size_t index) const {
    return _members[index];
}

void Table::_Insert(const Member& member) {
    // добавляем в мапу новый индекс
    _indexes.insert(std::make_pair(member.Addr(), _members.size()));
    // добавляем в конец ноду
    _members.push_back(member);
    // создаем для ноды FD-запись в таблице
    _fd.emplace_back(FDStatus{});
    // добавляем в флаг о том, что нода ждет от нас акк
    _ackWaiters.push_back(false);
}

void Table::_InsertInLatest(size_t index) {
    // ищем позицию для вставки `index`
    auto bound = std::find_if(_latestUpdates.begin(), _latestUpdates.end(),
            [&](size_t latest) {
                return _members[latest].Info().LatestUpdate().
                       OlderThan(
                       _members[index].Info().LatestUpdate());
            });

    _latestUpdates.insert(bound, index);

    // при необходимсти обрезаем до допустимого размера
    if (_latestUpdates.size() > _latestPartSize) {
        _latestUpdates.pop_back();
    }
}


PullTable::PullTable(const Proto::Table& table) {
    for (size_t i = 0; i < table.table_size(); ++i) {
        _members.emplace_back(Member{table.table(i)});
    }
}

PullTable::PullTable(PullTable&& oth) noexcept
  : _members{std::move(oth._members)}
{}

size_t PullTable::Size() const {
    return _members.size();
}

const Member& PullTable::operator[](size_t index) const {
    return _members[index];
}


PushTable::PushTable(const Table& table, std::vector<size_t>&& indexes)
  : _refTable{table}
  , _indexes{std::move(indexes)}
{}

PushTable::PushTable(PushTable&& oth) noexcept
  : _refTable(oth._refTable)
  , _indexes{std::move(oth._indexes)}
{}

Proto::Table PushTable::ToProtoType() const {
    Proto::Table protoTable;

    for (const auto& index : _indexes) {
        *protoTable.add_table() = _refTable[index].ToProtoType();
    }

    return std::move(protoTable);
}

const Member& PushTable::WhoAmI() const {
    return _refTable.WhoAmI();
}
