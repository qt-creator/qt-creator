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

#include <toolchainargumentscache.h>

namespace {

using ClangBackEnd::ProjectPartContainer;
using ClangBackEnd::ProjectPartIds;

MATCHER_P2(IsEntry,
           projectPartIds,
           arguments,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(ClangBackEnd::ArgumentsEntry(Utils::clone(projectPartIds), arguments)))
{
    const ClangBackEnd::ArgumentsEntry &entry= arg;

    return entry.ids == projectPartIds && entry.arguments == arguments;
}

class ToolChainArgumentsCache : public testing::Test
{
public:
    ClangBackEnd::ToolChainsArgumentsCache cache;
    ProjectPartContainer projectPart1{1, {}, {}, {}, {}, {}, {}, {}, {}, {}};
    ProjectPartContainer projectPart2{2, {}, {}, {}, {}, {}, {}, {}, {}, {}};
    ProjectPartContainer projectPart3{3, {}, {}, {}, {}, {}, {}, {}, {}, {}};
    ProjectPartContainer projectPart4{4, {}, {}, {}, {}, {}, {}, {}, {}, {}};
    Utils::SmallStringVector arguments1{"yi", "er"};
    Utils::SmallStringVector arguments2{"san", "se"};
};

TEST_F(ToolChainArgumentsCache, ArgumentsDoesNotExistForProjectPartId)
{
    cache.update({projectPart2, projectPart4}, arguments2);

    ASSERT_THAT(cache.arguments({projectPart1.projectPartId}), IsEmpty());
}

TEST_F(ToolChainArgumentsCache, AddNewArguments)
{
    cache.update({projectPart2, projectPart4}, arguments2);

    cache.update({projectPart1, projectPart3}, arguments1);

    ASSERT_THAT(cache.arguments({projectPart1.projectPartId}),
                ElementsAre(IsEntry(ProjectPartIds{projectPart1.projectPartId}, arguments1)));
}

TEST_F(ToolChainArgumentsCache, AddDifferentProjectParts)
{
    cache.update({projectPart1, projectPart3}, arguments1);

    cache.update({projectPart2, projectPart4}, arguments1);

    ASSERT_THAT(cache.arguments(ProjectPartIds{projectPart2.projectPartId}),
                ElementsAre(IsEntry(ProjectPartIds{projectPart2.projectPartId}, arguments1)));
}

TEST_F(ToolChainArgumentsCache, AddDifferentProjectPartsReverseOrder)
{
    cache.update({projectPart2, projectPart3}, arguments1);

    cache.update({projectPart1, projectPart4}, arguments1);

    ASSERT_THAT(cache.arguments(ProjectPartIds{projectPart2.projectPartId}),
                ElementsAre(IsEntry(ProjectPartIds{projectPart2.projectPartId}, arguments1)));
}

TEST_F(ToolChainArgumentsCache, AddDifferentProjectPartsDoesNotRemoveOldEntry)
{
    cache.update({projectPart1, projectPart3}, arguments1);

    cache.update({projectPart2, projectPart4}, arguments1);

    ASSERT_THAT(cache.arguments(ProjectPartIds{projectPart1.projectPartId}),
                ElementsAre(IsEntry(ProjectPartIds{projectPart1.projectPartId}, arguments1)));
}

TEST_F(ToolChainArgumentsCache, AddSameArgumentsDoesNotIncreseEntryCount)
{
    cache.update({projectPart1, projectPart3}, arguments1);

    cache.update({projectPart2, projectPart4}, arguments1);

    ASSERT_THAT(cache.size(), 1);
}

TEST_F(ToolChainArgumentsCache, AddDifferentArgumentsDoesIncreseEntryCount)
{
    cache.update({projectPart1, projectPart3}, arguments1);

    cache.update({projectPart2, projectPart4}, arguments2);

    ASSERT_THAT(cache.size(), 2);
}

TEST_F(ToolChainArgumentsCache, RemoveIdsFromOtherEntries)
{
    cache.update({projectPart2, projectPart4}, arguments1);
    cache.update({projectPart1, projectPart3}, arguments1);

    cache.update({projectPart2, projectPart4}, arguments2);

    ASSERT_THAT(cache.arguments(ProjectPartIds{projectPart2.projectPartId}),
                ElementsAre(IsEntry(ProjectPartIds{projectPart2.projectPartId}, arguments2)));
}

TEST_F(ToolChainArgumentsCache, RemoveIdsFromOtherEntriesWithArgumentsAlreadyExists)
{
    cache.update({projectPart2, projectPart4}, arguments1);
    cache.update({projectPart1, projectPart3}, arguments2);

    cache.update({projectPart2, projectPart4}, arguments2);

    ASSERT_THAT(cache.arguments({projectPart2.projectPartId}),
                ElementsAre(IsEntry(ProjectPartIds{projectPart2.projectPartId}, arguments2)));
}

TEST_F(ToolChainArgumentsCache, RemoveEntryIfEmpty)
{
    cache.update({projectPart2, projectPart4}, arguments2);
    cache.update({projectPart1, projectPart3}, arguments1);

    cache.update({projectPart2, projectPart4}, arguments1);

    ASSERT_THAT(cache.size(), 1);
}

TEST_F(ToolChainArgumentsCache, GetMutipleEntries)
{
    cache.update({projectPart2, projectPart4}, arguments1);
    cache.update({projectPart1, projectPart3}, arguments2);

    auto arguments = cache.arguments(
        {projectPart1.projectPartId, projectPart2.projectPartId, projectPart3.projectPartId});

    ASSERT_THAT(arguments,
                ElementsAre(IsEntry(ProjectPartIds{projectPart2.projectPartId}, arguments1),
                            IsEntry(ProjectPartIds{projectPart1.projectPartId,
                                                   projectPart3.projectPartId},
                                    arguments2)));
}

TEST_F(ToolChainArgumentsCache, RemoveMutipleIds)
{
    cache.update({projectPart2, projectPart4}, arguments1);
    cache.update({projectPart1, projectPart3}, arguments2);

    cache.remove({projectPart1.projectPartId, projectPart2.projectPartId});

    ASSERT_THAT(cache.arguments({projectPart1.projectPartId,
                                 projectPart2.projectPartId,
                                 projectPart3.projectPartId}),
                ElementsAre(IsEntry(ProjectPartIds{projectPart3.projectPartId}, arguments2)));
}

TEST_F(ToolChainArgumentsCache, RemoveEntriesIfEntryIsEmptyAfterRemovingIds)
{
    cache.update({projectPart2, projectPart4}, arguments1);
    cache.update({projectPart1, projectPart3}, arguments2);

    cache.remove({projectPart1.projectPartId, projectPart3.projectPartId});

    ASSERT_THAT(cache.size(), 1);
}

} // namespace
