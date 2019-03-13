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

#include "mocksqlitedatabase.h"

#include <projectpartsstorage.h>
#include <refactoringdatabaseinitializer.h>
#include <sqlitedatabase.h>
#include <sqlitereadstatement.h>
#include <sqlitewritestatement.h>

namespace {

using ClangBackEnd::FilePathId;
using ClangBackEnd::IncludeSearchPath;
using ClangBackEnd::IncludeSearchPaths;
using ClangBackEnd::IncludeSearchPathType;
using ClangBackEnd::ProjectPartId;
using ClangBackEnd::ProjectPartIds;

class Data
{
protected:
    ClangBackEnd::ProjectPartContainer projectPart1{1,
                                                    {"-m32"},
                                                    {{"FOO", "1", 1}},
                                                    {{"/include", 1, IncludeSearchPathType::System}},
                                                    {{"/home/yi", 2, IncludeSearchPathType::User}},
                                                    {1, 2},
                                                    {3, 4},
                                                    Utils::Language::Cxx,
                                                    Utils::LanguageVersion::CXX14,
                                                    Utils::LanguageExtension::Microsoft};
    ClangBackEnd::ProjectPartContainer projectPart2{2,
                                                    {"-m64"},
                                                    {{"BAR", "2", 1}},
                                                    {{"/usr/include", 1, IncludeSearchPathType::System}},
                                                    {{"/home/er", 2, IncludeSearchPathType::User}},
                                                    {5, 6},
                                                    {7, 8},
                                                    Utils::Language::C,
                                                    Utils::LanguageVersion::C11,
                                                    Utils::LanguageExtension::Gnu};
};

class ProjectPartsStorage : public testing::Test, public Data
{
    using Storage = ClangBackEnd::ProjectPartsStorage<MockSqliteDatabase>;

protected:
    ProjectPartsStorage()
    {
        ON_CALL(fetchProjectPartByIdStatement, valueReturnProjectPartContainer(Eq(1)))
            .WillByDefault(Return(projectPart1));
        ON_CALL(fetchProjectPartByIdStatement, valueReturnProjectPartContainer(Eq(2)))
            .WillByDefault(Return(projectPart2));
        ON_CALL(fetchProjectPartsHeadersByIdStatement, valuesReturnFilePathIds(_, Eq(1)))
            .WillByDefault(Return(projectPart1.headerPathIds));
        ON_CALL(fetchProjectPartsHeadersByIdStatement, valuesReturnFilePathIds(_, Eq(2)))
            .WillByDefault(Return(projectPart2.headerPathIds));
        ON_CALL(fetchProjectPartsSourcesByIdStatement, valuesReturnFilePathIds(_, Eq(1)))
            .WillByDefault(Return(projectPart1.sourcePathIds));
        ON_CALL(fetchProjectPartsSourcesByIdStatement, valuesReturnFilePathIds(_, Eq(2)))
            .WillByDefault(Return(projectPart2.sourcePathIds));
    }
    NiceMock<MockSqliteDatabase> mockDatabase;
    Storage storage{mockDatabase};
    MockSqliteReadStatement &fetchProjectPartIdStatement = storage.fetchProjectPartIdStatement;
    MockSqliteWriteStatement &insertProjectPartNameStatement = storage.insertProjectPartNameStatement;
    MockSqliteReadStatement &fetchProjectPartNameStatement = storage.fetchProjectPartNameStatement;
    MockSqliteReadStatement &fetchProjectPartsStatement = storage.fetchProjectPartsStatement;
    MockSqliteReadStatement &fetchProjectPartByIdStatement = storage.fetchProjectPartByIdStatement;
    MockSqliteWriteStatement &updateProjectPartStatement = storage.updateProjectPartStatement;
    MockSqliteReadStatement &getProjectPartArtefactsBySourceId = storage.getProjectPartArtefactsBySourceId;
    MockSqliteReadStatement &getProjectPartArtefactsByProjectPartId = storage.getProjectPartArtefactsByProjectPartId;
    MockSqliteWriteStatement &deleteProjectPartsHeadersByIdStatement = storage.deleteProjectPartsHeadersByIdStatement;
    MockSqliteWriteStatement &deleteProjectPartsSourcesByIdStatement = storage.deleteProjectPartsSourcesByIdStatement;
    MockSqliteWriteStatement &insertProjectPartsHeadersStatement = storage.insertProjectPartsHeadersStatement;
    MockSqliteWriteStatement &insertProjectPartsSourcesStatement = storage.insertProjectPartsSourcesStatement;
    MockSqliteReadStatement &fetchProjectPartsHeadersByIdStatement = storage.fetchProjectPartsHeadersByIdStatement;
    MockSqliteReadStatement &fetchProjectPartsSourcesByIdStatement = storage.fetchProjectPartsSourcesByIdStatement;
    IncludeSearchPaths systemIncludeSearchPaths{{"/includes", 1, IncludeSearchPathType::BuiltIn},
                                                {"/other/includes", 2, IncludeSearchPathType::System}};
    IncludeSearchPaths projectIncludeSearchPaths{{"/project/includes", 1, IncludeSearchPathType::User},
                                                 {"/other/project/includes",
                                                  2,
                                                  IncludeSearchPathType::User}};
    Utils::SmallString systemIncludeSearchPathsText{R"([["/includes",1,2],["/other/includes",2,3]])"};
    Utils::SmallString projectIncludeSearchPathsText{
        R"([["/project/includes",1,1],["/other/project/includes",2,1]])"};
    ClangBackEnd::ProjectPartArtefact artefact{R"(["-DFOO"])",
                                               R"([["FOO","1",1]])",
                                               systemIncludeSearchPathsText,
                                               projectIncludeSearchPathsText,
                                               74,
                                               Utils::Language::Cxx,
                                               Utils::LanguageVersion::CXX11,
                                               Utils::LanguageExtension::None};
};

TEST_F(ProjectPartsStorage, CallsFetchProjectIdWithNonExistingProjectPartName)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, deferredBegin());
    EXPECT_CALL(fetchProjectPartIdStatement,
                valueReturnProjectPartId(TypedEq<Utils::SmallStringView>("test")));
    EXPECT_CALL(insertProjectPartNameStatement, write(TypedEq<Utils::SmallStringView>("test")));
    EXPECT_CALL(mockDatabase, commit());

    storage.fetchProjectPartId("test");
}

TEST_F(ProjectPartsStorage, CallsFetchProjectIdWithExistingProjectPart)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, deferredBegin());
    EXPECT_CALL(fetchProjectPartIdStatement,
                valueReturnProjectPartId(TypedEq<Utils::SmallStringView>("test")))
        .WillOnce(Return(Utils::optional<ProjectPartId>{20}));
    EXPECT_CALL(insertProjectPartNameStatement, write(TypedEq<Utils::SmallStringView>("test"))).Times(0);
    EXPECT_CALL(mockDatabase, commit());

    storage.fetchProjectPartId("test");
}

TEST_F(ProjectPartsStorage, CallsFetchProjectIdWithBusyDatabaset)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, deferredBegin());
    EXPECT_CALL(fetchProjectPartIdStatement,
                valueReturnProjectPartId(TypedEq<Utils::SmallStringView>("test")));
    EXPECT_CALL(insertProjectPartNameStatement, write(TypedEq<Utils::SmallStringView>("test")))
        .WillOnce(Throw(Sqlite::StatementIsBusy("busy")));
    EXPECT_CALL(mockDatabase, rollback());
    EXPECT_CALL(mockDatabase, deferredBegin());
    EXPECT_CALL(fetchProjectPartIdStatement,
                valueReturnProjectPartId(TypedEq<Utils::SmallStringView>("test")));
    EXPECT_CALL(insertProjectPartNameStatement, write(TypedEq<Utils::SmallStringView>("test")));
    EXPECT_CALL(mockDatabase, commit());

    storage.fetchProjectPartId("test");
}

TEST_F(ProjectPartsStorage, FetchProjectIdWithNonExistingProjectPartName)
{
    ON_CALL(fetchProjectPartIdStatement,
            valueReturnProjectPartId(TypedEq<Utils::SmallStringView>("test")))
        .WillByDefault(Return(Utils::optional<ProjectPartId>{}));
    ON_CALL(mockDatabase, lastInsertedRowId()).WillByDefault(Return(21));

    auto id = storage.fetchProjectPartId("test");

    ASSERT_THAT(id.projectPathId, 21);
}

TEST_F(ProjectPartsStorage, FetchProjectIdWithNonExistingProjectPartNameAndIsBusy)
{
    InSequence s;
    EXPECT_CALL(fetchProjectPartIdStatement,
                valueReturnProjectPartId(TypedEq<Utils::SmallStringView>("test")))
        .WillOnce(Throw(Sqlite::StatementIsBusy("busy")));
    EXPECT_CALL(fetchProjectPartIdStatement,
                valueReturnProjectPartId(TypedEq<Utils::SmallStringView>("test")))
        .WillOnce(Return(ClangBackEnd::ProjectPartId{21}));
    ON_CALL(mockDatabase, lastInsertedRowId()).WillByDefault(Return(21));

    auto id = storage.fetchProjectPartId("test");

    ASSERT_THAT(id.projectPathId, 21);
}

TEST_F(ProjectPartsStorage, FetchProjectIdWithExistingProjectPartName)
{
    ON_CALL(fetchProjectPartIdStatement,
            valueReturnProjectPartId(TypedEq<Utils::SmallStringView>("test")))
        .WillByDefault(Return(Utils::optional<ProjectPartId>{20}));

    auto id = storage.fetchProjectPartId("test");

    ASSERT_THAT(id.projectPathId, 20);
}

TEST_F(ProjectPartsStorage, FetchProjectPartName)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, deferredBegin());
    EXPECT_CALL(fetchProjectPartNameStatement, valueReturnPathString(TypedEq<int>(12)))
        .WillOnce(Return(Utils::optional<Utils::PathString>{"test"}));
    EXPECT_CALL(mockDatabase, commit());

    storage.fetchProjectPartName(12);
}

TEST_F(ProjectPartsStorage, FetchProjectPartNameStatementIsBusy)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, deferredBegin());
    EXPECT_CALL(fetchProjectPartNameStatement, valueReturnPathString(TypedEq<int>(12)))
        .WillOnce(Throw(Sqlite::StatementIsBusy{""}));
    EXPECT_CALL(mockDatabase, rollback());
    EXPECT_CALL(mockDatabase, deferredBegin());
    EXPECT_CALL(fetchProjectPartNameStatement, valueReturnPathString(TypedEq<int>(12)))
        .WillOnce(Return(Utils::optional<Utils::PathString>{"test"}));
    EXPECT_CALL(mockDatabase, commit());

    storage.fetchProjectPartName(12);
}

TEST_F(ProjectPartsStorage, FetchProjectParts)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, deferredBegin());
    EXPECT_CALL(fetchProjectPartsStatement, valuesReturnProjectPartContainers(4096));
    EXPECT_CALL(mockDatabase, commit());

    storage.fetchProjectParts();
}

TEST_F(ProjectPartsStorage, FetchProjectPartsByIds)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, deferredBegin());
    EXPECT_CALL(fetchProjectPartByIdStatement, valueReturnProjectPartContainer(Eq(1)));
    EXPECT_CALL(fetchProjectPartByIdStatement, valueReturnProjectPartContainer(Eq(2)));
    EXPECT_CALL(mockDatabase, commit());

    storage.fetchProjectParts({1, 2});
}

TEST_F(ProjectPartsStorage, FetchProjectPartsByIdsIsBusy)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, deferredBegin());
    EXPECT_CALL(fetchProjectPartByIdStatement, valueReturnProjectPartContainer(Eq(1)));
    EXPECT_CALL(fetchProjectPartByIdStatement, valueReturnProjectPartContainer(Eq(2)))
        .WillOnce(Throw(Sqlite::StatementIsBusy{""}));
    EXPECT_CALL(mockDatabase, rollback());
    EXPECT_CALL(mockDatabase, deferredBegin());
    EXPECT_CALL(fetchProjectPartByIdStatement, valueReturnProjectPartContainer(Eq(1)));
    EXPECT_CALL(fetchProjectPartByIdStatement, valueReturnProjectPartContainer(Eq(2)));
    EXPECT_CALL(mockDatabase, commit());

    storage.fetchProjectParts({1, 2});
}

TEST_F(ProjectPartsStorage, FetchProjectPartsByIdsHasMissingId)
{
    auto projectParts = storage.fetchProjectParts({1, 2, 3});

    ASSERT_THAT(projectParts, ElementsAre(projectPart1, projectPart2));
}

TEST_F(ProjectPartsStorage, ConvertStringsToJson)
{
    Utils::SmallStringVector strings{"foo", "bar", "foo"};

    auto jsonText = storage.toJson(strings);

    ASSERT_THAT(jsonText, Eq("[\"foo\",\"bar\",\"foo\"]"));
}

TEST_F(ProjectPartsStorage, UpdateProjectParts)
{
    InSequence sequence;

    EXPECT_CALL(mockDatabase, immediateBegin());
    EXPECT_CALL(updateProjectPartStatement,
                write(TypedEq<int>(1),
                      TypedEq<Utils::SmallStringView>(R"(["-m32"])"),
                      TypedEq<Utils::SmallStringView>(R"([["FOO","1",1]])"),
                      TypedEq<Utils::SmallStringView>(R"([["/include",1,3]])"),
                      TypedEq<Utils::SmallStringView>(R"([["/home/yi",2,1]])"),
                      1,
                      35,
                      2));
    EXPECT_CALL(deleteProjectPartsHeadersByIdStatement, write(TypedEq<int>(1)));
    EXPECT_CALL(insertProjectPartsHeadersStatement, write(TypedEq<int>(1), TypedEq<int>(1)));
    EXPECT_CALL(insertProjectPartsHeadersStatement, write(TypedEq<int>(1), TypedEq<int>(2)));
    EXPECT_CALL(deleteProjectPartsSourcesByIdStatement, write(TypedEq<int>(1)));
    EXPECT_CALL(insertProjectPartsSourcesStatement, write(TypedEq<int>(1), TypedEq<int>(3)));
    EXPECT_CALL(insertProjectPartsSourcesStatement, write(TypedEq<int>(1), TypedEq<int>(4)));
    EXPECT_CALL(updateProjectPartStatement,
                write(TypedEq<int>(2),
                      TypedEq<Utils::SmallStringView>(R"(["-m64"])"),
                      TypedEq<Utils::SmallStringView>(R"([["BAR","2",1]])"),
                      TypedEq<Utils::SmallStringView>(R"([["/usr/include",1,3]])"),
                      TypedEq<Utils::SmallStringView>(R"([["/home/er",2,1]])"),
                      0,
                      2,
                      1));
    EXPECT_CALL(deleteProjectPartsHeadersByIdStatement, write(TypedEq<int>(2)));
    EXPECT_CALL(insertProjectPartsHeadersStatement, write(TypedEq<int>(2), TypedEq<int>(5)));
    EXPECT_CALL(insertProjectPartsHeadersStatement, write(TypedEq<int>(2), TypedEq<int>(6)));
    EXPECT_CALL(deleteProjectPartsSourcesByIdStatement, write(TypedEq<int>(2)));
    EXPECT_CALL(insertProjectPartsSourcesStatement, write(TypedEq<int>(2), TypedEq<int>(7)));
    EXPECT_CALL(insertProjectPartsSourcesStatement, write(TypedEq<int>(2), TypedEq<int>(8)));
    EXPECT_CALL(mockDatabase, commit());

    storage.updateProjectParts({projectPart1, projectPart2});
}

TEST_F(ProjectPartsStorage, UpdateProjectPartsIsBusy)
{
    InSequence sequence;

    EXPECT_CALL(mockDatabase, immediateBegin()).WillOnce(Throw(Sqlite::StatementIsBusy{""}));
    EXPECT_CALL(mockDatabase, immediateBegin());
    EXPECT_CALL(updateProjectPartStatement,
                write(TypedEq<int>(1),
                      TypedEq<Utils::SmallStringView>(R"(["-m32"])"),
                      TypedEq<Utils::SmallStringView>(R"([["FOO","1",1]])"),
                      TypedEq<Utils::SmallStringView>(R"([["/include",1,3]])"),
                      TypedEq<Utils::SmallStringView>(R"([["/home/yi",2,1]])"),
                      1,
                      35,
                      2));
    EXPECT_CALL(mockDatabase, commit());

    storage.updateProjectParts({projectPart1});
}

TEST_F(ProjectPartsStorage, FetchProjectPartArtefactBySourceIdCallsValueInStatement)
{
    EXPECT_CALL(getProjectPartArtefactsBySourceId, valueReturnProjectPartArtefact(1))
        .WillRepeatedly(Return(artefact));

    storage.fetchProjectPartArtefact(FilePathId{1});
}

TEST_F(ProjectPartsStorage, FetchProjectPartArtefactBySourceIdReturnArtefact)
{
    EXPECT_CALL(getProjectPartArtefactsBySourceId, valueReturnProjectPartArtefact(1))
        .WillRepeatedly(Return(artefact));

    auto result = storage.fetchProjectPartArtefact(FilePathId{1});

    ASSERT_THAT(result, Eq(artefact));
}

TEST_F(ProjectPartsStorage, FetchProjectPartArtefactByProjectPartIdCallsValueInStatement)
{
    EXPECT_CALL(getProjectPartArtefactsByProjectPartId, valueReturnProjectPartArtefact(74))
        .WillRepeatedly(Return(artefact));

    storage.fetchProjectPartArtefact(ProjectPartId{74});
}

TEST_F(ProjectPartsStorage, FetchProjectPartArtefactByProjectPartIdReturnArtefact)
{
    EXPECT_CALL(getProjectPartArtefactsByProjectPartId, valueReturnProjectPartArtefact(74))
        .WillRepeatedly(Return(artefact));

    auto result = storage.fetchProjectPartArtefact(ProjectPartId{74});

    ASSERT_THAT(result, Eq(artefact));
}

class ProjectPartsStorageSlow : public testing::Test, public Data
{
    using Storage = ClangBackEnd::ProjectPartsStorage<Sqlite::Database>;

protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> databaseInitializer{database};
    Storage storage{database};
};

TEST_F(ProjectPartsStorageSlow, FetchProjectPartName)
{
    auto id = storage.fetchProjectPartId("test");

    auto name = storage.fetchProjectPartName(id);

    ASSERT_THAT(name, "test");
}

TEST_F(ProjectPartsStorageSlow, FetchNonExistingProjectPartName)
{
    ASSERT_THROW(storage.fetchProjectPartName(ClangBackEnd::ProjectPartId{1}),
                 ClangBackEnd::ProjectPartDoesNotExists);
}

TEST_F(ProjectPartsStorageSlow, FetchProjectPartId)
{
    auto first = storage.fetchProjectPartId("test");

    auto second = storage.fetchProjectPartId("test");

    ASSERT_THAT(first, Eq(second));
}

TEST_F(ProjectPartsStorageSlow, FetchProjectParts)
{
    projectPart1.projectPartId = storage.fetchProjectPartId("project1");
    projectPart2.projectPartId = storage.fetchProjectPartId("project2");
    storage.updateProjectParts({projectPart1, projectPart2});

    auto projectParts = storage.fetchProjectParts(
        {projectPart1.projectPartId, projectPart2.projectPartId});

    ASSERT_THAT(projectParts, ElementsAre(projectPart1, projectPart2));
}
} // namespace
