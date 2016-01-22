/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/
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

INSTANTIATE_TEST_CASE_P(First, DummyTest, ::testing::ValuesIn(testValues.keys()));
INSTANTIATE_TEST_CASE_P(Second, DummyTest, ::testing::ValuesIn(testValuesSec.keys()));

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
