/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <sourcerangefilter.h>

namespace {

using testing::ContainerEq;
using testing::IsEmpty;

using ClangBackEnd::SourceRangeWithTextContainer;
using ClangBackEnd::SourceRangeWithTextContainers;
using ClangBackEnd::SourceRangesAndDiagnosticsForQueryMessage;

class SourceRangeFilter : public ::testing::Test
{
protected:


protected:
    SourceRangeWithTextContainers sourceRanges1{{1, 1, 1, 1, 2, 1, 4, "foo"},
                                               {2, 1, 1, 1, 2, 1, 4, "foo"},
                                               {1, 1, 1, 1, 2, 2, 5, "foo"}};
    SourceRangeWithTextContainers sourceRanges2{{1, 1, 1, 1, 2, 1, 4, "foo"},
                                                {3, 1, 1, 1, 2, 1, 4, "foo"},
                                                {1, 1, 1, 1, 2, 2, 6, "foo"}};
    SourceRangeWithTextContainers sourceRanges3{{3, 1, 1, 1, 2, 1, 4, "foo"},
                                                {1, 1, 1, 1, 2, 2, 6, "foo"}};
    SourceRangesAndDiagnosticsForQueryMessage message1{{{}, Utils::clone(sourceRanges1)}, {}};
    SourceRangesAndDiagnosticsForQueryMessage message2{{{}, Utils::clone(sourceRanges2)}, {}};
    ClangBackEnd::SourceRangeFilter filter{3};
};

TEST_F(SourceRangeFilter, DontChangeForFirstTime)
{
    auto expectedSourceRanges = sourceRanges1;

    filter.removeDuplicates(sourceRanges1);

    ASSERT_THAT(sourceRanges1, ContainerEq(expectedSourceRanges));
}

TEST_F(SourceRangeFilter, DoNotFilterNonDuplicates)
{
    SourceRangeWithTextContainers expectedSourceRanges = sourceRanges3;
    filter.removeDuplicates(sourceRanges1);

    filter.removeDuplicates(sourceRanges3);

    ASSERT_THAT(sourceRanges3, ContainerEq(expectedSourceRanges));
}

TEST_F(SourceRangeFilter, FilterDuplicates)
{
    filter.removeDuplicates(sourceRanges1);

    filter.removeDuplicates(sourceRanges2);

    ASSERT_THAT(sourceRanges2, ContainerEq(sourceRanges3));
}

TEST_F(SourceRangeFilter, FilterMoreDuplicates)
{
    filter.removeDuplicates(sourceRanges1);
    filter.removeDuplicates(sourceRanges2);

    filter.removeDuplicates(sourceRanges3);

    ASSERT_THAT(sourceRanges3, IsEmpty());
}

TEST_F(SourceRangeFilter, FilterDuplicatesFromMessage)
{
    filter.removeDuplicates(std::move(message1));

    auto filteredMessage = filter.removeDuplicates(std::move(message2));

    ASSERT_THAT(filteredMessage.sourceRanges().sourceRangeWithTextContainers(),
                ContainerEq(sourceRanges3));
}

}
