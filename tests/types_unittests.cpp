// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <gtest/gtest.h>
#include <arpa/inet.h>

#include <random>

#include <types.hpp>
#include <buffer.hpp>



class MemberList {
private:
    std::vector<Member> list_;
    mutable std::mt19937 generator_;

public:
    MemberList()
      : generator_(std::random_device{}())
    {
        std::vector<std::string> strIPVec = {
                "127.0.0.1",
                "192.72.1.12",
                "111.111.111.111",
                "217.69.128.44",
                "77.88.8.8",
                "213.180.217.10",
                "213.180.217.219",
                "213.180.217.7",
                "213.180.210.1",
                "213.180.194.138",
                "213.180.194.164",
                "213.180.194.185",
                "213.180.216.233",
                "213.180.216.234",
                "213.180.210.5",
                "213.180.210.9",
                "213.180.210.7",
                "213.180.210.10",
                "213.180.216.30",
                "213.180.216.160",
                "213.180.216.164",
                "213.180.216.165",
                "213.180.216.7"
        };
        std::vector<boost::asio::ip::address> uintIPVec;
        std::vector<uint16_t> portVec;
        std::vector<MemberAddr> addrVec;

        std::vector<MemberInfo::State> statesVec;
        std::vector<uint32_t> incarnationVec;
        std::vector<TimeStamp> timeVec;
        std::vector<MemberInfo> infoVec;

        // initialize loop
        for (size_t i = 0; i < strIPVec.size(); ++i) {
            // IP
            uintIPVec.emplace_back(boost::asio::ip::address::from_string(strIPVec[i]));
            // port
            portVec.push_back(80 + i);
            // addr
            addrVec.emplace_back(MemberAddr{uintIPVec[i],
                                             portVec[i]});

            // state
            statesVec.push_back(MemberInfo::State(generator_()%4));
            // incarnation
            incarnationVec.push_back(static_cast<uint32_t>(generator_()));
            // timestamp
            timeVec.push_back(TimeStamp {static_cast<uint32_t>(generator_())});
            // info
            infoVec.emplace_back(MemberInfo{statesVec[i],
                                            incarnationVec[i],
                                            timeVec[i]});

            // member
            list_.emplace_back(addrVec[i], infoVec[i]);
        }
    }

    const Member& RandomMember() const {
        return list_[generator_() % list_.size()];
    }

    const std::vector<Member>& GetList() const {
        return list_;
    }
} list;

TEST(TypeTranslation, Member) {
    // Normal buffer test
    for (const auto& member : list.GetList()) {
        std::vector<byte> buffer;
        buffer.resize(member.ByteSize());

        auto writePtr = member.Write(buffer.data(), buffer.data() + buffer.size());
        EXPECT_NE(writePtr, nullptr);
        EXPECT_EQ(writePtr, buffer.data() + buffer.size());

        Member result {};
        auto readPtr = result.Read(buffer.data(), buffer.data() + buffer.size());
        EXPECT_NE(readPtr, nullptr);
        EXPECT_EQ(readPtr, buffer.data() + buffer.size());

        EXPECT_EQ(result, member);
    }

    // Short buffer test
    Member member{list.RandomMember()};

    std::vector<byte> shortBuff;
    shortBuff.resize(member.ByteSize() - 1);

    auto writePtr = member.Write(shortBuff.data(), shortBuff.data() + shortBuff.size());
    EXPECT_EQ(writePtr, nullptr);

    auto readPtr = member.Read(shortBuff.data(), shortBuff.data() + shortBuff.size());
    EXPECT_EQ(readPtr, nullptr);
}

TEST(TypeTranslation, MemberTable) {
    // Normal buffer test
    MemberTable table;
    for (const auto& member : list.GetList())
        table.DebugInsert(member);

    ByteBuffer buffer(table.ByteSize());

    auto writePtr = table.Write(buffer.Begin(), buffer.End());
    EXPECT_NE(writePtr, nullptr);
    EXPECT_EQ(writePtr, buffer.End());

    MemberTable result;
    auto readPtr = result.Read(buffer.Begin(), buffer.End());

    EXPECT_NE(readPtr, nullptr);
    EXPECT_EQ(readPtr, buffer.End());

    EXPECT_EQ(result, table);

    // Short buffer test
    ByteBuffer shortBuff{table.ByteSize() - 1};

    writePtr = table.Write(shortBuff.Begin(), shortBuff.End());
    EXPECT_EQ(writePtr, nullptr);

    readPtr = table.Read(shortBuff.Begin(), shortBuff.End());
    EXPECT_EQ(readPtr, nullptr);
}

TEST(TypeTranslation, Gossip) {
    // Normal buffer test
    Gossip gossip;
    gossip.TTL = 42;
    gossip.Owner = list.RandomMember();
    gossip.Dest = list.RandomMember();
    gossip.Events = list.GetList();

    for (const auto& member : list.GetList())
        gossip.Table.DebugInsert(member);

    ByteBuffer buffer{gossip.ByteSize()};

    auto writePtr = gossip.Write(buffer.Begin(), buffer.End());
    EXPECT_NE(writePtr, nullptr);
    EXPECT_EQ(writePtr, buffer.End());

    Gossip result;
    auto readPtr = result.Read(buffer.Begin(), buffer.End());

    EXPECT_NE(writePtr, nullptr);
    EXPECT_EQ(writePtr, buffer.End());

    EXPECT_EQ(result, gossip);

    // Short buffer test
    ByteBuffer shortBuff{gossip.ByteSize() - 1};

    writePtr = gossip.Write(shortBuff.Begin(), shortBuff.End());
    EXPECT_EQ(writePtr, nullptr);

    readPtr = gossip.Read(shortBuff.Begin(), shortBuff.End());
    EXPECT_EQ(readPtr, nullptr);
}

TEST(MemberTable, RandomMethods) {
    MemberTable table;
    for (const auto& member : list.GetList()) {
        table.DebugInsert(member);
    }

    // Get random member method
    for (size_t i = 0; i < 1000; ++i) {
        auto member = table.RandomMember();
        EXPECT_TRUE(table.DebugIsExists(member));
    }

    // Get subset method
    auto subset = table.GetSubset(list.GetList().size()/2);

    std::vector<Member> membersInSubset;
    for (const auto& member : list.GetList()) {
        if (subset.DebugIsExists(member)) {
            membersInSubset.push_back(member);
        }
    }
    EXPECT_EQ(membersInSubset.size(), list.GetList().size()/2);

    for (size_t i = 0; i < membersInSubset.size(); ++i) {
        for (size_t j = 0; j < membersInSubset.size(); ++j) {
            if (i == j) continue;
            EXPECT_FALSE(membersInSubset[i] == membersInSubset[j]);
        }
    }
}


