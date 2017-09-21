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
#include "mocksymbolscollector.h"
#include "mocksymbolstorage.h"

#include <symbolindexer.h>
#include <updatepchprojectpartsmessage.h>
#include <projectpartcontainerv2.h>

namespace {

using testing::_;
using testing::Contains;
using testing::Field;
using testing::IsEmpty;
using testing::NiceMock;
using testing::Property;
using testing::Return;
using testing::ReturnRef;
using testing::Sequence;

using Utils::PathString;
using ClangBackEnd::V2::ProjectPartContainer;
using ClangBackEnd::V2::ProjectPartContainers;
using ClangBackEnd::V2::FileContainers;
using ClangBackEnd::SymbolEntries;
using ClangBackEnd::SymbolEntry;
using ClangBackEnd::SourceLocationEntries;
using ClangBackEnd::SourceLocationEntry;
using ClangBackEnd::SymbolType;

class SymbolIndexer : public testing::Test
{
protected:
    void SetUp();

protected:
    PathString main1Path = TESTDATA_DIR "/includecollector_main3.cpp";
    PathString main2Path = TESTDATA_DIR "/includecollector_main2.cpp";
    PathString header1Path = TESTDATA_DIR "/includecollector_header1.h";
    PathString header2Path = TESTDATA_DIR "/includecollector_header2.h";
    PathString generatedFileName = "includecollector_generated_file.h";
    PathString generatedFilePath = TESTDATA_DIR "/includecollector_generated_file.h";
    ProjectPartContainer projectPart1{"project1",
                                      {"-I", TESTDATA_DIR, "-Wno-pragma-once-outside-header"},
                                      {header1Path.clone()},
                                      {main1Path.clone()}};
    ProjectPartContainer projectPart2{"project2",
                                      {"-I", TESTDATA_DIR, "-x", "c++-header", "-Wno-pragma-once-outside-header"},
                                      {header2Path.clone()},
                                      {main2Path.clone()}};
    FileContainers unsaved{{{TESTDATA_DIR, "query_simplefunction.h"},
                            "void f();",
                            {}}};
    SymbolEntries symbolEntries{{1, {"function", "function"}}};
    SourceLocationEntries sourceLocations{{1, {1, 1}, {42, 23}, SymbolType::Declaration}};
    NiceMock<MockSymbolsCollector> mockCollector;
    NiceMock<MockSymbolStorage> mockStorage;
    ClangBackEnd::SymbolIndexer indexer{mockCollector, mockStorage};
};

TEST_F(SymbolIndexer, UpdateProjectPartsCallsAddFilesInCollector)
{
    EXPECT_CALL(mockCollector, addFiles(projectPart1.sourcePaths(), projectPart1.arguments()));

    indexer.updateProjectParts({projectPart1}, Utils::clone(unsaved));
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsAddFilesInCollectorForEveryProjectPart)
{
    EXPECT_CALL(mockCollector, addFiles(_, _))
            .Times(2);

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
    EXPECT_CALL(mockCollector, collectSymbols());

    indexer.updateProjectParts({projectPart1}, Utils::clone(unsaved));
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsSymbolsInCollector)
{
    EXPECT_CALL(mockCollector, symbols());

    indexer.updateProjectParts({projectPart1}, Utils::clone(unsaved));
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsSourceLocationsInCollector)
{
    EXPECT_CALL(mockCollector, sourceLocations());

    indexer.updateProjectParts({projectPart1}, Utils::clone(unsaved));
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsAddUnsavedFilesInCollector)
{
    EXPECT_CALL(mockCollector, addUnsavedFiles(unsaved));

    indexer.updateProjectParts({projectPart1}, Utils::clone(unsaved));
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsAddSymbolsAndSourceLocationsInStorage)
{
    EXPECT_CALL(mockStorage, addSymbolsAndSourceLocations(symbolEntries, sourceLocations));

    indexer.updateProjectParts({projectPart1}, Utils::clone(unsaved));
}

TEST_F(SymbolIndexer, UpdateProjectPartsCallsInOrder)
{
    EXPECT_CALL(mockCollector, addFiles(_, _));
    EXPECT_CALL(mockCollector, addUnsavedFiles(unsaved));
    EXPECT_CALL(mockCollector, collectSymbols());
    EXPECT_CALL(mockCollector, symbols());
    EXPECT_CALL(mockCollector, sourceLocations());
    EXPECT_CALL(mockStorage, addSymbolsAndSourceLocations(symbolEntries, sourceLocations));

    indexer.updateProjectParts({projectPart1}, Utils::clone(unsaved));
}

void SymbolIndexer::SetUp()
{
    ON_CALL(mockCollector, symbols()).WillByDefault(ReturnRef(symbolEntries));
    ON_CALL(mockCollector, sourceLocations()).WillByDefault(ReturnRef(sourceLocations));
}

}

