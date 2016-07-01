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

using namespace testing;

int sum(int a, int b)
{
    return a + b;
}

TEST(Sum, HandlePositives)
{
    EXPECT_EQ(5, sum(2, 3));
    EXPECT_EQ(5, sum(3, 2));
}

TEST(Sum, HandleZero)
{
    EXPECT_EQ(1, sum(0, 0));
}

TEST(FactorialTest, DISABLED_Fake)
{
    EXPECT_EQ(-4, sum(-2, -2));
}

namespace Dummy {
namespace {
namespace Internal {

TEST(NamespaceTest, Valid)
{
    EXPECT_EQ(1, 1);
}

} // namespace Internal
} // anon namespace
} // namespace Dummy
