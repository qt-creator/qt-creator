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
using ClangBackEnd::SourceRangesForQueryMessage;

class SourceRangeFilter : public ::testing::Test
{
protected:


protected:
    SourceRangeWithTextContainers sourceRanges1{{{1, 1}, 1, 1, 1, 2, 1, 4, "foo"},
                                                {{1, 1}, 1, 1, 1, 2, 2, 5, "foo"},
                                                {{1, 2}, 1, 1, 1, 2, 1, 4, "foo"}};
    SourceRangeWithTextContainers sourceRanges2{{{1, 3}, 1, 1, 1, 2, 1, 4, "foo"},
                                                {{1, 1}, 1, 1, 1, 2, 1, 4, "foo"},
                                                {{1, 1}, 1, 1, 1, 2, 3, 6, "foo"}};
    SourceRangeWithTextContainers sourceRanges3{{{1, 1}, 1, 1, 1, 2, 3, 6, "foo"},
                                                {{1, 3}, 1, 1, 1, 2, 1, 4, "foo"}};
    SourceRangeWithTextContainers sourceRanges4{{{1, 1}, 1, 1, 1, 2, 3, 6, "foo"},
                                                {{1, 3}, 1, 1, 1, 2, 1, 4, "foo"},
                                                {{1, 3}, 1, 1, 1, 2, 1, 4, "foo"}};
    SourceRangeWithTextContainers sourceRanges5{{{1, 3}, 1, 1, 1, 2, 1, 4, "foo"},
                                                {{1, 1}, 1, 1, 1, 2, 3, 6, "foo"}};
    SourceRangesForQueryMessage message1{{Utils::clone(sourceRanges1)}};
    SourceRangesForQueryMessage message2{{Utils::clone(sourceRanges2)}};
    ClangBackEnd::SourceRangeFilter filter{3};
};

TEST_F(SourceRangeFilter, DontChangeForFirstTimeIfElementsAreUnique)
{
    auto sourceRange = filter.removeDuplicates(Utils::clone(sourceRanges1));

    ASSERT_THAT(sourceRange, ContainerEq(sourceRanges1));
}

TEST_F(SourceRangeFilter, DoNotFilterNonDuplicates)
{
    filter.removeDuplicates(Utils::clone(sourceRanges1));

    auto sourceRange = filter.removeDuplicates(Utils::clone(sourceRanges3));

    ASSERT_THAT(sourceRange, ContainerEq(sourceRanges3));
}

TEST_F(SourceRangeFilter, FilterDuplicates)
{
    filter.removeDuplicates(Utils::clone(sourceRanges1));

    auto sourceRange = filter.removeDuplicates(Utils::clone(sourceRanges2));

    ASSERT_THAT(sourceRange, ContainerEq(sourceRanges3));
}

TEST_F(SourceRangeFilter, FilterMoreDuplicates)
{
    filter.removeDuplicates(Utils::clone(sourceRanges1));
    filter.removeDuplicates(Utils::clone(sourceRanges2));

    auto sourceRange = filter.removeDuplicates(Utils::clone(sourceRanges3));

    ASSERT_THAT(sourceRange, IsEmpty());
}

TEST_F(SourceRangeFilter, FilterDuplicatesFromMessage)
{
    filter.removeDuplicates(std::move(message1));

    auto filteredMessage = filter.removeDuplicates(std::move(message2));

    ASSERT_THAT(filteredMessage.sourceRanges().sourceRangeWithTextContainers(),
                ContainerEq(sourceRanges3));
}

TEST_F(SourceRangeFilter, FilterDuplicatesInOneRangeSet)
{
    auto sourceRange = filter.removeDuplicates(Utils::clone(sourceRanges4));

    ASSERT_THAT(sourceRange, ContainerEq(sourceRanges3));
}

TEST_F(SourceRangeFilter, SortSourceRanges)
{
    auto sourceRange = filter.removeDuplicates(Utils::clone(sourceRanges5));

    ASSERT_THAT(sourceRange, ContainerEq(sourceRanges3));
}


}
