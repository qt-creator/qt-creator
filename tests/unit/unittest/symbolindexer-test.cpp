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

#include "filesystem-utilities.h"
#include "googletest.h"
#include "mockbuilddependenciesstorage.h"
#include "mockclangpathwatcher.h"
#include "mockfilepathcaching.h"
#include "mockprecompiledheaderstorage.h"
#include "mockprojectpartsstorage.h"
#include "mocksqlitetransactionbackend.h"
#include "mocksymbolscollector.h"
#include "mocksymbolstorage.h"

#include <filepathcaching.h>
#include <filestatuscache.h>
#include <projectpartcontainer.h>
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

using ClangBackEnd::CompilerMacro;
using ClangBackEnd::FilePathId;
using ClangBackEnd::FilePathIds;
using ClangBackEnd::FilePathView;
using ClangBackEnd::FileStatuses;
using ClangBackEnd::ProcessorManager;
using ClangBackEnd::ProjectPartContainer;
using ClangBackEnd::ProjectPartContainers;
using ClangBackEnd::ProjectPartId;
using ClangBackEnd::ProjectPartIds;
using ClangBackEnd::SourceDependencies;
using ClangBackEnd::SourceLocationEntries;
using ClangBackEnd::SourceLocationEntry;
using ClangBackEnd::SourceLocationKind;
using ClangBackEnd::SymbolEntries;
using ClangBackEnd::SymbolEntry;
using ClangBackEnd::SymbolIndexerTask;
using ClangBackEnd::SymbolIndexerTaskQueue;
using ClangBackEnd::SymbolKind;
using ClangBackEnd::TaskScheduler;
using ClangBackEnd::UsedMacros;
using ClangBackEnd::V2::FileContainers;
using Utils::PathString;
using OptionalProjectPartArtefact = Utils::optional<ClangBackEnd::ProjectPartArtefact>;

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
        ON_CALL(mockProjectPartsStorage, fetchProjectPartArtefact(A<FilePathId>()))
            .WillByDefault(Return(artefact));
        ON_CALL(mockBuildDependenciesStorage, fetchLowestLastModifiedTime(A<FilePathId>())).WillByDefault(Return(-1));
        ON_CALL(mockCollector, collectSymbols()).WillByDefault(Return(true));

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
    ClangBackEnd::IncludeSearchPaths systemIncludeSearchPaths{
        {"/includes", 1, ClangBackEnd::IncludeSearchPathType::BuiltIn},
        {TESTDATA_DIR, 2, ClangBackEnd::IncludeSearchPathType::System},
        {"/other/includes", 3, ClangBackEnd::IncludeSearchPathType::System}};
    ClangBackEnd::IncludeSearchPaths projectIncludeSearchPaths{
        {"/project/includes", 1, ClangBackEnd::IncludeSearchPathType::User},
        {"/other/project/includes", 2, ClangBackEnd::IncludeSearchPathType::User}};
    ProjectPartContainer projectPart1{1,
                                      {"-Wno-pragma-once-outside-header"},
                                      {{"BAR", "1", 1}, {"FOO", "1", 2}},
                                      Utils::clone(systemIncludeSearchPaths),
                                      Utils::clone(projectIncludeSearchPaths),
                                      {header1PathId},
                                      {main1PathId},
                                      Utils::Language::Cxx,
                                      Utils::LanguageVersion::CXX14,
                                      Utils::LanguageExtension::None};
    ProjectPartContainer projectPart2{2,
                                      {"-Wno-pragma-once-outside-header"},
                                      {{"BAR", "1", 1}, {"FOO", "0", 2}},
                                      Utils::clone(systemIncludeSearchPaths),
                                      Utils::clone(projectIncludeSearchPaths),
                                      {header2PathId},
                                      {main2PathId},
                                      Utils::Language::Cxx,
                                      Utils::LanguageVersion::CXX14,
                                      Utils::LanguageExtension::None};
    ProjectPartContainer projectPart3{3,
                                      {"-Wno-pragma-once-outside-header"},
                                      {{"BAR", "1", 1}, {"FOO", "1", 2}},
                                      Utils::clone(systemIncludeSearchPaths),
                                      Utils::clone(projectIncludeSearchPaths),
                                      {header1PathId},
                                      {main1PathId},
                                      Utils::Language::Cxx,
                                      Utils::LanguageVersion::CXX14,
                                      Utils::LanguageExtension::None};
    FileContainers unsaved{{{TESTDATA_DIR, "query_simplefunction.h"},
                            "void f();",
                            {}}};
    SymbolEntries symbolEntries{{1, {"function", "function", SymbolKind::Function}}};
    SourceLocationEntries sourceLocations{{1, 1, {42, 23}, SourceLocationKind::Declaration}};
    FilePathIds sourceFileIds{1, 23};
    UsedMacros usedMacros{{"Foo", 1}};
    FileStatuses fileStatus{{2, 3, 4}};
    SourceDependencies sourceDependencies{{1, 2}, {1, 3}};
    Utils::SmallString systemIncludeSearchPathsText{
         R"([["/includes", 1, 2], [")" TESTDATA_DIR R"(" ,2 , 3], ["/other/includes", 3, 3]])"};
    Utils::SmallString projectIncludeSearchPathsText{
        R"([["/project/includes", 1, 1], ["/other/project/includes", 2, 1]])"};
    ClangBackEnd::ProjectPartArtefact artefact{R"(["-DFOO"])",
                                               R"([["FOO","1", 2],["BAR","1", 1]])",
                                               systemIncludeSearchPathsText,
                                               projectIncludeSearchPathsText,
                                               74,
                                               Utils::Language::Cxx,
                                               Utils::LanguageVersion::CXX14,
                                               Utils::LanguageExtension::None};
    ClangBackEnd::ProjectPartArtefact emptyArtefact{"",
                                                    "",
                                                    "",
                                                    "",
                                                    1,
                                                    Utils::Language::Cxx,
                                                    Utils::LanguageVersion::CXX14,
                                                    Utils::LanguageExtension::None};
    Utils::optional<ClangBackEnd::ProjectPartArtefact > nullArtefact;
    ClangBackEnd::ProjectPartPch projectPartPch{74, "/path/to/pch", 4};
    NiceMock<MockSqliteTransactionBackend> mockSqliteTransactionBackend;
    NiceMock<MockSymbolStorage> mockSymbolStorage;
    NiceMock<MockBuildDependenciesStorage> mockBuildDependenciesStorage;
    NiceMock<MockPrecompiledHeaderStorage> mockPrecompiledHeaderStorage;
    NiceMock<MockProjectPartsStorage> mockProjectPartsStorage;
    NiceMock<MockClangPathWatcher> mockPathWatcher;
    ClangBackEnd::FileStatusCache fileStatusCache{filePathCache};
    ClangBackEnd::GeneratedFiles generatedFiles;
    Manager collectorManger{generatedFiles};
    NiceMock<MockFunction<void(int, int)>> mockSetProgressCallback;
    ClangBackEnd::ProgressCounter progressCounter{mockSetProgressCallback.AsStdFunction()};
    ClangBackEnd::SymbolIndexer indexer{indexerQueue,
                                        mockSymbolStorage,
                                        mockBuildDependenciesStorage,
                                        mockPrecompiledHeaderStorage,
                                        mockPathWatcher,
                                        filePathCache,
                                        fileStatusCache,
                                        mockSqliteTransactionBackend,
                                        mockProjectPartsStorage};
    SymbolIndexerTaskQueue indexerQueue{indexerScheduler, progressCounter};
    Scheduler indexerScheduler{collectorManger,
                               indexerQueue,
                               progressCounter,
                               1,
                               ClangBackEnd::CallDoInMainThreadAfterFinished::Yes};
    MockSymbolsCollector &mockCollector{static_cast<MockSymbolsCollector&>(collectorManger.unusedProcessor())};
};

std::unique_ptr<Data> SymbolIndexer::data;

TEST_F(SymbolIndexer, UpdateProjectPartsCallsAddFilesInCollector)
{
    EXPECT_CALL(mockCollector,
                setFile(main1PathId,
                        ElementsAre("clang++",
                                    "-w",
                                    "-Wno-pragma-once-outside-header",
                                    "-DNOMINMAX",
                                    "-x",
                                    "c++",
                                    "-std=c++14",
                                    "-nostdinc",
                                    "-nostdinc++",
                                    "-DBAR=1",
                                    "-DFOO=1",
                                    "-I",
                                    toNativePath("/project/includes"),
                                    "-I",
                                    toNativePath("/other/project/includes"),
                                    "-isystem",
                                    toNativePath(TESTDATA_DIR),
                                    "-isystem",
                                    toNativePath("/other/includes"),
                                    "-isystem",
                                    toNativePath("/includes"))));

    indexer.updateProjectParts({projectPart1});
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsAddFilesWithPrecompiledHeaderInCollector)
{
    ON_CALL(mockPrecompiledHeaderStorage, fetchPrecompiledHeader(Eq(projectPart1.projectPartId)))
        .WillByDefault(Return(projectPartPch));

    EXPECT_CALL(mockCollector,
                setFile(main1PathId,
                        ElementsAre("clang++",
                                    "-w",
                                    "-Wno-pragma-once-outside-header",
                                    "-DNOMINMAX",
                                    "-x",
                                    "c++",
                                    "-std=c++14",
                                    "-nostdinc",
                                    "-nostdinc++",
                                    "-DBAR=1",
                                    "-DFOO=1",
                                    "-I",
                                    toNativePath("/project/includes"),
                                    "-I",
                                    toNativePath("/other/project/includes"),
                                    "-isystem",
                                    toNativePath(TESTDATA_DIR),
                                    "-isystem",
                                    toNativePath("/other/includes"),
                                    "-isystem",
                                    toNativePath("/includes"),
                                    "-Xclang",
                                    "-include-pch",
                                    "-Xclang",
                                    toNativePath("/path/to/pch"))));

    indexer.updateProjectParts({projectPart1});
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsAddFilesWithoutPrecompiledHeaderInCollector)
{
    ON_CALL(mockProjectPartsStorage, fetchProjectPartArtefact(TypedEq<ProjectPartId>(1)))
        .WillByDefault(Return(emptyArtefact));

    EXPECT_CALL(mockCollector,
                setFile(main1PathId,
                        ElementsAre("clang++",
                                    "-w",
                                    "-Wno-pragma-once-outside-header",
                                    "-DNOMINMAX",
                                    "-x",
                                    "c++",
                                    "-std=c++14",
                                    "-nostdinc",
                                    "-nostdinc++",
                                    "-DBAR=1",
                                    "-DFOO=1",
                                    "-I",
                                    toNativePath("/project/includes"),
                                    "-I",
                                    toNativePath("/other/project/includes"),
                                    "-isystem",
                                    toNativePath(TESTDATA_DIR),
                                    "-isystem",
                                    toNativePath("/other/includes"),
                                    "-isystem",
                                    toNativePath("/includes"))));

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

TEST_F(SymbolIndexer, UpdateProjectPartsCallsInOrder)
{
    InSequence s;

    EXPECT_CALL(mockSqliteTransactionBackend, deferredBegin());
    EXPECT_CALL(mockPrecompiledHeaderStorage, fetchPrecompiledHeader(Eq(projectPart1.projectPartId)));
    EXPECT_CALL(mockSqliteTransactionBackend, commit());
    EXPECT_CALL(mockCollector,
                setFile(main1PathId,
                        ElementsAre("clang++",
                                    "-w",
                                    "-Wno-pragma-once-outside-header",
                                    "-DNOMINMAX",
                                    "-x",
                                    "c++",
                                    "-std=c++14",
                                    "-nostdinc",
                                    "-nostdinc++",
                                    "-DBAR=1",
                                    "-DFOO=1",
                                    "-I",
                                    toNativePath("/project/includes"),
                                    "-I",
                                    toNativePath("/other/project/includes"),
                                    "-isystem",
                                    toNativePath(TESTDATA_DIR),
                                    "-isystem",
                                    toNativePath("/other/includes"),
                                    "-isystem",
                                    toNativePath("/includes"))));
    EXPECT_CALL(mockCollector, collectSymbols());
    EXPECT_CALL(mockSqliteTransactionBackend, immediateBegin());
    EXPECT_CALL(mockSymbolStorage, addSymbolsAndSourceLocations(symbolEntries, sourceLocations));
    EXPECT_CALL(mockSqliteTransactionBackend, commit());

    indexer.updateProjectParts({projectPart1});
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsInOrderButGetsAnErrorForCollectingSymbols)
{
    InSequence s;

    EXPECT_CALL(mockSqliteTransactionBackend, deferredBegin());
    EXPECT_CALL(mockPrecompiledHeaderStorage, fetchPrecompiledHeader(Eq(projectPart1.projectPartId)));
    EXPECT_CALL(mockSqliteTransactionBackend, commit());
    EXPECT_CALL(mockCollector,
                setFile(main1PathId,
                        ElementsAre("clang++",
                                    "-w",
                                    "-Wno-pragma-once-outside-header",
                                    "-DNOMINMAX",
                                    "-x",
                                    "c++",
                                    "-std=c++14",
                                    "-nostdinc",
                                    "-nostdinc++",
                                    "-DBAR=1",
                                    "-DFOO=1",
                                    "-I",
                                    toNativePath("/project/includes"),
                                    "-I",
                                    toNativePath("/other/project/includes"),
                                    "-isystem",
                                    toNativePath(TESTDATA_DIR),
                                    "-isystem",
                                    toNativePath("/other/includes"),
                                    "-isystem",
                                    toNativePath("/includes"))));
    EXPECT_CALL(mockCollector, collectSymbols()).WillOnce(Return(false));
    EXPECT_CALL(mockSqliteTransactionBackend, immediateBegin()).Times(0);
    EXPECT_CALL(mockSymbolStorage, addSymbolsAndSourceLocations(symbolEntries, sourceLocations)).Times(0);
    EXPECT_CALL(mockSqliteTransactionBackend, commit()).Times(0);

    indexer.updateProjectParts({projectPart1});
}

TEST_F(SymbolIndexer, CallSetNotifier)
{
    EXPECT_CALL(mockPathWatcher, setNotifier(_));

    ClangBackEnd::SymbolIndexer indexer{indexerQueue,
                                        mockSymbolStorage,
                                        mockBuildDependenciesStorage,
                                        mockPrecompiledHeaderStorage,
                                        mockPathWatcher,
                                        filePathCache,
                                        fileStatusCache,
                                        mockSqliteTransactionBackend,
                                        mockProjectPartsStorage};
}

TEST_F(SymbolIndexer, PathChangedCallsFetchProjectPartArtefactInStorage)
{
    EXPECT_CALL(mockProjectPartsStorage, fetchProjectPartArtefact(sourceFileIds[0]));
    EXPECT_CALL(mockProjectPartsStorage, fetchProjectPartArtefact(sourceFileIds[1]));

    indexer.pathsChanged(sourceFileIds);
}

TEST_F(SymbolIndexer, UpdateChangedPathCallsInOrder)
{
    InSequence s;

    EXPECT_CALL(mockSqliteTransactionBackend, deferredBegin());
    EXPECT_CALL(mockProjectPartsStorage,
                fetchProjectPartArtefact(TypedEq<FilePathId>(sourceFileIds[0])))
        .WillOnce(Return(artefact));
    EXPECT_CALL(mockPrecompiledHeaderStorage, fetchPrecompiledHeader(Eq(artefact.projectPartId)));
    EXPECT_CALL(mockSqliteTransactionBackend, commit());
    EXPECT_CALL(mockCollector,
                setFile(Eq(sourceFileIds[0]),
                        ElementsAre("clang++",
                                    "-w",
                                    "-DFOO",
                                    "-DNOMINMAX",
                                    "-x",
                                    "c++",
                                    "-std=c++14",
                                    "-nostdinc",
                                    "-nostdinc++",
                                    "-DBAR=1",
                                    "-DFOO=1",
                                    "-I",
                                    toNativePath("/project/includes"),
                                    "-I",
                                    toNativePath("/other/project/includes"),
                                    "-isystem",
                                    toNativePath(TESTDATA_DIR),
                                    "-isystem",
                                    toNativePath("/other/includes"),
                                    "-isystem",
                                    toNativePath("/includes"))));
    EXPECT_CALL(mockCollector, collectSymbols());
    EXPECT_CALL(mockSqliteTransactionBackend, immediateBegin());
    EXPECT_CALL(mockSymbolStorage, addSymbolsAndSourceLocations(symbolEntries, sourceLocations));
    EXPECT_CALL(mockSqliteTransactionBackend, commit());

    indexer.pathsChanged({sourceFileIds[0]});
}

TEST_F(SymbolIndexer, HandleEmptyOptionalArtifactInUpdateChangedPath)
{
    InSequence s;

    EXPECT_CALL(mockSqliteTransactionBackend, deferredBegin());
    EXPECT_CALL(mockProjectPartsStorage, fetchProjectPartArtefact(sourceFileIds[0]))
        .WillOnce(Return(nullArtefact));
    EXPECT_CALL(mockPrecompiledHeaderStorage, fetchPrecompiledHeader(_)).Times(0);
    EXPECT_CALL(mockSqliteTransactionBackend, commit()).Times(0);
    EXPECT_CALL(mockCollector, setFile(_, _)).Times(0);
    EXPECT_CALL(mockCollector, collectSymbols()).Times(0);
    EXPECT_CALL(mockSqliteTransactionBackend, immediateBegin()).Times(0);
    EXPECT_CALL(mockSymbolStorage, addSymbolsAndSourceLocations(_, _)).Times(0);
    EXPECT_CALL(mockSqliteTransactionBackend, commit()).Times(0);

    indexer.pathsChanged({sourceFileIds[0]});
}

TEST_F(SymbolIndexer, UpdateChangedPathCallsInOrderButGetsAnErrorForCollectingSymbols)
{
    InSequence s;

    EXPECT_CALL(mockSqliteTransactionBackend, deferredBegin());
    EXPECT_CALL(mockProjectPartsStorage,
                fetchProjectPartArtefact(TypedEq<FilePathId>(sourceFileIds[0])))
        .WillOnce(Return(artefact));
    EXPECT_CALL(mockPrecompiledHeaderStorage, fetchPrecompiledHeader(Eq(artefact.projectPartId)));
    EXPECT_CALL(mockSqliteTransactionBackend, commit());
    EXPECT_CALL(mockCollector,
                setFile(Eq(sourceFileIds[0]),
                        ElementsAre("clang++",
                                    "-w",
                                    "-DFOO",
                                    "-DNOMINMAX",
                                    "-x",
                                    "c++",
                                    "-std=c++14",
                                    "-nostdinc",
                                    "-nostdinc++",
                                    "-DBAR=1",
                                    "-DFOO=1",
                                    "-I",
                                    toNativePath("/project/includes"),
                                    "-I",
                                    toNativePath("/other/project/includes"),
                                    "-isystem",
                                    toNativePath(TESTDATA_DIR),
                                    "-isystem",
                                    toNativePath("/other/includes"),
                                    "-isystem",
                                    toNativePath("/includes"))));
    EXPECT_CALL(mockCollector, collectSymbols()).WillOnce(Return(false));
    EXPECT_CALL(mockSqliteTransactionBackend, immediateBegin()).Times(0);
    EXPECT_CALL(mockSymbolStorage, addSymbolsAndSourceLocations(symbolEntries, sourceLocations)).Times(0);
    EXPECT_CALL(mockSqliteTransactionBackend, commit()).Times(0);

    indexer.pathsChanged({sourceFileIds[0]});
}

TEST_F(SymbolIndexer, UpdateChangedPathIsUsingPrecompiledHeader)
{
    ON_CALL(mockProjectPartsStorage, fetchProjectPartArtefact(TypedEq<FilePathId>(sourceFileIds[0])))
        .WillByDefault(Return(artefact));
    ON_CALL(mockPrecompiledHeaderStorage, fetchPrecompiledHeader(Eq(artefact.projectPartId)))
        .WillByDefault(Return(projectPartPch));
    std::vector<SymbolIndexerTask> symbolIndexerTask;

    EXPECT_CALL(mockCollector,
                setFile(Eq(sourceFileIds[0]),
                        ElementsAre("clang++",
                                    "-w",
                                    "-DFOO",
                                    "-DNOMINMAX",
                                    "-x",
                                    "c++",
                                    "-std=c++14",
                                    "-nostdinc",
                                    "-nostdinc++",
                                    "-DBAR=1",
                                    "-DFOO=1",
                                    "-I",
                                    toNativePath("/project/includes"),
                                    "-I",
                                    toNativePath("/other/project/includes"),
                                    "-isystem",
                                    toNativePath(TESTDATA_DIR),
                                    "-isystem",
                                    toNativePath("/other/includes"),
                                    "-isystem",
                                    toNativePath("/includes"),
                                    "-Xclang",
                                    "-include-pch",
                                    "-Xclang",
                                    toNativePath("/path/to/pch"))));

    indexer.pathsChanged({sourceFileIds[0]});
}

TEST_F(SymbolIndexer, UpdateChangedPathIsNotUsingPrecompiledHeaderIfItNotExists)
{
    ON_CALL(mockProjectPartsStorage, fetchProjectPartArtefact(TypedEq<FilePathId>(sourceFileIds[0])))
        .WillByDefault(Return(artefact));
    std::vector<SymbolIndexerTask> symbolIndexerTask;

    EXPECT_CALL(mockCollector,
                setFile(Eq(sourceFileIds[0]),
                        ElementsAre("clang++",
                                    "-w",
                                    "-DFOO",
                                    "-DNOMINMAX",
                                    "-x",
                                    "c++",
                                    "-std=c++14",
                                    "-nostdinc",
                                    "-nostdinc++",
                                    "-DBAR=1",
                                    "-DFOO=1",
                                    "-I",
                                    toNativePath("/project/includes"),
                                    "-I",
                                    toNativePath("/other/project/includes"),
                                    "-isystem",
                                    toNativePath(TESTDATA_DIR),
                                    "-isystem",
                                    toNativePath("/other/includes"),
                                    "-isystem",
                                    toNativePath("/includes"))));

    indexer.pathsChanged({sourceFileIds[0]});
}


TEST_F(SymbolIndexer, CompilerMacrosAndIncludeSearchPathsAreNotDifferent)
{
    ON_CALL(mockProjectPartsStorage, fetchProjectPartArtefact(A<ProjectPartId>()))
        .WillByDefault(Return(artefact));

    auto areDifferent = indexer.compilerMacrosOrIncludeSearchPathsAreDifferent(projectPart1,
                                                                               artefact);

    ASSERT_FALSE(areDifferent);
}

TEST_F(SymbolIndexer, CompilerMacrosAreDifferent)
{
    ON_CALL(mockProjectPartsStorage, fetchProjectPartArtefact(A<ProjectPartId>()))
        .WillByDefault(Return(artefact));

    auto areDifferent = indexer.compilerMacrosOrIncludeSearchPathsAreDifferent(projectPart2,
                                                                               artefact);

    ASSERT_TRUE(areDifferent);
}

TEST_F(SymbolIndexer, SystemIncludeSearchPathsAreDifferent)
{
    ClangBackEnd::IncludeSearchPaths newSystemIncludeSearchPaths{
        {"/includes", 1, ClangBackEnd::IncludeSearchPathType::BuiltIn},
        {"/other/includes2", 2, ClangBackEnd::IncludeSearchPathType::System}};
    ClangBackEnd::IncludeSearchPaths newProjectIncludeSearchPaths{
        {"/project/includes", 1, ClangBackEnd::IncludeSearchPathType::User},
        {"/other/project/includes2", 2, ClangBackEnd::IncludeSearchPathType::User}};
    ProjectPartContainer projectPart3{3,
                                      {"-I", TESTDATA_DIR, "-Wno-pragma-once-outside-header"},
                                      {{"BAR", "1", 1}, {"FOO", "1", 2}},
                                      {{"/includes", 1, ClangBackEnd::IncludeSearchPathType::BuiltIn},
                                       {"/other/includes2",
                                        2,
                                        ClangBackEnd::IncludeSearchPathType::System}},
                                      Utils::clone(projectIncludeSearchPaths),
                                      {header1PathId},
                                      {main1PathId},
                                      Utils::Language::C,
                                      Utils::LanguageVersion::C11,
                                      Utils::LanguageExtension::All};
    ON_CALL(mockProjectPartsStorage, fetchProjectPartArtefact(A<ProjectPartId>()))
        .WillByDefault(Return(artefact));

    auto areDifferent = indexer.compilerMacrosOrIncludeSearchPathsAreDifferent(
        projectPart3, artefact);

    ASSERT_TRUE(areDifferent);
}

TEST_F(SymbolIndexer, ProjectIncludeSearchPathsAreDifferent)
{
    ProjectPartContainer projectPart3{
        3,
        {"-I", TESTDATA_DIR, "-Wno-pragma-once-outside-header"},
        {{"BAR", "1", 1}, {"FOO", "1", 2}},
        Utils::clone(systemIncludeSearchPaths),
        {{"/project/includes", 1, ClangBackEnd::IncludeSearchPathType::User},
         {"/other/project/includes2", 2, ClangBackEnd::IncludeSearchPathType::User}},
        {header1PathId},
        {main1PathId},
        Utils::Language::C,
        Utils::LanguageVersion::C11,
        Utils::LanguageExtension::All};
    ON_CALL(mockProjectPartsStorage, fetchProjectPartArtefact(A<ProjectPartId>()))
        .WillByDefault(Return(artefact));

    auto areDifferent = indexer.compilerMacrosOrIncludeSearchPathsAreDifferent(
        projectPart3, artefact);

    ASSERT_TRUE(areDifferent);
}

TEST_F(SymbolIndexer, DISABLED_DontReparseInUpdateProjectPartsIfDefinesAreTheSame)
{
    InSequence s;
    ON_CALL(mockBuildDependenciesStorage, fetchLowestLastModifiedTime(A<FilePathId>())).WillByDefault(Return(QDateTime::currentSecsSinceEpoch()));

    EXPECT_CALL(mockSqliteTransactionBackend, immediateBegin());
    EXPECT_CALL(mockProjectPartsStorage, fetchProjectPartArtefact(TypedEq<ProjectPartId>(1)))
        .WillRepeatedly(Return(artefact));
    EXPECT_CALL(mockProjectPartsStorage,
                updateProjectPart(Eq(projectPart1.projectPartId),
                                  Eq(projectPart1.toolChainArguments),
                                  Eq(projectPart1.compilerMacros),
                                  Eq(projectPart1.systemIncludeSearchPaths),
                                  Eq(projectPart1.projectIncludeSearchPaths),
                                  Eq(Utils::Language::Cxx),
                                  Eq(Utils::LanguageVersion::CXX14),
                                  Eq(Utils::LanguageExtension::None)));
    EXPECT_CALL(mockBuildDependenciesStorage, fetchLowestLastModifiedTime(A<FilePathId>())).WillRepeatedly(Return(QDateTime::currentSecsSinceEpoch()));
    EXPECT_CALL(mockSqliteTransactionBackend, commit());
    EXPECT_CALL(mockCollector, setFile(_, _)).Times(0);
    EXPECT_CALL(mockCollector, collectSymbols()).Times(0);
    EXPECT_CALL(mockSqliteTransactionBackend, immediateBegin()).Times(0);
    EXPECT_CALL(mockSymbolStorage, addSymbolsAndSourceLocations(_, _)).Times(0);
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
    ON_CALL(mockProjectPartsStorage, fetchProjectPartArtefact(A<ProjectPartId>()))
        .WillByDefault(Return(artefact));

    auto filePathIds = indexer.updatableFilePathIds(projectPart2, artefact);

    ASSERT_THAT(filePathIds, projectPart2.sourcePathIds);
}

TEST_F(SymbolIndexer, GetUpdatableFilePathIdsIfIncludeSearchPathsAreDifferent)
{
    ON_CALL(mockProjectPartsStorage, fetchProjectPartArtefact(A<ProjectPartId>()))
        .WillByDefault(Return(artefact));

    auto filePathIds = indexer.updatableFilePathIds(projectPart3, artefact);

    ASSERT_THAT(filePathIds, projectPart3.sourcePathIds);
}

TEST_F(SymbolIndexer, GetNoUpdatableFilePathIdsIfArtefactsAreTheSame)
{
    ON_CALL(mockProjectPartsStorage, fetchProjectPartArtefact(A<ProjectPartId>()))
        .WillByDefault(Return(artefact));
    ON_CALL(mockBuildDependenciesStorage, fetchLowestLastModifiedTime(A<FilePathId>())).WillByDefault(Return(QDateTime::currentSecsSinceEpoch()));

    auto filePathIds = indexer.updatableFilePathIds(projectPart1, artefact);

    ASSERT_THAT(filePathIds, IsEmpty());
}

TEST_F(SymbolIndexer, OutdatedFilesPassUpdatableFilePathIds)
{
    indexer.pathsChanged({main1PathId});
    ON_CALL(mockProjectPartsStorage, fetchProjectPartArtefact(A<ProjectPartId>()))
        .WillByDefault(Return(artefact));
    ON_CALL(mockBuildDependenciesStorage, fetchLowestLastModifiedTime(A<FilePathId>()))
            .WillByDefault(Return(0));

    auto filePathIds = indexer.updatableFilePathIds(projectPart1, artefact);

    ASSERT_THAT(filePathIds, ElementsAre(main1PathId));
}

TEST_F(SymbolIndexer, UpToDateFilesDontPassFilteredUpdatableFilePathIds)
{
    indexer.pathsChanged({main1PathId});
    ON_CALL(mockProjectPartsStorage, fetchProjectPartArtefact(A<ProjectPartId>()))
        .WillByDefault(Return(artefact));
    ON_CALL(mockBuildDependenciesStorage, fetchLowestLastModifiedTime(A<FilePathId>()))
            .WillByDefault(Return(QDateTime::currentSecsSinceEpoch()));

    auto filePathIds = indexer.updatableFilePathIds(projectPart1, artefact);

    ASSERT_THAT(filePathIds, IsEmpty());
}

TEST_F(SymbolIndexer, OutdatedFilesAreParsedInUpdateProjectParts)
{
    indexer.pathsChanged({main1PathId});
    indexerScheduler.syncTasks();
    ON_CALL(mockProjectPartsStorage, fetchProjectPartArtefact(A<ProjectPartId>()))
        .WillByDefault(Return(artefact));
    ON_CALL(mockBuildDependenciesStorage, fetchLowestLastModifiedTime(A<FilePathId>()))
            .WillByDefault(Return(0));

    EXPECT_CALL(mockCollector, setFile(Eq(main1PathId), _));

    indexer.updateProjectParts({projectPart1});
}

TEST_F(SymbolIndexer, DISABLED_UpToDateFilesAreNotParsedInUpdateProjectParts)
{
    indexer.pathsChanged({main1PathId});

    indexerScheduler.syncTasks();
    ON_CALL(mockProjectPartsStorage, fetchProjectPartArtefact(A<ProjectPartId>()))
        .WillByDefault(Return(artefact));
    ON_CALL(mockBuildDependenciesStorage, fetchLowestLastModifiedTime(A<FilePathId>()))
            .WillByDefault(Return(QDateTime::currentSecsSinceEpoch()));

    EXPECT_CALL(mockCollector, setFile(_, _)).Times(0);

    indexer.updateProjectParts({projectPart1});
}

}
