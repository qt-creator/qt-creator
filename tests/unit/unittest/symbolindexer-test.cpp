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
#include "mockclangpathwatcher.h"
#include "mocksymbolscollector.h"
#include "mocksymbolstorage.h"
#include "mockfilepathcaching.h"
#include "mocksqlitetransactionbackend.h"

#include <symbolindexer.h>
#include <updatepchprojectpartsmessage.h>
#include <projectpartcontainerv2.h>

namespace {

using Utils::PathString;
using ClangBackEnd::CompilerMacro;
using ClangBackEnd::FileStatuses;
using ClangBackEnd::FilePathId;
using ClangBackEnd::FilePathIds;
using ClangBackEnd::FilePathView;
using ClangBackEnd::V2::ProjectPartContainer;
using ClangBackEnd::V2::ProjectPartContainers;
using ClangBackEnd::V2::FileContainers;
using ClangBackEnd::SymbolEntries;
using ClangBackEnd::SymbolEntry;
using ClangBackEnd::SourceDependencies;
using ClangBackEnd::SourceLocationEntries;
using ClangBackEnd::SourceLocationEntry;
using ClangBackEnd::SymbolType;
using ClangBackEnd::UsedMacros;

MATCHER_P2(IsFileId, directoryId, fileNameId,
          std::string(negation ? "isn't " : "is ")
          + PrintToString(ClangBackEnd::FilePathId(directoryId, fileNameId)))
{
    return arg == ClangBackEnd::FilePathId(directoryId, fileNameId);
}

class SymbolIndexer : public testing::Test
{
protected:
    void SetUp()
    {
        ON_CALL(mockCollector, symbols()).WillByDefault(ReturnRef(symbolEntries));
        ON_CALL(mockCollector, sourceLocations()).WillByDefault(ReturnRef(sourceLocations));
        ON_CALL(mockCollector, sourceFiles()).WillByDefault(ReturnRef(sourceFileIds));
        ON_CALL(mockCollector, usedMacros()).WillByDefault(ReturnRef(usedMacros));
        ON_CALL(mockCollector, fileStatuses()).WillByDefault(ReturnRef(fileStatus));
        ON_CALL(mockCollector, sourceDependencies()).WillByDefault(ReturnRef(sourceDependencies));
        ON_CALL(mockStorage, fetchProjectPartArtefact(A<FilePathId>())).WillByDefault(Return(artefact));
    }

protected:
    ClangBackEnd::FilePathId main1PathId{1, 1};
    ClangBackEnd::FilePathId main2PathId{1, 2};
    ClangBackEnd::FilePathId header2PathId{1, 12};
    ClangBackEnd::FilePathId header1PathId{1, 11};
    PathString generatedFileName = "includecollector_generated_file.h";
    ClangBackEnd::FilePathId generatedFilePathId{1, 21};
    ProjectPartContainer projectPart1{"project1",
                                      {"-I", TESTDATA_DIR, "-Wno-pragma-once-outside-header"},
                                      {{"BAR", "1"}, {"FOO", "1"}},
                                      {header1PathId},
                                      {main1PathId}};
    ProjectPartContainer projectPart2{"project2",
                                      {"-I", TESTDATA_DIR, "-x", "c++-header", "-Wno-pragma-once-outside-header"},
                                      {{"BAR", "1"}, {"FOO", "0"}},
                                      {header2PathId},
                                      {main2PathId}};
    FileContainers unsaved{{{TESTDATA_DIR, "query_simplefunction.h"},
                            "void f();",
                            {}}};
    SymbolEntries symbolEntries{{1, {"function", "function"}}};
    SourceLocationEntries sourceLocations{{1, {1, 1}, {42, 23}, SymbolType::Declaration}};
    FilePathIds sourceFileIds{{1, 1}, {42, 23}};
    UsedMacros usedMacros{{"Foo", {1, 1}}};
    FileStatuses fileStatus{{{1, 2}, 3, 4}};
    SourceDependencies sourceDependencies{{{1, 1}, {1, 2}}, {{1, 1}, {1, 3}}};
    ClangBackEnd::ProjectPartArtefact artefact{"[\"-DFOO\"]", "{\"FOO\":\"1\",\"BAR\":\"1\"}", 74};
    NiceMock<MockSqliteTransactionBackend> mockSqliteTransactionBackend;
    NiceMock<MockSymbolsCollector> mockCollector;
    NiceMock<MockSymbolStorage> mockStorage;
    NiceMock<MockClangPathWatcher> mockPathWatcher;
    NiceMock<MockFilePathCaching> mockFilePathCaching;
    ClangBackEnd::SymbolIndexer indexer{mockCollector, mockStorage, mockPathWatcher, mockFilePathCaching, mockSqliteTransactionBackend};
};

TEST_F(SymbolIndexer, UpdateProjectPartsCallsAddFilesInCollector)
{
    EXPECT_CALL(mockCollector, addFiles(projectPart1.sourcePathIds(), projectPart1.arguments()));

    indexer.updateProjectParts({projectPart1}, Utils::clone(unsaved));
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsClearInCollector)
{
    EXPECT_CALL(mockCollector, clear()).Times(2);

    indexer.updateProjectParts({projectPart1, projectPart2}, Utils::clone(unsaved));
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsAddFilesInCollectorForEveryProjectPart)
{
    EXPECT_CALL(mockCollector, addFiles(_, _)).Times(2);

    indexer.updateProjectParts({projectPart1, projectPart2}, Utils::clone(unsaved));
}

TEST_F(SymbolIndexer, UpdateProjectPartsDoesNotCallAddFilesInCollectorForEmptyEveryProjectParts)
{
    EXPECT_CALL(mockCollector, addFiles(_, _))
            .Times(0);

    indexer.updateProjectParts({}, {});
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallscollectSymbolsInCollector)
{
    EXPECT_CALL(mockCollector, collectSymbols()).Times(2);;

    indexer.updateProjectParts({projectPart1, projectPart2}, Utils::clone(unsaved));
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsSymbolsInCollector)
{
    EXPECT_CALL(mockCollector, symbols()).Times(2);

    indexer.updateProjectParts({projectPart1, projectPart2}, Utils::clone(unsaved));
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsSourceLocationsInCollector)
{
    EXPECT_CALL(mockCollector, sourceLocations()).Times(2);

    indexer.updateProjectParts({projectPart1, projectPart2}, Utils::clone(unsaved));
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsAddUnsavedFilesInCollector)
{
    EXPECT_CALL(mockCollector, addUnsavedFiles(unsaved)).Times(2);

    indexer.updateProjectParts({projectPart1, projectPart2}, Utils::clone(unsaved));
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsAddSymbolsAndSourceLocationsInStorage)
{
    EXPECT_CALL(mockSqliteTransactionBackend, immediateBegin()).Times(2);
    EXPECT_CALL(mockStorage, addSymbolsAndSourceLocations(symbolEntries, sourceLocations)).Times(2);
    EXPECT_CALL(mockSqliteTransactionBackend, commit()).Times(2);

    indexer.updateProjectParts({projectPart1, projectPart2}, Utils::clone(unsaved));
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsUpdateProjectPartsInStorage)
{
    EXPECT_CALL(mockStorage, insertOrUpdateProjectPart(Eq("project1"),
                                                       ElementsAre("-I", TESTDATA_DIR, "-Wno-pragma-once-outside-header"),
                                                       ElementsAre(CompilerMacro{"BAR", "1"}, CompilerMacro{"FOO", "1"})));
    EXPECT_CALL(mockStorage, insertOrUpdateProjectPart(Eq("project2"),
                                                       ElementsAre("-I", TESTDATA_DIR, "-x", "c++-header", "-Wno-pragma-once-outside-header"),
                                                       ElementsAre(CompilerMacro{"BAR", "1"}, CompilerMacro{"FOO", "0"})));

    indexer.updateProjectParts({projectPart1, projectPart2}, Utils::clone(unsaved));
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsUpdateProjectPartSources)
{
    EXPECT_CALL(mockStorage, updateProjectPartSources(TypedEq<Utils::SmallStringView>("project1"), ElementsAre(IsFileId(1, 1), IsFileId(42, 23))));
    EXPECT_CALL(mockStorage, updateProjectPartSources(TypedEq<Utils::SmallStringView>("project2"), ElementsAre(IsFileId(1, 1), IsFileId(42, 23))));

    indexer.updateProjectParts({projectPart1, projectPart2}, Utils::clone(unsaved));
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsInsertOrUpdateUsedMacros)
{
    EXPECT_CALL(mockStorage, insertOrUpdateUsedMacros(Eq(usedMacros)))
            .Times(2);

    indexer.updateProjectParts({projectPart1, projectPart2}, Utils::clone(unsaved));
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsInsertFileStatuses)
{
    EXPECT_CALL(mockStorage, insertFileStatuses(Eq(fileStatus)))
            .Times(2);

    indexer.updateProjectParts({projectPart1, projectPart2}, Utils::clone(unsaved));
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsInsertOrUpdateSourceDependencies)
{
    EXPECT_CALL(mockStorage, insertOrUpdateSourceDependencies(Eq(sourceDependencies)))
            .Times(2);

    indexer.updateProjectParts({projectPart1, projectPart2}, Utils::clone(unsaved));
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsFetchProjectPartArtefacts)
{
    EXPECT_CALL(mockStorage, fetchProjectPartArtefact(TypedEq<Utils::SmallStringView>(projectPart1.projectPartId())));
    EXPECT_CALL(mockStorage, fetchProjectPartArtefact(TypedEq<Utils::SmallStringView>(projectPart2.projectPartId())));

    indexer.updateProjectParts({projectPart1, projectPart2}, Utils::clone(unsaved));
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsInOrder)
{
    InSequence s;

    EXPECT_CALL(mockCollector, clear());
    EXPECT_CALL(mockStorage, fetchProjectPartArtefact(TypedEq<Utils::SmallStringView>(projectPart1.projectPartId())));
    EXPECT_CALL(mockCollector, addFiles(_, _));
    EXPECT_CALL(mockCollector, addUnsavedFiles(unsaved));
    EXPECT_CALL(mockCollector, collectSymbols());
    EXPECT_CALL(mockSqliteTransactionBackend, immediateBegin());
    EXPECT_CALL(mockStorage, addSymbolsAndSourceLocations(symbolEntries, sourceLocations));
    EXPECT_CALL(mockStorage, insertOrUpdateProjectPart(Eq(projectPart1.projectPartId()), Eq(projectPart1.arguments()), Eq(projectPart1.compilerMacros())));
    EXPECT_CALL(mockStorage, updateProjectPartSources(TypedEq<Utils::SmallStringView>(projectPart1.projectPartId()), Eq(sourceFileIds)));
    EXPECT_CALL(mockStorage, insertOrUpdateUsedMacros(Eq(usedMacros)));
    EXPECT_CALL(mockStorage, insertFileStatuses(Eq(fileStatus)));
    EXPECT_CALL(mockStorage, insertOrUpdateSourceDependencies(Eq(sourceDependencies)));
    EXPECT_CALL(mockSqliteTransactionBackend, commit());

    indexer.updateProjectParts({projectPart1}, Utils::clone(unsaved));
}

TEST_F(SymbolIndexer, CallSetNotifier)
{
    EXPECT_CALL(mockPathWatcher, setNotifier(_));

    ClangBackEnd::SymbolIndexer indexer{mockCollector, mockStorage, mockPathWatcher, mockFilePathCaching, mockSqliteTransactionBackend};
}

TEST_F(SymbolIndexer, PathChangedCallsFetchProjectPartArtefactInStorage)
{
    EXPECT_CALL(mockStorage, fetchProjectPartArtefact(sourceFileIds[0]));
    EXPECT_CALL(mockStorage, fetchProjectPartArtefact(sourceFileIds[1]));

    indexer.pathsChanged(sourceFileIds);
}

TEST_F(SymbolIndexer, UpdateChangedPathCallsInOrder)
{
    InSequence s;

    EXPECT_CALL(mockCollector, clear());
    EXPECT_CALL(mockStorage, fetchProjectPartArtefact(sourceFileIds[0]));
    EXPECT_CALL(mockCollector, addFiles(ElementsAre(sourceFileIds[0]), Eq(artefact.compilerArguments)));
    EXPECT_CALL(mockCollector, collectSymbols());
    EXPECT_CALL(mockSqliteTransactionBackend, immediateBegin());
    EXPECT_CALL(mockStorage, addSymbolsAndSourceLocations(symbolEntries, sourceLocations));
    EXPECT_CALL(mockStorage, updateProjectPartSources(artefact.projectPartId, Eq(sourceFileIds)));
    EXPECT_CALL(mockStorage, insertOrUpdateUsedMacros(Eq(usedMacros)));
    EXPECT_CALL(mockStorage, insertFileStatuses(Eq(fileStatus)));
    EXPECT_CALL(mockStorage, insertOrUpdateSourceDependencies(Eq(sourceDependencies)));
    EXPECT_CALL(mockSqliteTransactionBackend, commit());

    indexer.updateChangedPath(sourceFileIds[0]);
}

TEST_F(SymbolIndexer, HandleEmptyOptionalInUpdateChangedPath)
{
    ON_CALL(mockStorage, fetchProjectPartArtefact(A<FilePathId>()))
            .WillByDefault(Return(Utils::optional<ClangBackEnd::ProjectPartArtefact>()));

    EXPECT_CALL(mockCollector, clear());
    EXPECT_CALL(mockStorage, fetchProjectPartArtefact(sourceFileIds[0]));
    EXPECT_CALL(mockCollector, addFiles(_, _)).Times(0);
    EXPECT_CALL(mockCollector, collectSymbols()).Times(0);
    EXPECT_CALL(mockSqliteTransactionBackend, immediateBegin()).Times(0);
    EXPECT_CALL(mockStorage, addSymbolsAndSourceLocations(_, _)).Times(0);
    EXPECT_CALL(mockStorage, updateProjectPartSources(An<int>(), _)).Times(0);
    EXPECT_CALL(mockStorage, insertOrUpdateUsedMacros(_)).Times(0);
    EXPECT_CALL(mockStorage, insertFileStatuses(_)).Times(0);
    EXPECT_CALL(mockStorage, insertOrUpdateSourceDependencies(_)).Times(0);
    EXPECT_CALL(mockSqliteTransactionBackend, commit()).Times(0);

    indexer.updateChangedPath(sourceFileIds[0]);
}

TEST_F(SymbolIndexer, CompilerMacrosAreNotDifferent)
{
    ON_CALL(mockStorage, fetchProjectPartArtefact(An<Utils::SmallStringView>())).WillByDefault(Return(artefact));

    auto areDifferent = indexer.compilerMacrosAreDifferent(projectPart1);

    ASSERT_FALSE(areDifferent);
}

TEST_F(SymbolIndexer, CompilerMacrosAreDifferent)
{
    ON_CALL(mockStorage, fetchProjectPartArtefact(An<Utils::SmallStringView>())).WillByDefault(Return(artefact));

    auto areDifferent = indexer.compilerMacrosAreDifferent(projectPart2);

    ASSERT_TRUE(areDifferent);
}

TEST_F(SymbolIndexer, DontReparseInUpdateProjectPartsIfDefinesAreTheSame)
{
    ON_CALL(mockStorage, fetchProjectPartArtefact(An<Utils::SmallStringView>())).WillByDefault(Return(artefact));

    EXPECT_CALL(mockCollector, clear());
    EXPECT_CALL(mockStorage, fetchProjectPartArtefact(TypedEq<Utils::SmallStringView>(projectPart1.projectPartId())));
    EXPECT_CALL(mockCollector, addFiles(_, _)).Times(0);
    EXPECT_CALL(mockCollector, collectSymbols()).Times(0);
    EXPECT_CALL(mockSqliteTransactionBackend, immediateBegin()).Times(0);
    EXPECT_CALL(mockStorage, addSymbolsAndSourceLocations(_, _)).Times(0);
    EXPECT_CALL(mockStorage, updateProjectPartSources(An<int>(), _)).Times(0);
    EXPECT_CALL(mockStorage, insertOrUpdateUsedMacros(_)).Times(0);
    EXPECT_CALL(mockStorage, insertFileStatuses(_)).Times(0);
    EXPECT_CALL(mockStorage, insertOrUpdateSourceDependencies(_)).Times(0);
    EXPECT_CALL(mockSqliteTransactionBackend, commit()).Times(0);

    indexer.updateProjectPart(std::move(projectPart1), {});
}

}
