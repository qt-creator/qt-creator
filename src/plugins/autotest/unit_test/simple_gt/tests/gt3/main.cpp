// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include "dummytest.h"

using namespace testing;

#include <QMap>
#include <QStringList>

static QMap<const char *, QStringList> testValues = {
    { "DummyTest",
      {QStringLiteral("dummytest"), QStringLiteral("DUMMYTEST"), QStringLiteral("DummyTest")}
    },
    { "Hello World",
      {QStringLiteral("hello world"), QStringLiteral("HELLO WORLD"), QStringLiteral("Hello World")}
    },
    { "#include <QString>\n#include \"test.h\"",
      {QStringLiteral("#include <qstring>\n#include \"test.h\""),
       QStringLiteral("#INCLUDE <QSTRING>\n#INCLUDE \"TEST.H\""),
       QStringLiteral("#include &lt;QString&gt;\n#include &quot;test.h&quot;")
      }
    }
};

static QMap<const char *, QStringList> testValuesSec = {
    { "BlAh",
      {QStringLiteral("blah"), QStringLiteral("BLAH"), QStringLiteral("BlAh")}
    },
    { "<html>",
      {QStringLiteral("<html>"), QStringLiteral("<HTML>"), QStringLiteral("&lt;html&gt;")}
    },
};

INSTANTIATE_TEST_SUITE_P(First, DummyTest, ::testing::ValuesIn(testValues.keys()));
INSTANTIATE_TEST_SUITE_P(Second, DummyTest, ::testing::ValuesIn(testValuesSec.keys()));

TEST_P(DummyTest, Easy)
{
    // total wrong usage, but this is for testing purpose
    bool first = testValues.contains(GetParam());
    bool second = testValuesSec.contains(GetParam());
    QStringList expected;
    if (first)
        expected = testValues.value(GetParam());
    else if (second)
        expected = testValuesSec.value(GetParam());
    else
        FAIL();

    ASSERT_EQ(3, expected.size());

    EXPECT_EQ(m_str1, expected.at(0));
    EXPECT_EQ(m_str2, expected.at(1));
    EXPECT_EQ(m_str3, expected.at(2));
}

TEST(DummyTest, BlaBlubb)
{
    ASSERT_EQ(3, testValues.size());
}

int main(int argc, char *argv[])
{
    InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
