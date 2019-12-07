// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#ifndef INCLUDE_MATRIX_HPP_
#define INCLUDE_MATRIX_HPP_

#include <gossip.hpp>

class MemberMatrix {
private:
    std::unordered_map<MemberAddr, size_t ,MemberAddr::Hasher> _indexes;
    std::vector<std::vector<Member>> _matrix;

    std::vector<TimeStamp> _fd;

    Member _nullMember;

public:
    MemberMatrix();

    void Push(PullGossip&& pull);
    void DetectFailure(milliseconds timeout);

    void SearchAbsolutelyDead();

    void Log(std::ostream& out) const;

private:
    bool _IsInMatrix(const Member& member) const;

    void _NewRow(const Member& member);
};

#endif // INCLUDE_MATRIX_HPP_
