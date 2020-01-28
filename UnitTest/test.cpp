#include "pch.h"
#include "..\\ProteusServer\stdafx.h"
#include "..\\ProteusServer\Utils.h"

TEST(StringLength, BigNumber)
{
    const unsigned length = 123456;
    const unsigned bytes = 3;
    std::string str = LengthToStr(length, bytes);
    const unsigned len = StrToLength(str.c_str(), str.length());
    EXPECT_EQ(len, length);
}

TEST(NumberSumTest, PositiveNos)
{
    EXPECT_EQ(15, numberSum(12345));
    EXPECT_EQ(10, numberSum(1234));
}

class TestPacketParser : public ::testing::Test
{
protected:
 
    CPacketDispather m_dispather;
    std::string MakeString(const std::string& str)
    {
        std::string sMaked = m_dispather.MakeMessage(str);
        std::list<std::string> pMsgs;
        m_dispather.OnNewMsg((char*)sMaked.c_str(), sMaked.length(), pMsgs);

        std::string parsed; 
        if (pMsgs.size())
        {
            parsed = *pMsgs.begin();
        }
        return parsed;
    }
};

TEST_F(TestPacketParser, BigString)
{
    std::string str = "dcsdgsdfsd123214ttfhfgjh9089065hgdgsfasaqwa434565768543asdasdsfvkmlgbg788934434 jjkfhsjkhsaher23dsdsd";
    ASSERT_STREQ(str.c_str(), MakeString(str).c_str());
}

TEST_F(TestPacketParser, SmallString)
{
    std::string str = "kjh';5";
    ASSERT_STREQ(str.c_str(), MakeString(str).c_str());
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}