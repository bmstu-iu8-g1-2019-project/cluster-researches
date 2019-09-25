// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#ifndef HEADERS_BEHAVIOR_HPP_
#define HEADERS_BEHAVIOR_HPP_

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <stdexcept>

#include <membertable.hpp>

// It will be env variable in future
std::string socket_file{"/tmp/app.socket"};

void AppBridging(MemberTable& table);

void Gossiping(MemberTable& table);

#endif // HEADERS_BEHAVIOR_HPP_
