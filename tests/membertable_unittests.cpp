// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <gtest/gtest.h>

#include <membertable.hpp>
#include <arpa/inet.h>

TEST(MemberTable, Insert) {
    MemberTable table;

    EXPECT_EQ(table.Size(), 0);

    MemberAddr addr{inet_addr("127.0.0.1"), 80};
    EXPECT_FALSE(table.IsExist(addr));
    MemberInfo info{"alive", 1};
    table.Insert(Member{addr,info});
    EXPECT_EQ(table.Size(), 1);
    EXPECT_TRUE(table.IsExist(addr));
}

TEST(MemberTable, Erase) {
    MemberTable table;

    MemberAddr addr1{inet_addr("127.0.0.1"), 80};
    MemberInfo info1{"alive", 1};
    table.Insert(Member{addr1, info1});

    MemberAddr addr2{inet_addr("127.0.0.2"), 80};
    MemberInfo info2{"suspected", 1};
    table.Insert(Member{addr2, info2});

    MemberAddr addr3{inet_addr("127.0.0.2"), 81};
    MemberInfo info3{"dead", 3};
    table.Insert(Member{addr3, info3});
    EXPECT_EQ(table.Size(), 3);
    std::cout << table.ToJson() << std::endl;

    table.Erase(addr1);

    EXPECT_FALSE(table.IsExist(addr1));
    EXPECT_TRUE(table.IsExist(addr2));
    EXPECT_TRUE(table.IsExist(addr3));
}

TEST(MemberTable, Update) {
    MemberTable table;

    MemberAddr addr{inet_addr("127.0.0.1"), 83};
    MemberInfo info1{"dead", 1};
    MemberInfo info2{"alive", 2};

    table.Insert(Member{addr, info1});
    auto beforeUpdate = table.ToJson();

    table.Update(Member{addr, info2});
    auto afterUpdate = table.ToJson();
    EXPECT_NE(beforeUpdate, afterUpdate);
}

TEST(MemberTable, RoundQueueAndSubset) {
    MemberTable table;

    MemberAddr addr1{inet_addr("127.0.0.1"), 80};
    MemberAddr addr2{inet_addr("127.0.0.1"), 81};
    MemberAddr addr3{inet_addr("127.0.0.2"), 81};
    MemberAddr addr4{inet_addr("127.0.0.2"), 82};
    MemberAddr addr5{inet_addr("127.0.0.3"), 82};

    MemberInfo info1{"alive", 0};
    MemberInfo info2{"alive", 1};
    MemberInfo info3{"alive", 2};
    MemberInfo info4{"alive", 3};
    MemberInfo info5{"alive", 4};

    table.Insert(Member{addr1, info1});
    table.Insert(Member{addr2, info2});
    table.Insert(Member{addr3, info3});
    table.Insert(Member{addr4, info4});
    table.Insert(Member{addr5, info5});

    auto firstQueue = table.GenerateRoundQueue();
    auto secondQueue = table.GenerateRoundQueue();

    EXPECT_NE(firstQueue, secondQueue);

    std::cout << table.GetSubset(3).ToJson() << std::endl;
}
