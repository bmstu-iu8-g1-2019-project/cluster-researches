// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <table.hpp>

std::mt19937 Table::_rGenerator{42};

Table::Table(const MemberAddr& addr, size_t latestPartSize, size_t randomPartSize)
  : _me{addr}
  , _latestPartSize{latestPartSize}
  , _randomPartSize{randomPartSize}
{}

const Member& Table::WhoAmI() const {
    _me.Info().SetNewTimeStamp();

    return _me;
}

bool Table::Update(const Member& member) {
    // если узнали, что кто-то думает, что мы мертвы, то необходимо увеличить инкарнацию
    if (member.Addr() == _me.Addr()) {
        if (member.Info().Status() == MemberInfo::Suspicious ||
            member.Info().Status() == MemberInfo::Dead) {
            _me.Info().IncreaseIncarnation();
        }
        return true;
    }

    auto found = _indexes.find(member.Addr());

    // если `member` -- неизвестный член кластера
    if (found == _indexes.end()) {
        _indexes.insert(std::make_pair(member.Addr(), _members.size()));
        return true;
    }

    // `knownMember` -- информация о `member` из `this->_members`
    // переменная для того, чтобы каждый раз не искать `member` в `this->_members`
    auto& knownMember = _members[found->second];

    // если у `member` новая инкарнация
    if (member.Info(). IsIncarnationMoreThan (knownMember.Info())) {
        knownMember.Info() = member.Info();
        return true;
    }

    // если информация страее, чем наша, то мы игнорируем ее
    if (member.Info().LatestUpdate(). OlderThan (knownMember.Info().LatestUpdate())) {
        return false;
    }

    // если конфликт состояний члена кластера
    // она возникает, если `member->Status()` лучше, чем `knownMember->Status()`
    if (member.Info(). IsStatusBetterThan (knownMember.Info())) {
        return false;
    }

    // все остальные случаи являются валидными, ими нужно обновить таблицу
    knownMember.Info() = member.Info();
    return true;
}

std::vector<size_t> Table::Update(PullTable&& pullTable) {
    std::vector<size_t> conflicts;

    for (size_t i = 0; i < pullTable.Size(); ++i) {
        if (!Update(pullTable[i])) {
            conflicts.push_back(_indexes.find(pullTable[i].Addr())->second);
        }
    }

    return std::move(conflicts);
}

PushTable Table::MakePushTable() const {
    std::vector<size_t> indexes;

    for (const auto& latest : _latestUpdates) {
        indexes.push_back(latest);
    }

    for (size_t i = 0; i < _randomPartSize &&
                       i + _latestUpdates.size() < _members.size();) {
        size_t randomIndex = _rGenerator();

        if (std::find(indexes.begin(), indexes.end(), randomIndex) == indexes.end()) {
            indexes.push_back(randomIndex);
            ++i;
        }
    }

    PushTable pushTable{std::shared_ptr<const Table>(this, [](const Table*){}),
                        std::move(indexes)
    };

    return std::move(pushTable);

    // Самый быстрый алгоритм, но очень дорогой по памяти
    /*std::vector<size_t> indexes;

    for (const auto& latest : _latestUpdates) {
        indexes.push_back(latest);
    }

    std::vector<size_t> oldestUpdates;
    oldestUpdates.resize(_members.size() - _latestUpdates.size());
    for (size_t index = 0; index < _members.size(); ++index) {
        if (std::find(_latestUpdates.begin(), _latestUpdates.end(), index) == indexes.end()) {
            oldestUpdates.push_back(index);
        }
    }

    std::shuffle(oldestUpdates.begin(), oldestUpdates.end(), rand);

    for (size_t i = 0; i < _randomPartSize &&
                       i + _latestUpdates.size() < _members.size();
                       ++i) {
        oldestUpdates.push_back(indexes[i]);
    }

    PushTable pushTable{std::make_shared<const Table>(this, [](const Table*){}),
                        std::move(indexes)
    };

    return std::move(pushTable);*/
}

std::vector<size_t> Table::MakeDestList() const {
    std::vector<size_t> destIndexes;

    std::iota(destIndexes.begin(), destIndexes.end(), 0);
    std::shuffle(destIndexes.begin(), destIndexes.end(), _rGenerator);

    return std::move(destIndexes);
}

const Member& Table::operator[](const MemberAddr& addr) const {
    return _members[_indexes.find(addr)->second];
}

const Member& Table::operator[](size_t index) const {
    return _members[index];
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


PushTable::PushTable(std::shared_ptr<const Table> pTable, std::vector<size_t>&& indexes)
  : _pTable{std::move(pTable)}
  , _indexes{indexes}
{}

Proto::Table PushTable::ToProtoType() const {
    Proto::Table protoTable;

    for (const auto& index : _indexes) {
        *protoTable.add_table() = (*_pTable)[index].ToProtoType();
    }

    return std::move(protoTable);
}

const Member& PushTable::WhoAmI() const {
    return _pTable->WhoAmI();
}


