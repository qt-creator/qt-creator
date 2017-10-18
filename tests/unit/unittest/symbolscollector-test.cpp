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

#include "filesystem-utilities.h"

#include <symbolscollector.h>

#include <filepathcaching.h>
#include <refactoringdatabaseinitializer.h>

#include <QDir>

using testing::PrintToString;
using testing::AllOf;
using testing::Contains;
using testing::Not;
using testing::Field;
using testing::Key;
using testing::Pair;
using testing::Value;
using testing::_;

using ClangBackEnd::FilePathId;
using ClangBackEnd::FilePathCaching;
using ClangBackEnd::V2::FileContainers;
using ClangBackEnd::SourceLocationEntry;
using ClangBackEnd::SymbolEntry;
using ClangBackEnd::SymbolType;
using ClangBackEnd::SymbolIndex;

using Sqlite::Database;

namespace {

class SymbolsCollector : public testing::Test
{
protected:
    FilePathId filePathId(Utils::SmallStringView string)
    {
        return filePathCache.filePathId(string);
    }

    SymbolIndex symbolIdForSymbolName(const Utils::SmallString &symbolName);

protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> initializer{database};
    FilePathCaching filePathCache{database};
    ClangBackEnd::SymbolsCollector collector{filePathCache};
};

TEST_F(SymbolsCollector, CollectSymbolName)
{
    collector.addFile(TESTDATA_DIR, "symbolscollector_simple.cpp", "", {"cc", TESTDATA_DIR"/symbolscollector_simple.cpp"});

    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(),
                Contains(
                    Pair(_, Field(&SymbolEntry::symbolName, "x"))));
}

TEST_F(SymbolsCollector, SymbolMatchesLocation)
{
    collector.addFile(TESTDATA_DIR, "symbolscollector_simple.cpp", "", {"cc", TESTDATA_DIR"/symbolscollector_simple.cpp"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(
                    AllOf(Field(&SourceLocationEntry::symbolId, symbolIdForSymbolName("function")),
                          Field(&SourceLocationEntry::line, 1),
                          Field(&SourceLocationEntry::column, 6))));
}

TEST_F(SymbolsCollector, OtherSymboldMatchesLocation)
{
    collector.addFile(TESTDATA_DIR, "symbolscollector_simple.cpp", "", {"cc", TESTDATA_DIR"/symbolscollector_simple.cpp"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(
                    AllOf(Field(&SourceLocationEntry::symbolId, symbolIdForSymbolName("function")),
                          Field(&SourceLocationEntry::line, 2),
                          Field(&SourceLocationEntry::column, 6))));
}

TEST_F(SymbolsCollector, CollectFilePath)
{
    collector.addFile(TESTDATA_DIR, "symbolscollector_simple.cpp", "", {"cc", TESTDATA_DIR"/symbolscollector_simple.cpp"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(
                    AllOf(Field(&SourceLocationEntry::filePathId,
                                filePathId(TESTDATA_DIR"/symbolscollector_simple.cpp")),
                          Field(&SourceLocationEntry::symbolType, SymbolType::Declaration))));
}

TEST_F(SymbolsCollector, CollectLineColumn)
{
    collector.addFile(TESTDATA_DIR, "symbolscollector_simple.cpp", "", {"cc", TESTDATA_DIR"/symbolscollector_simple.cpp"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(
                    AllOf(Field(&SourceLocationEntry::line, 1),
                          Field(&SourceLocationEntry::column, 6),
                          Field(&SourceLocationEntry::symbolType, SymbolType::Declaration))));
}

TEST_F(SymbolsCollector, CollectReference)
{
    collector.addFile(TESTDATA_DIR, "symbolscollector_simple.cpp", "", {"cc", TESTDATA_DIR"/symbolscollector_simple.cpp"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(
                    AllOf(Field(&SourceLocationEntry::line, 14),
                          Field(&SourceLocationEntry::column, 5),
                          Field(&SourceLocationEntry::symbolType, SymbolType::DeclarationReference))));
}

TEST_F(SymbolsCollector, ReferencedSymboldMatchesLocation)
{
    collector.addFile(TESTDATA_DIR, "symbolscollector_simple.cpp", "", {"cc", TESTDATA_DIR"/symbolscollector_simple.cpp"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(
                    AllOf(Field(&SourceLocationEntry::symbolId, symbolIdForSymbolName("function")),
                          Field(&SourceLocationEntry::line, 14),
                          Field(&SourceLocationEntry::column, 5))));
}

TEST_F(SymbolsCollector, DISABLED_ON_WINDOWS(CollectInUnsavedFile))
{
    FileContainers unsaved{{{TESTDATA_DIR, "symbolscollector_generated_file.h"},
                            "void function();",
                            {}}};
    collector.addFile(TESTDATA_DIR, "symbolscollector_unsaved.cpp", "", {"cc", TESTDATA_DIR"/symbolscollector_unsaved.cpp"});
    collector.addUnsavedFiles(std::move(unsaved));

    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(),
                Contains(
                    Pair(_, Field(&SymbolEntry::symbolName, "function"))));
}

SymbolIndex SymbolsCollector::symbolIdForSymbolName(const Utils::SmallString &symbolName)
{
    for (const auto &entry : collector.symbols()) {
        if (entry.second.symbolName == symbolName)
            return entry.first;
    }

    return 0;
}

}
