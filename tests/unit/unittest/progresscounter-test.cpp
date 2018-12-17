/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "googletest.h"

#include <progresscounter.h>

namespace {

class ProgressCounter : public testing::Test
{
protected:
    NiceMock<MockFunction<void(int, int)>> mockSetProgressCallback;
    ClangBackEnd::ProgressCounter counter{mockSetProgressCallback.AsStdFunction()};
};

TEST_F(ProgressCounter, TotalAfterInitializing)
{
    ASSERT_THAT(counter.total(), 0);
}

TEST_F(ProgressCounter, ProgressAfterInitializing)
{
    ASSERT_THAT(counter.progress(), 0);
}

TEST_F(ProgressCounter, AddTotal)
{
    counter.addTotal(5);
    counter.addProgress(3);

    EXPECT_CALL(mockSetProgressCallback, Call(3, 12));

    counter.addTotal(7);
}

TEST_F(ProgressCounter, AddTotalZero)
{
    counter.addTotal(5);

    EXPECT_CALL(mockSetProgressCallback, Call(_, _)).Times(0);

    counter.addTotal(0);
}

TEST_F(ProgressCounter, RemoveTotal)
{
    counter.addTotal(11);
    counter.addProgress(2);

    EXPECT_CALL(mockSetProgressCallback, Call(2, 4));

    counter.removeTotal(7);
}

TEST_F(ProgressCounter, RemoveTotalZero)
{
    counter.addTotal(11);

    EXPECT_CALL(mockSetProgressCallback, Call(_, _)).Times(0);

    counter.removeTotal(0);
}

TEST_F(ProgressCounter, AddProgress)
{
    counter.addTotal(11);
    counter.addProgress(3);

    EXPECT_CALL(mockSetProgressCallback, Call(7, 11));

    counter.addProgress(4);
}

TEST_F(ProgressCounter, AddProgressZero)
{
    counter.addTotal(11);
    counter.addProgress(3);

    EXPECT_CALL(mockSetProgressCallback, Call(_, _)).Times(0);

    counter.addProgress(0);
}

TEST_F(ProgressCounter, AddTotalAfterFinishingProgress)
{
    counter.addTotal(11);
    counter.addProgress(3);
    counter.addProgress(8);

    EXPECT_CALL(mockSetProgressCallback, Call(0, 9));

    counter.addTotal(9);
}

TEST_F(ProgressCounter, AddProgressAfterFinishingProgress)
{
    counter.addTotal(11);
    counter.addProgress(3);
    counter.addProgress(8);
    counter.addTotal(9);

    EXPECT_CALL(mockSetProgressCallback, Call(4, 9));

    counter.addProgress(4);
}

TEST_F(ProgressCounter, AddTotalAfterFinishingProgressByRemoving)
{
    counter.addTotal(11);
    counter.addProgress(3);
    counter.removeTotal(8);

    EXPECT_CALL(mockSetProgressCallback, Call(0, 9));

    counter.addTotal(9);
}

TEST_F(ProgressCounter, AddProgressAfterFinishingProgressByRemoving)
{
    counter.addTotal(11);
    counter.addProgress(3);
    counter.removeTotal(8);
    counter.addTotal(9);

    EXPECT_CALL(mockSetProgressCallback, Call(4, 9));

    counter.addProgress(4);
}

}
