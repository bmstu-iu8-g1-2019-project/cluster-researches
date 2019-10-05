// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#ifndef GOSSIPING_GOSSIPING_HPP_
#define GOSSIPING_GOSSIPING_HPP_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include <membertable.hpp>
#include <behavior.hpp>

// It will be env variables in future
size_t max_connections = 10;

void FailureDetection(int sd, MemberTable& table);

void Distribution(int sd, MemberTable& table);


// TODO(AndreevSemen): Incomplete class
class Gossip {
    MemberAddr initiator;
    MemberTable subSet;

};

/*
 * Temporary magic-fairy functions
 * TODO(AndreevSemen): Realize them!
 */
Gossip RecvGossip(int sd);
void SendGossip(int sd, const MemberAddr& dest, const Gossip& gossip);

void Update(MemberTable& table, const Gossip& gossip);
Gossip CreatePingMsg(const MemberTable& table, const MemberAddr& dest);
Gossip CreateAckMsg(const MemberTable& table, const Gossip& request)

#endif // GOSSIPING_GOSSIPING_HPP_
