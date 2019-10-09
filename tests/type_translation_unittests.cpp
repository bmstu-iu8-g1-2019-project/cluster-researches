// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <gtest/gtest.h>
#include <arpa/inet.h>

#include <types.hpp>

TEST(TypeTranslation, Member) {
    Member member;

    inet_aton("127.0.0.1", &member.Addr.IP);
    member.Addr.Port = 80;
    member.Info.Status = MemberInfo::Dead;
    member.Info.Incarnation = -1;
    member.Info.LastUpdate.Time = std::clock();

    byte buffer[100];

    auto ptrResult = member.InsertToBytes(buffer, buffer + 18);
    EXPECT_NE(ptrResult, nullptr);

    Member translated{*reinterpret_cast<Member*>(buffer)};

    EXPECT_EQ(translated, member);
}

TEST(TypeTranslation, MemberTable) {

}
