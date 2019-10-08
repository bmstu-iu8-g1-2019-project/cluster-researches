// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#ifndef HEADERS_BEHAVIOR_HPP_
#define HEADERS_BEHAVIOR_HPP_

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <unistd.h>

#include <stdexcept>
#include <thread>

#include <types.hpp>
#include <membertable.hpp>
#include <gossiping.hpp>

void AppCommunicator(int sd, MemberTable& table);

void Gossiping(int sd, MemberTable& table);

#endif // HEADERS_BEHAVIOR_HPP_
