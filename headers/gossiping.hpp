// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#ifndef GOSSIPING_GOSSIPING_HPP_
#define GOSSIPING_GOSSIPING_HPP_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include <membertable.hpp>

// It will be env variables in future
size_t max_connections = 10;

void SelfGossiping(int sd, MemberTable& table);

void OutsideGossiping(int sd, MemberTable& table);

#endif // GOSSIPING_GOSSIPING_HPP_
