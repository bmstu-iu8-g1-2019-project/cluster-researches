// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <matrix.hpp>

MemberMatrix::MemberMatrix()
  : _nullMember{MemberAddr{ip_v4::from_string("0.0.0.0"), 0}}
{
    _nullMember.Info().Status() = MemberInfo::Dead;
}

void MemberMatrix::Push(PullGossip&& pull) {
    if (!_IsInMatrix(pull.Owner())) {
        _NewRow(pull.Owner());
    }
    auto ownerFound = _indexes.find(pull.Owner().Addr());

    auto table = pull.MoveTable();
    for (size_t i = 0; i < table.Size(); ++i) {
        auto neighbour = table[i];

        if (!_IsInMatrix(neighbour)) {
            _NewRow(neighbour);
        }
        auto neighbourFound = _indexes.find(neighbour.Addr());

        _matrix[ownerFound->second][neighbourFound->second] = std::move(neighbour);
    }
    _matrix[ownerFound->second][ownerFound->second] = pull.Owner();
    _fd[ownerFound->second] = TimeStamp::Now();
}

void MemberMatrix::DetectFailure(milliseconds timeout) {
    for (size_t i = 0; i < _fd.size(); ++i) {
        if (_fd[i].Time() != milliseconds{0} &&
            _fd[i].TimeDistance(TimeStamp::Now()) > timeout) {
            _fd[i] = TimeStamp{milliseconds{0}};

            for (auto& member : _matrix[i]) {
                member.Info().Status() = MemberInfo::Dead;
            }
        }
    }
}

void MemberMatrix::SearchAbsolutelyDead() {
    if (_matrix.empty()) {
        return;
    }

    for (size_t j = 0; j < _matrix.front().size(); ++j) {
        if (_matrix[j][j].Info().Status() == MemberInfo::Dead) {
            continue;
        }

        bool isDead = true;
        for (size_t i = 0; i < _matrix.size(); ++i) {
            if (i == j) {
                continue;
            }

            if (_matrix[i][j].Info().Status() != MemberInfo::Dead) {
                isDead = false;
                break;
            }
        }

        if (isDead) {
            TimeStamp deathPoint = TimeStamp::Now();

            _matrix[j][j].Info().Status() = MemberInfo::Dead;

            std::cout << "Member " << j << " dead" << std::endl
                      << "Death point : " << deathPoint.Time().count() << std::endl;
        }
    }
}

void MemberMatrix::Log(std::ostream& out) const {
    system("clear");
    for (auto i = 0; i < _matrix.size(); ++i) {
        for (auto j = 0; j < _matrix.front().size(); ++j) {
            switch (_matrix[i][j].Info().Status()) {
                case MemberInfo::Alive :
                    out << "\033[0;32mA\033[0m";
                    break;
                case MemberInfo::Suspicious :
                    out << "\033[0;33mS\033[0m";
                    break;
                case MemberInfo::Dead :
                    out << "\033[1;31mF\033[0m";
            }
            out << " ";
        }
        std::string status;
        switch (_matrix[i][i].Info().Status()) {
            case MemberInfo::Alive :
                status = "alive";
                break;
            case MemberInfo::Suspicious :
                status = "suspicious";
            case MemberInfo::Dead :
                status = "dead";
        }
        out << " Member " << i << " : " << _matrix[i][i].Addr().IP().to_string() << ":" << _matrix[i][i].Addr().Port()
            << " Incarnation : " << _matrix[i][i].Info().Incarnation()
            << " Status : " << status
            << std::endl;
    }
    out << "Cluster size : " << _matrix.size()
        << std::endl;
}

bool MemberMatrix::_IsInMatrix(const Member& member) const {
    return _indexes.find(member.Addr()) != _indexes.end();
}

void MemberMatrix::_NewRow(const Member& member) {
    _indexes.emplace(std::make_pair(member.Addr(), _matrix.size()));
    _matrix.emplace_back(std::vector<Member>(_matrix.size(), _nullMember));
    _fd.emplace_back(TimeStamp::Now());

    for (auto& row : _matrix) {
        row.push_back(_nullMember);
    }
}
