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
#include "mockbuilddependenciesstorage.h"

#include <filepathcaching.h>
#include <filestatuscache.h>
#include <projectpartcontainerv2.h>
#include <refactoringdatabaseinitializer.h>
#include <processormanager.h>
#include <symbolindexer.h>
#include <symbolindexertaskqueue.h>
#include <taskscheduler.h>
#include <updateprojectpartsmessage.h>

#include <QCoreApplication>
#include <QDateTime>

#include <fstream>

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
using ClangBackEnd::SymbolIndexerTask;
using ClangBackEnd::SymbolIndexerTaskQueue;
using ClangBackEnd::TaskScheduler;
using ClangBackEnd::ProcessorManager;
using ClangBackEnd::SourceDependencies;
using ClangBackEnd::SourceLocationEntries;
using ClangBackEnd::SourceLocationEntry;
using ClangBackEnd::SymbolKind;
using ClangBackEnd::SourceLocationKind;
using ClangBackEnd::UsedMacros;
using OptionalProjectPartArtefact = Utils::optional<ClangBackEnd::ProjectPartArtefact>;

MATCHER_P(IsFileId, fileNameId,
          std::string(negation ? "isn't " : "is ")
          + PrintToString(ClangBackEnd::FilePathId(fileNameId)))
{
    return arg == ClangBackEnd::FilePathId(fileNameId);
}

struct Data
{
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> databaseInitializer{database};
    ClangBackEnd::FilePathCaching filePathCache{database};
};

class Manager final : public ProcessorManager<NiceMock<MockSymbolsCollector>>
{
public:
    using Processor = NiceMock<MockSymbolsCollector>;
    Manager(const ClangBackEnd::GeneratedFiles &generatedFiles)
        : ProcessorManager(generatedFiles)
    {}

protected:
    std::unique_ptr<NiceMock<MockSymbolsCollector>> createProcessor() const
    {
        return std::make_unique<NiceMock<MockSymbolsCollector>>();
    }
};

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
        ON_CALL(mockSymbolStorage, fetchProjectPartArtefact(A<FilePathId>())).WillByDefault(Return(artefact));
        ON_CALL(mockBuildDependenciesStorage, fetchLowestLastModifiedTime(A<FilePathId>())).WillByDefault(Return(-1));

        mockCollector.setIsUsed(false);

        generatedFiles.update(unsaved);
    }

    void TearDown()
    {
        syncTasks();
    }

    void syncTasks()
    {
        while (!indexerQueue.tasks().empty() || !indexerScheduler.futures().empty()) {
            indexerScheduler.syncTasks();
            QCoreApplication::processEvents();
        }
    }

    static void SetUpTestCase()
    {
        data = std::make_unique<Data>();
    }

    static void TearDownTestCase()
    {
        data.reset();
    }

    void touchFile(FilePathId filePathId)
    {
        std::ofstream ostream(std::string(filePathCache.filePath(filePathId)), std::ios::binary);
        ostream.write("\n", 1);
        ostream.close();
    }

    FilePathId filePathId(Utils::SmallStringView path) const
    {
        return filePathCache.filePathId(ClangBackEnd::FilePathView(path));
    }

protected:
    static std::unique_ptr<Data> data; // it can be non const because data holds no tested classes
    using Scheduler = TaskScheduler<Manager, ClangBackEnd::SymbolIndexerTask::Callable>;
    ClangBackEnd::FilePathCaching &filePathCache = data->filePathCache;
    ClangBackEnd::FilePathId main1PathId{filePathId(TESTDATA_DIR "/symbolindexer_main1.cpp")};
    ClangBackEnd::FilePathId main2PathId{filePathId(TESTDATA_DIR "/symbolindexer_main2.cpp")};
    ClangBackEnd::FilePathId header2PathId{filePathId(TESTDATA_DIR "/symbolindexer_header1.h")};
    ClangBackEnd::FilePathId header1PathId{filePathId(TESTDATA_DIR "/symbolindexer_header2.h")};
    PathString generatedFileName = "BuildDependencyCollector_generated_file.h";
    ClangBackEnd::FilePathId generatedFilePathId21;
    ProjectPartContainer projectPart1{"project1",
                                      {"-I", TESTDATA_DIR, "-Wno-pragma-once-outside-header"},
                                      {{"BAR", "1"}, {"FOO", "1"}},
                                      {"/includes"},
                                      {header1PathId},
                                      {main1PathId}};
    ProjectPartContainer projectPart2{"project2",
                                      {"-I", TESTDATA_DIR, "-x", "c++-header", "-Wno-pragma-once-outside-header"},
                                      {{"BAR", "1"}, {"FOO", "0"}},
                                      {"/includes"},
                                      {header2PathId},
                                      {main2PathId}};
    ProjectPartContainer projectPart3{"project3",
                                      {"-I", TESTDATA_DIR, "-Wno-pragma-once-outside-header"},
                                      {{"BAR", "1"}, {"FOO", "1"}},
                                      {"/includes", "/other/includes"},
                                      {header1PathId},
                                      {main1PathId}};
    FileContainers unsaved{{{TESTDATA_DIR, "query_simplefunction.h"},
                            "void f();",
                            {}}};
    SymbolEntries symbolEntries{{1, {"function", "function", SymbolKind::Function}}};
    SourceLocationEntries sourceLocations{{1, 1, {42, 23}, SourceLocationKind::Declaration}};
    FilePathIds sourceFileIds{1, 23};
    UsedMacros usedMacros{{"Foo", 1}};
    FileStatuses fileStatus{{2, 3, 4, false}};
    SourceDependencies sourceDependencies{{1, 2}, {1, 3}};
    ClangBackEnd::ProjectPartArtefact artefact{"[\"-DFOO\"]", "{\"FOO\":\"1\",\"BAR\":\"1\"}", "[\"/includes\"]", 74};
    ClangBackEnd::ProjectPartArtefact emptyArtefact{"", "", "", 74};
    Utils::optional<ClangBackEnd::ProjectPartArtefact > nullArtefact;
    ClangBackEnd::ProjectPartPch projectPartPch{"/path/to/pch", 4};
    NiceMock<MockSqliteTransactionBackend> mockSqliteTransactionBackend;
    NiceMock<MockSymbolStorage> mockSymbolStorage;
    NiceMock<MockBuildDependenciesStorage> mockBuildDependenciesStorage;
    NiceMock<MockClangPathWatcher> mockPathWatcher;
    ClangBackEnd::FileStatusCache fileStatusCache{filePathCache};
    ClangBackEnd::GeneratedFiles generatedFiles;
    Manager collectorManger{generatedFiles};
    Scheduler indexerScheduler{collectorManger, indexerQueue, 1};
    SymbolIndexerTaskQueue indexerQueue{indexerScheduler};
    ClangBackEnd::SymbolIndexer indexer{indexerQueue,
                                        mockSymbolStorage,
                                        mockBuildDependenciesStorage,
                                        mockPathWatcher,
                                        filePathCache,
                                        fileStatusCache,
                                        mockSqliteTransactionBackend};
    MockSymbolsCollector &mockCollector{static_cast<MockSymbolsCollector&>(collectorManger.unusedProcessor())};
};

std::unique_ptr<Data> SymbolIndexer::data;

TEST_F(SymbolIndexer, UpdateProjectPartsCallsAddFilesInCollector)
{
    EXPECT_CALL(mockCollector, setFile(main1PathId, projectPart1.arguments));

    indexer.updateProjectParts({projectPart1});
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsAddFilesWithPrecompiledHeaderInCollector)
{
    ON_CALL(mockSymbolStorage, fetchProjectPartArtefact(TypedEq<Utils::SmallStringView>(projectPart1.projectPartId))).WillByDefault(Return(emptyArtefact));
    ON_CALL(mockSymbolStorage, fetchPrecompiledHeader(Eq(artefact.projectPartId))).WillByDefault(Return(projectPartPch));

    EXPECT_CALL(mockCollector, setFile(main1PathId,
                                       ElementsAre(Eq("-I"),
                                                   Eq(TESTDATA_DIR),
                                                   Eq("-Wno-pragma-once-outside-header"),
                                                   Eq("-Xclang"),
                                                   Eq("-include-pch"),
                                                   Eq("-Xclang"),
                                                   Eq("/path/to/pch"))));

    indexer.updateProjectParts({projectPart1});
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsAddFilesWithoutPrecompiledHeaderInCollector)
{
    ON_CALL(mockSymbolStorage, fetchProjectPartArtefact(TypedEq<Utils::SmallStringView>(projectPart1.projectPartId))).WillByDefault(Return(emptyArtefact));

    EXPECT_CALL(mockCollector, setFile(main1PathId,
                                       ElementsAre(Eq("-I"),
                                                   Eq(TESTDATA_DIR),
                                                   Eq("-Wno-pragma-once-outside-header"))));

    indexer.updateProjectParts({projectPart1});
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsClearInCollector)
{
    EXPECT_CALL(mockCollector, clear()).Times(2);

    indexer.updateProjectParts({projectPart1, projectPart2});
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsAddFilesInCollectorForEveryProjectPart)
{
    EXPECT_CALL(mockCollector, setFile(_, _)).Times(2);

    indexer.updateProjectParts({projectPart1, projectPart2});
}

TEST_F(SymbolIndexer, UpdateProjectPartsDoesNotCallAddFilesInCollectorForEmptyEveryProjectParts)
{
    EXPECT_CALL(mockCollector, setFile(_, _))
            .Times(0);

    indexer.updateProjectParts({});
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallscollectSymbolsInCollector)
{
    EXPECT_CALL(mockCollector, collectSymbols()).Times(2);;

    indexer.updateProjectParts({projectPart1, projectPart2});
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsSymbolsInCollector)
{
    EXPECT_CALL(mockCollector, symbols()).Times(2);

    indexer.updateProjectParts({projectPart1, projectPart2});
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsSourceLocationsInCollector)
{
    EXPECT_CALL(mockCollector, sourceLocations()).Times(2);

    indexer.updateProjectParts({projectPart1, projectPart2});
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsAddUnsavedFilesInCollector)
{
    EXPECT_CALL(mockCollector, setUnsavedFiles(unsaved)).Times(2);

    indexer.updateProjectParts({projectPart1, projectPart2});
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsAddSymbolsAndSourceLocationsInStorage)
{
    EXPECT_CALL(mockSymbolStorage, addSymbolsAndSourceLocations(symbolEntries, sourceLocations)).Times(2);

    indexer.updateProjectParts({projectPart1, projectPart2});
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsUpdateProjectPartsInStorage)
{
    EXPECT_CALL(mockSymbolStorage, insertOrUpdateProjectPart(Eq("project1"),
                                                       ElementsAre("-I", TESTDATA_DIR, "-Wno-pragma-once-outside-header"),
                                                       ElementsAre(CompilerMacro{"BAR", "1"}, CompilerMacro{"FOO", "1"}),
                                                       ElementsAre("/includes")));
    EXPECT_CALL(mockSymbolStorage, insertOrUpdateProjectPart(Eq("project2"),
                                                       ElementsAre("-I", TESTDATA_DIR, "-x", "c++-header", "-Wno-pragma-once-outside-header"),
                                                       ElementsAre(CompilerMacro{"BAR", "1"}, CompilerMacro{"FOO", "0"}),
                                                       ElementsAre("/includes")));

    indexer.updateProjectParts({projectPart1, projectPart2});
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsUpdateProjectPartSourcesWithArtifact)
{
    ON_CALL(mockSymbolStorage, fetchProjectPartArtefact(TypedEq<Utils::SmallStringView>("project1"))).WillByDefault(Return(artefact));
    ON_CALL(mockSymbolStorage, insertOrUpdateProjectPart(Eq("project1"), _, _, _)).WillByDefault(Return(-1));

    EXPECT_CALL(mockSymbolStorage, updateProjectPartSources(_, _));

    indexer.updateProjectParts({projectPart1});
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsUpdateProjectPartSourcesWithoutArtifact)
{
    ON_CALL(mockSymbolStorage, fetchProjectPartArtefact(TypedEq<Utils::SmallStringView>("project2"))).WillByDefault(Return(nullArtefact));
    ON_CALL(mockSymbolStorage, insertOrUpdateProjectPart(Eq("project2"), _, _, _)).WillByDefault(Return(3));

    EXPECT_CALL(mockSymbolStorage, updateProjectPartSources(3, ElementsAre(IsFileId(1), IsFileId(23))));

    indexer.updateProjectParts({projectPart2});
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsInsertOrUpdateUsedMacros)
{
    EXPECT_CALL(mockBuildDependenciesStorage, insertOrUpdateUsedMacros(Eq(usedMacros)))
            .Times(2);

    indexer.updateProjectParts({projectPart1, projectPart2});
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsInsertFileStatuses)
{
    EXPECT_CALL(mockBuildDependenciesStorage, insertFileStatuses(Eq(fileStatus)))
            .Times(2);

    indexer.updateProjectParts({projectPart1, projectPart2});
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsInsertOrUpdateSourceDependencies)
{
    EXPECT_CALL(mockBuildDependenciesStorage, insertOrUpdateSourceDependencies(Eq(sourceDependencies)))
            .Times(2);

    indexer.updateProjectParts({projectPart1, projectPart2});
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsFetchProjectPartArtefacts)
{
    EXPECT_CALL(mockSymbolStorage, fetchProjectPartArtefact(TypedEq<Utils::SmallStringView>(projectPart1.projectPartId)));
    EXPECT_CALL(mockSymbolStorage, fetchProjectPartArtefact(TypedEq<Utils::SmallStringView>(projectPart2.projectPartId)));

    indexer.updateProjectParts({projectPart1, projectPart2});
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsInOrderWithoutProjectPartArtifact)
{
    InSequence s;

    EXPECT_CALL(mockSqliteTransactionBackend, immediateBegin());
    EXPECT_CALL(mockSymbolStorage, fetchProjectPartArtefact(TypedEq<Utils::SmallStringView>(projectPart1.projectPartId))).WillOnce(Return(nullArtefact));
    EXPECT_CALL(mockSymbolStorage, insertOrUpdateProjectPart(Eq(projectPart1.projectPartId), Eq(projectPart1.arguments), Eq(projectPart1.compilerMacros), Eq(projectPart1.includeSearchPaths))).WillOnce(Return(12));
    EXPECT_CALL(mockSymbolStorage, fetchPrecompiledHeader(Eq(12)));
    EXPECT_CALL(mockBuildDependenciesStorage, fetchLowestLastModifiedTime(Eq(main1PathId))).Times(0);
    EXPECT_CALL(mockSqliteTransactionBackend, commit());
    EXPECT_CALL(mockCollector, setFile(main1PathId, projectPart1.arguments));
    EXPECT_CALL(mockCollector, collectSymbols());
    EXPECT_CALL(mockSqliteTransactionBackend, immediateBegin());
    EXPECT_CALL(mockSymbolStorage, addSymbolsAndSourceLocations(symbolEntries, sourceLocations));
    EXPECT_CALL(mockSymbolStorage, updateProjectPartSources(TypedEq<int>(12), Eq(sourceFileIds)));
    EXPECT_CALL(mockBuildDependenciesStorage, insertOrUpdateUsedMacros(Eq(usedMacros)));
    EXPECT_CALL(mockBuildDependenciesStorage, insertFileStatuses(Eq(fileStatus)));
    EXPECT_CALL(mockBuildDependenciesStorage, insertOrUpdateSourceDependencies(Eq(sourceDependencies)));
    EXPECT_CALL(mockSqliteTransactionBackend, commit());

    indexer.updateProjectParts({projectPart1});
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsInOrderWithProjectPartArtifact)
{
    InSequence s;

    EXPECT_CALL(mockSqliteTransactionBackend, immediateBegin());
    EXPECT_CALL(mockSymbolStorage, fetchProjectPartArtefact(TypedEq<Utils::SmallStringView>(projectPart1.projectPartId))).WillRepeatedly(Return(artefact));
    EXPECT_CALL(mockSymbolStorage, insertOrUpdateProjectPart(Eq(projectPart1.projectPartId), Eq(projectPart1.arguments), Eq(projectPart1.compilerMacros), Eq(projectPart1.includeSearchPaths))).WillOnce(Return(-1));
    EXPECT_CALL(mockSymbolStorage, fetchPrecompiledHeader(Eq(artefact.projectPartId)));
    EXPECT_CALL(mockBuildDependenciesStorage, fetchLowestLastModifiedTime(Eq(main1PathId))).WillOnce(Return(-1));
    EXPECT_CALL(mockSqliteTransactionBackend, commit());
    EXPECT_CALL(mockCollector, setFile(Eq(main1PathId), Eq(projectPart1.arguments)));
    EXPECT_CALL(mockCollector, collectSymbols());
    EXPECT_CALL(mockSqliteTransactionBackend, immediateBegin());
    EXPECT_CALL(mockSymbolStorage, addSymbolsAndSourceLocations(symbolEntries, sourceLocations));
    EXPECT_CALL(mockSymbolStorage, updateProjectPartSources(TypedEq<int>(artefact.projectPartId), Eq(sourceFileIds)));
    EXPECT_CALL(mockBuildDependenciesStorage, insertOrUpdateUsedMacros(Eq(usedMacros)));
    EXPECT_CALL(mockBuildDependenciesStorage, insertFileStatuses(Eq(fileStatus)));
    EXPECT_CALL(mockBuildDependenciesStorage, insertOrUpdateSourceDependencies(Eq(sourceDependencies)));
    EXPECT_CALL(mockSqliteTransactionBackend, commit());

    indexer.updateProjectParts({projectPart1});
}

TEST_F(SymbolIndexer, CallSetNotifier)
{
    EXPECT_CALL(mockPathWatcher, setNotifier(_));

    ClangBackEnd::SymbolIndexer indexer{indexerQueue, mockSymbolStorage, mockBuildDependenciesStorage, mockPathWatcher, filePathCache, fileStatusCache, mockSqliteTransactionBackend};
}

TEST_F(SymbolIndexer, PathChangedCallsFetchProjectPartArtefactInStorage)
{
    EXPECT_CALL(mockSymbolStorage, fetchProjectPartArtefact(sourceFileIds[0]));
    EXPECT_CALL(mockSymbolStorage, fetchProjectPartArtefact(sourceFileIds[1]));

    indexer.pathsChanged(sourceFileIds);
}

TEST_F(SymbolIndexer, UpdateChangedPathCallsInOrder)
{
    InSequence s;

    EXPECT_CALL(mockSqliteTransactionBackend, deferredBegin());
    EXPECT_CALL(mockSymbolStorage, fetchProjectPartArtefact(TypedEq<FilePathId>(sourceFileIds[0]))).WillOnce(Return(artefact));
    EXPECT_CALL(mockSymbolStorage, fetchPrecompiledHeader(Eq(artefact.projectPartId)));
    EXPECT_CALL(mockSqliteTransactionBackend, commit());
    EXPECT_CALL(mockCollector, setFile(Eq(sourceFileIds[0]), Eq(artefact.compilerArguments)));
    EXPECT_CALL(mockCollector, collectSymbols());
    EXPECT_CALL(mockSqliteTransactionBackend, immediateBegin());
    EXPECT_CALL(mockSymbolStorage, addSymbolsAndSourceLocations(symbolEntries, sourceLocations));
    EXPECT_CALL(mockSymbolStorage, updateProjectPartSources(artefact.projectPartId, Eq(sourceFileIds)));
    EXPECT_CALL(mockBuildDependenciesStorage, insertOrUpdateUsedMacros(Eq(usedMacros)));
    EXPECT_CALL(mockBuildDependenciesStorage, insertFileStatuses(Eq(fileStatus)));
    EXPECT_CALL(mockBuildDependenciesStorage, insertOrUpdateSourceDependencies(Eq(sourceDependencies)));
    EXPECT_CALL(mockSqliteTransactionBackend, commit());

    indexer.pathsChanged({sourceFileIds[0]});
}

TEST_F(SymbolIndexer, HandleEmptyOptionalArtifactInUpdateChangedPath)
{
    InSequence s;

    EXPECT_CALL(mockSqliteTransactionBackend, deferredBegin());
    EXPECT_CALL(mockSymbolStorage, fetchProjectPartArtefact(sourceFileIds[0])).WillOnce(Return(nullArtefact));
    EXPECT_CALL(mockSymbolStorage, fetchPrecompiledHeader(_)).Times(0);
    EXPECT_CALL(mockSqliteTransactionBackend, commit()).Times(0);
    EXPECT_CALL(mockCollector, setFile(_, _)).Times(0);
    EXPECT_CALL(mockCollector, collectSymbols()).Times(0);
    EXPECT_CALL(mockSqliteTransactionBackend, immediateBegin()).Times(0);
    EXPECT_CALL(mockSymbolStorage, addSymbolsAndSourceLocations(_, _)).Times(0);
    EXPECT_CALL(mockSymbolStorage, updateProjectPartSources(An<int>(), _)).Times(0);
    EXPECT_CALL(mockBuildDependenciesStorage, insertOrUpdateUsedMacros(_)).Times(0);
    EXPECT_CALL(mockBuildDependenciesStorage, insertFileStatuses(_)).Times(0);
    EXPECT_CALL(mockBuildDependenciesStorage, insertOrUpdateSourceDependencies(_)).Times(0);
    EXPECT_CALL(mockSqliteTransactionBackend, commit()).Times(0);

    indexer.pathsChanged({sourceFileIds[0]});
}

TEST_F(SymbolIndexer, UpdateChangedPathIsUsingPrecompiledHeader)
{
    ON_CALL(mockSymbolStorage, fetchProjectPartArtefact(TypedEq<FilePathId>(sourceFileIds[0])))
            .WillByDefault(Return(artefact));
    ON_CALL(mockSymbolStorage, fetchPrecompiledHeader(Eq(artefact.projectPartId)))
            .WillByDefault(Return(projectPartPch));
    std::vector<SymbolIndexerTask> symbolIndexerTask;

    EXPECT_CALL(mockCollector, setFile(Eq(sourceFileIds[0]),
                                       ElementsAre(Eq("-DFOO"),
                                                   Eq("-Xclang"),
                                                   Eq("-include-pch"),
                                                   Eq("-Xclang"),
                                                   Eq("/path/to/pch"))));

    indexer.pathsChanged({sourceFileIds[0]});
}

TEST_F(SymbolIndexer, UpdateChangedPathIsNotUsingPrecompiledHeaderIfItNotExists)
{
    ON_CALL(mockSymbolStorage, fetchProjectPartArtefact(TypedEq<FilePathId>(sourceFileIds[0])))
            .WillByDefault(Return(artefact));
    std::vector<SymbolIndexerTask> symbolIndexerTask;

    EXPECT_CALL(mockCollector, setFile(Eq(sourceFileIds[0]),
                                       ElementsAre(Eq("-DFOO"))));

    indexer.pathsChanged({sourceFileIds[0]});
}


TEST_F(SymbolIndexer, CompilerMacrosAndIncludeSearchPathsAreNotDifferent)
{
    ON_CALL(mockSymbolStorage, fetchProjectPartArtefact(An<Utils::SmallStringView>())).WillByDefault(Return(artefact));

    auto areDifferent = indexer.compilerMacrosOrIncludeSearchPathsAreDifferent(projectPart1,
                                                                               artefact);

    ASSERT_FALSE(areDifferent);
}

TEST_F(SymbolIndexer, CompilerMacrosAreDifferent)
{
    ON_CALL(mockSymbolStorage, fetchProjectPartArtefact(An<Utils::SmallStringView>())).WillByDefault(Return(artefact));

    auto areDifferent = indexer.compilerMacrosOrIncludeSearchPathsAreDifferent(projectPart2,
                                                                               artefact);

    ASSERT_TRUE(areDifferent);
}

TEST_F(SymbolIndexer, IncludeSearchPathsAreDifferent)
{
    ProjectPartContainer projectPart3{"project3",
                                      {"-I", TESTDATA_DIR, "-Wno-pragma-once-outside-header"},
                                      {{"BAR", "1"}, {"FOO", "1"}},
                                      {"/includes", "/other/includes"},
                                      {header1PathId},
                                      {main1PathId}};
    ON_CALL(mockSymbolStorage, fetchProjectPartArtefact(An<Utils::SmallStringView>())).WillByDefault(Return(artefact));

    auto areDifferent = indexer.compilerMacrosOrIncludeSearchPathsAreDifferent(projectPart3,
                                                                               artefact);

    ASSERT_TRUE(areDifferent);
}

TEST_F(SymbolIndexer, DontReparseInUpdateProjectPartsIfDefinesAreTheSame)
{
    InSequence s;
    ON_CALL(mockBuildDependenciesStorage, fetchLowestLastModifiedTime(A<FilePathId>())).WillByDefault(Return(QDateTime::currentSecsSinceEpoch()));

    EXPECT_CALL(mockSqliteTransactionBackend, immediateBegin());
    EXPECT_CALL(mockSymbolStorage, fetchProjectPartArtefact(TypedEq<Utils::SmallStringView>(projectPart1.projectPartId))).WillRepeatedly(Return(artefact));
    EXPECT_CALL(mockSymbolStorage, insertOrUpdateProjectPart(Eq(projectPart1.projectPartId), Eq(projectPart1.arguments), Eq(projectPart1.compilerMacros), Eq(projectPart1.includeSearchPaths)));
    EXPECT_CALL(mockBuildDependenciesStorage, fetchLowestLastModifiedTime(A<FilePathId>())).WillRepeatedly(Return(QDateTime::currentSecsSinceEpoch()));
    EXPECT_CALL(mockSqliteTransactionBackend, commit());
    EXPECT_CALL(mockCollector, setFile(_, _)).Times(0);
    EXPECT_CALL(mockCollector, collectSymbols()).Times(0);
    EXPECT_CALL(mockSqliteTransactionBackend, immediateBegin()).Times(0);
    EXPECT_CALL(mockSymbolStorage, addSymbolsAndSourceLocations(_, _)).Times(0);
    EXPECT_CALL(mockSymbolStorage, updateProjectPartSources(An<int>(), _)).Times(0);
    EXPECT_CALL(mockBuildDependenciesStorage, insertOrUpdateUsedMacros(_)).Times(0);
    EXPECT_CALL(mockBuildDependenciesStorage, insertFileStatuses(_)).Times(0);
    EXPECT_CALL(mockBuildDependenciesStorage, insertOrUpdateSourceDependencies(_)).Times(0);
    EXPECT_CALL(mockSqliteTransactionBackend, commit()).Times(0);

    indexer.updateProjectPart(std::move(projectPart1));
}

TEST_F(SymbolIndexer, PathsChangedUpdatesFileStatusCache)
{
    auto sourceId = filePathId(TESTDATA_DIR "/symbolindexer_pathChanged.cpp");
    auto oldLastModified = fileStatusCache.lastModifiedTime(sourceId);
    touchFile(sourceId);

    indexer.pathsChanged({sourceId});

    ASSERT_THAT(fileStatusCache.lastModifiedTime(sourceId), Gt(oldLastModified));
}

TEST_F(SymbolIndexer, GetUpdatableFilePathIdsIfCompilerMacrosAreDifferent)
{
    ON_CALL(mockSymbolStorage, fetchProjectPartArtefact(An<Utils::SmallStringView>())).WillByDefault(Return(artefact));

    auto filePathIds = indexer.updatableFilePathIds(projectPart2, artefact);

    ASSERT_THAT(filePathIds, projectPart2.sourcePathIds);
}

TEST_F(SymbolIndexer, GetUpdatableFilePathIdsIfIncludeSearchPathsAreDifferent)
{
    ON_CALL(mockSymbolStorage, fetchProjectPartArtefact(An<Utils::SmallStringView>())).WillByDefault(Return(artefact));

    auto filePathIds = indexer.updatableFilePathIds(projectPart3, artefact);

    ASSERT_THAT(filePathIds, projectPart3.sourcePathIds);
}

TEST_F(SymbolIndexer, GetNoUpdatableFilePathIdsIfArtefactsAreTheSame)
{
    ON_CALL(mockSymbolStorage, fetchProjectPartArtefact(An<Utils::SmallStringView>())).WillByDefault(Return(artefact));
    ON_CALL(mockBuildDependenciesStorage, fetchLowestLastModifiedTime(A<FilePathId>())).WillByDefault(Return(QDateTime::currentSecsSinceEpoch()));

    auto filePathIds = indexer.updatableFilePathIds(projectPart1, artefact);

    ASSERT_THAT(filePathIds, IsEmpty());
}

TEST_F(SymbolIndexer, OutdatedFilesPassUpdatableFilePathIds)
{
    indexer.pathsChanged({main1PathId});
    ON_CALL(mockSymbolStorage, fetchProjectPartArtefact(An<Utils::SmallStringView>())).WillByDefault(Return(artefact));
    ON_CALL(mockBuildDependenciesStorage, fetchLowestLastModifiedTime(A<FilePathId>()))
            .WillByDefault(Return(0));

    auto filePathIds = indexer.updatableFilePathIds(projectPart1, artefact);

    ASSERT_THAT(filePathIds, ElementsAre(main1PathId));
}

TEST_F(SymbolIndexer, UpToDateFilesDontPassFilteredUpdatableFilePathIds)
{
    indexer.pathsChanged({main1PathId});
    ON_CALL(mockSymbolStorage, fetchProjectPartArtefact(An<Utils::SmallStringView>())).WillByDefault(Return(artefact));
    ON_CALL(mockBuildDependenciesStorage, fetchLowestLastModifiedTime(A<FilePathId>()))
            .WillByDefault(Return(QDateTime::currentSecsSinceEpoch()));

    auto filePathIds = indexer.updatableFilePathIds(projectPart1, artefact);

    ASSERT_THAT(filePathIds, IsEmpty());
}

TEST_F(SymbolIndexer, OutdatedFilesAreParsedInUpdateProjectParts)
{
    indexer.pathsChanged({main1PathId});
    indexerScheduler.syncTasks();
    ON_CALL(mockSymbolStorage, fetchProjectPartArtefact(An<Utils::SmallStringView>())).WillByDefault(Return(artefact));
    ON_CALL(mockBuildDependenciesStorage, fetchLowestLastModifiedTime(A<FilePathId>()))
            .WillByDefault(Return(0));

    EXPECT_CALL(mockCollector, setFile(Eq(main1PathId), _));

    indexer.updateProjectParts({projectPart1});
}

TEST_F(SymbolIndexer, UpToDateFilesAreNotParsedInUpdateProjectParts)
{
    indexer.pathsChanged({main1PathId});
    indexerScheduler.syncTasks();
    ON_CALL(mockSymbolStorage, fetchProjectPartArtefact(An<Utils::SmallStringView>())).WillByDefault(Return(artefact));
    ON_CALL(mockBuildDependenciesStorage, fetchLowestLastModifiedTime(A<FilePathId>()))
            .WillByDefault(Return(QDateTime::currentSecsSinceEpoch()));

    EXPECT_CALL(mockCollector, setFile(_, _)).Times(0);

    indexer.updateProjectParts({projectPart1});
}

}
