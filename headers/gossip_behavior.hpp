// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#ifndef HEADERS_BEHAVIOR_HPP_
#define HEADERS_BEHAVIOR_HPP_

#include <sys/types.h>
#include <sys/socket.h>

#include <membertable.hpp>

void AppBridging(MemberTable& table);

void Gossiping(MemberTable& table);

#endif // HEADERS_BEHAVIOR_HPP_
