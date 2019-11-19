// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <table.hpp>

PullTable::PullTable(const gossip::Table& table) {
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

PushTable::PushTable(std::shared_ptr<Table> pTable, std::vector<size_t>&& indexes)
  : _pTable{std::move(pTable)}
  , _indexes{indexes}
{}

gossip::Table PushTable::ToProtoType() const {
    gossip::Table protoTable;

    for (const auto& index : _indexes) {
        *protoTable.add_table() = (*_pTable)[index].ToProtoType();
    }

    return std::move(protoTable);
}


void Table::Update(PullTable&& pullTable) {
    for (size_t i = 0; i < pullTable.Size(); ++i) {
        // если узнали, что кто-то думает, что мы мертвы, то необходимо увеличить инкарнацию
        if (pullTable[i].Addr() == _me.Addr()) {
            if (pullTable[i].Info().Status() == MemberInfo::Dead) {
                _me.Info().IncreaseIncarnation();
            }
            continue;
        }

        auto found = _indexes.find(pullTable[i].Addr());
        auto& knownMember = _members[found->second];

        // если информация страее, чем наша, то мы игнорируем ее
        if (pullTable[i].Info().LatestUpdate(). OlderThan (knownMember.Info().LatestUpdate())) {
            continue;
        }


    }
}
