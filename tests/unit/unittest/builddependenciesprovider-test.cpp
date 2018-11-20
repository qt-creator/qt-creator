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

#include "mockbuilddependenciesstorage.h"
#include "mockmodifiedtimechecker.h"
#include "mockbuilddependencygenerator.h"
#include "mocksqlitetransactionbackend.h"

#include <builddependenciesprovider.h>

namespace {

using ClangBackEnd::BuildDependency;
using ClangBackEnd::BuildDependencies;
using ClangBackEnd::FilePathId;
using ClangBackEnd::FilePathIds;
using ClangBackEnd::SourceEntry;
using ClangBackEnd::SourceEntries;
using ClangBackEnd::SourceType;
using ClangBackEnd::UsedMacro;
using ClangBackEnd::UsedMacros;

MATCHER_P(HasSourceId, sourceId,  std::string(negation ? "hasn't" : "has")
          + " sourceId " + PrintToString(sourceId))
{
    const SourceEntry & sourceEntry = arg;

    return sourceEntry.sourceId.filePathId == sourceId;
}

class BuildDependenciesProvider : public testing::Test
{
protected:
    NiceMock<MockSqliteTransactionBackend> mockSqliteTransactionBackend;
    NiceMock<MockBuildDependenciesStorage> mockBuildDependenciesStorage;
    NiceMock<MockModifiedTimeChecker> mockModifiedTimeChecker;
    NiceMock<MockBuildDependencyGenerator> mockBuildDependenciesGenerator;
    ClangBackEnd::BuildDependenciesProvider provider{mockBuildDependenciesStorage,
                                                     mockModifiedTimeChecker,
                                                     mockBuildDependenciesGenerator,
                                                     mockSqliteTransactionBackend};
    ClangBackEnd::V2::ProjectPartContainer projectPart1{"ProjectPart1",
                                                        {"--yi"},
                                                        {{"YI", "1"}},
                                                        {"/yi"},
                                                        {1},
                                                        {2}};
    ClangBackEnd::V2::ProjectPartContainer projectPart2{"ProjectPart2",
                                                        {"--er"},
                                                        {{"ER", "2"}},
                                                        {"/er"},
                                                        {1},
                                                        {2, 3, 4}};
    SourceEntries firstSources{{1, SourceType::UserInclude, 1},
                               {2, SourceType::UserInclude, 1},
                               {10, SourceType::UserInclude, 1}};
    SourceEntries secondSources{{1, SourceType::UserInclude, 1},
                                {3, SourceType::UserInclude, 1},
                                {8, SourceType::UserInclude, 1}};
    SourceEntries thirdSources{{4, SourceType::UserInclude, 1},
                               {8, SourceType::UserInclude, 1},
                               {10, SourceType::UserInclude, 1}};
    UsedMacros firstUsedMacros{{"YI", 1}};
    UsedMacros secondUsedMacros{{"LIANG", 2}, {"ER", 2}};
    UsedMacros thirdUsedMacros{{"SAN", 10}};
    FilePathIds sourceFiles{1, 3, 8};
    ClangBackEnd::SourceDependencies sourceDependencies{{1, 3}, {1, 8}};
    ClangBackEnd::FileStatuses fileStatuses{{1, 21, 12, false},
                                            {3, 21, 12, false},
                                            {8, 21, 12, false}};
    BuildDependency buildDependency{
        secondSources,
        secondUsedMacros,
        sourceFiles,
        sourceDependencies,
        fileStatuses
    };
};

TEST_F(BuildDependenciesProvider, CreateCallsFetchDependSourcesFromStorageIfTimeStampsAreUpToDate)
{
    InSequence s;

    EXPECT_CALL(mockSqliteTransactionBackend, deferredBegin());
    EXPECT_CALL(mockBuildDependenciesStorage,
                fetchDependSources({2}, TypedEq<Utils::SmallStringView>("ProjectPart1")))
        .WillRepeatedly(Return(firstSources));
    EXPECT_CALL(mockSqliteTransactionBackend, commit());
    EXPECT_CALL(mockModifiedTimeChecker, isUpToDate(firstSources)).WillRepeatedly(Return(true));
    EXPECT_CALL(mockBuildDependenciesGenerator, create(projectPart1)).Times(0);
    EXPECT_CALL(mockSqliteTransactionBackend, immediateBegin()).Times(0);
    EXPECT_CALL(mockSqliteTransactionBackend, commit()).Times(0);
    EXPECT_CALL(mockSqliteTransactionBackend, deferredBegin());
    EXPECT_CALL(mockSqliteTransactionBackend, commit());

    provider.create(projectPart1);
}

TEST_F(BuildDependenciesProvider, FetchDependSourcesFromStorage)
{
    ON_CALL(mockBuildDependenciesStorage, fetchDependSources({2}, TypedEq<Utils::SmallStringView>("ProjectPart2"))).WillByDefault(Return(firstSources));
    ON_CALL(mockBuildDependenciesStorage, fetchDependSources({3}, TypedEq<Utils::SmallStringView>("ProjectPart2"))).WillByDefault(Return(secondSources));
    ON_CALL(mockBuildDependenciesStorage, fetchDependSources({4}, TypedEq<Utils::SmallStringView>("ProjectPart2"))).WillByDefault(Return(thirdSources));

    ON_CALL(mockModifiedTimeChecker, isUpToDate(_)).WillByDefault(Return(true));

    auto buildDependency = provider.create(projectPart2);

    ASSERT_THAT(buildDependency.includes, ElementsAre(HasSourceId(1), HasSourceId(2), HasSourceId(3), HasSourceId(4), HasSourceId(8), HasSourceId(10)));
}

TEST_F(BuildDependenciesProvider, CreateCallsFetchDependSourcesFromGeneratorIfTimeStampsAreNotUpToDate)
{
    InSequence s;

    EXPECT_CALL(mockSqliteTransactionBackend, deferredBegin());
    EXPECT_CALL(mockBuildDependenciesStorage,
                fetchDependSources({2}, TypedEq<Utils::SmallStringView>("ProjectPart1")))
        .WillRepeatedly(Return(firstSources));
    EXPECT_CALL(mockSqliteTransactionBackend, commit());
    EXPECT_CALL(mockModifiedTimeChecker, isUpToDate(firstSources)).WillRepeatedly(Return(false));
    EXPECT_CALL(mockBuildDependenciesGenerator, create(projectPart1))
        .WillOnce(Return(buildDependency));
    EXPECT_CALL(mockSqliteTransactionBackend, immediateBegin());
    EXPECT_CALL(mockBuildDependenciesStorage, updateSources(Eq(secondSources)));
    EXPECT_CALL(mockBuildDependenciesStorage, insertFileStatuses(Eq(fileStatuses)));
    EXPECT_CALL(mockBuildDependenciesStorage, insertOrUpdateSourceDependencies(Eq(sourceDependencies)));
    EXPECT_CALL(mockBuildDependenciesStorage, insertOrUpdateUsedMacros(Eq(secondUsedMacros)));
    EXPECT_CALL(mockSqliteTransactionBackend, commit());

    provider.create(projectPart1);
}

TEST_F(BuildDependenciesProvider, FetchDependSourcesFromGenerator)
{
    ON_CALL(mockBuildDependenciesStorage, fetchDependSources({2}, TypedEq<Utils::SmallStringView>("ProjectPart1"))).WillByDefault(Return(firstSources));
    ON_CALL(mockModifiedTimeChecker, isUpToDate(_)).WillByDefault(Return(false));
    ON_CALL(mockBuildDependenciesGenerator, create(projectPart1)).WillByDefault(Return(buildDependency));

    auto buildDependency = provider.create(projectPart1);

    ASSERT_THAT(buildDependency.includes, ElementsAre(HasSourceId(1), HasSourceId(3), HasSourceId(8)));
}

TEST_F(BuildDependenciesProvider, CreateCallsFetchUsedMacrosFromStorageIfTimeStampsAreUpToDate)
{
    InSequence s;

    EXPECT_CALL(mockSqliteTransactionBackend, deferredBegin());
    EXPECT_CALL(mockBuildDependenciesStorage, fetchDependSources({2}, TypedEq<Utils::SmallStringView>("ProjectPart1"))).WillRepeatedly(Return(firstSources));
    EXPECT_CALL(mockSqliteTransactionBackend, commit());
    EXPECT_CALL(mockModifiedTimeChecker, isUpToDate(firstSources)).WillRepeatedly(Return(true));
    EXPECT_CALL(mockSqliteTransactionBackend, deferredBegin());
    EXPECT_CALL(mockBuildDependenciesStorage, fetchUsedMacros({1}));
    EXPECT_CALL(mockBuildDependenciesStorage, fetchUsedMacros({2}));
    EXPECT_CALL(mockBuildDependenciesStorage, fetchUsedMacros({10}));
    EXPECT_CALL(mockSqliteTransactionBackend, commit());

    provider.create(projectPart1);
}

TEST_F(BuildDependenciesProvider, FetchUsedMacrosFromStorageIfDependSourcesAreUpToDate)
{
    ON_CALL(mockBuildDependenciesStorage, fetchDependSources({2}, TypedEq<Utils::SmallStringView>("ProjectPart1"))).WillByDefault(Return(firstSources));
    ON_CALL(mockModifiedTimeChecker, isUpToDate(firstSources)).WillByDefault(Return(true));
    ON_CALL(mockBuildDependenciesStorage, fetchUsedMacros({1})).WillByDefault(Return(firstUsedMacros));
    ON_CALL(mockBuildDependenciesStorage, fetchUsedMacros({2})).WillByDefault(Return(secondUsedMacros));
    ON_CALL(mockBuildDependenciesStorage, fetchUsedMacros({10})).WillByDefault(Return(thirdUsedMacros));

    auto buildDependency = provider.create(projectPart1);

    ASSERT_THAT(buildDependency.usedMacros, ElementsAre(UsedMacro{"YI", 1}, UsedMacro{"ER", 2}, UsedMacro{"LIANG", 2}, UsedMacro{"SAN", 10}));
}
}
