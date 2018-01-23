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

using ClangBackEnd::FilePath;
using ClangBackEnd::FilePathId;
using ClangBackEnd::FilePathCaching;
using ClangBackEnd::V2::FileContainers;
using ClangBackEnd::SourceLocationEntry;
using ClangBackEnd::SymbolEntry;
using ClangBackEnd::SymbolType;
using ClangBackEnd::SymbolIndex;
using ClangBackEnd::UsedDefine;

using Sqlite::Database;

namespace {

MATCHER_P5(IsSourceLocationEntry, symbolId, filePathId, line, column, symbolType,
           std::string(negation ? "isn't" : "is")
           + PrintToString(SourceLocationEntry{symbolId, filePathId, {line, column}, symbolType})
           )
{
    const SourceLocationEntry &entry = arg;

    return entry.filePathId == filePathId
        && entry.lineColumn.line == line
        && entry.lineColumn.column == column
        && entry.symbolType == symbolType
        && entry.symbolId == symbolId;
}

MATCHER_P2(HasLineColumn, line, column,
           std::string(negation ? "isn't" : "is")
           + PrintToString(Utils::LineColumn{line, column})
           )
{
    const SourceLocationEntry &entry = arg;

    return entry.lineColumn.line == line
        && entry.lineColumn.column == column;
}

MATCHER_P(HasSymbolName, symbolName,
          std::string(negation ? "hasn't" : "has")
          + " symbol name: "
          + symbolName
          )
{
    const SymbolEntry &entry = arg.second;

    return entry.symbolName == symbolName;
}

class SymbolsCollector : public testing::Test
{
protected:
    FilePathId filePathId(Utils::SmallStringView string)
    {
        return filePathCache.filePathId(ClangBackEnd::FilePathView{string});
    }

    SymbolIndex symbolId(const Utils::SmallString &symbolName)
    {
        for (const auto &entry : collector.symbols()) {
            if (entry.second.symbolName == symbolName)
                return entry.first;
        }

        return 0;
    }

protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> initializer{database};
    FilePathCaching filePathCache{database};
    ClangBackEnd::SymbolsCollector collector{filePathCache};
};

TEST_F(SymbolsCollector, CollectSymbolName)
{
    collector.addFiles({filePathId(TESTDATA_DIR "/symbolscollector_simple.cpp")}, {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(),
                Contains(HasSymbolName("x")));
}

TEST_F(SymbolsCollector, SymbolMatchesLocation)
{
    collector.addFiles({filePathId(TESTDATA_DIR "/symbolscollector_simple.cpp")}, {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(
                    AllOf(Field(&SourceLocationEntry::symbolId, symbolId("function")),
                          HasLineColumn(1, 6))));
}

TEST_F(SymbolsCollector, OtherSymboldMatchesLocation)
{
    collector.addFiles({filePathId(TESTDATA_DIR "/symbolscollector_simple.cpp")}, {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(
                    AllOf(Field(&SourceLocationEntry::symbolId, symbolId("function")),
                          HasLineColumn(2, 6))));
}

TEST_F(SymbolsCollector, CollectFilePath)
{
    collector.addFiles({filePathId(TESTDATA_DIR "/symbolscollector_simple.cpp")}, {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(
                    AllOf(Field(&SourceLocationEntry::filePathId,
                                filePathId(TESTDATA_DIR"/symbolscollector_simple.cpp")),
                          Field(&SourceLocationEntry::symbolType, SymbolType::Declaration))));
}

TEST_F(SymbolsCollector, CollectLineColumn)
{
    collector.addFiles({filePathId(TESTDATA_DIR "/symbolscollector_simple.cpp")}, {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(
                    AllOf(HasLineColumn(1, 6),
                          Field(&SourceLocationEntry::symbolType, SymbolType::Declaration))));
}

TEST_F(SymbolsCollector, CollectReference)
{
    collector.addFiles({filePathId(TESTDATA_DIR "/symbolscollector_simple.cpp")}, {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(
                    AllOf(HasLineColumn(14, 5),
                          Field(&SourceLocationEntry::symbolType, SymbolType::DeclarationReference))));
}

TEST_F(SymbolsCollector, ReferencedSymboldMatchesLocation)
{
    collector.addFiles({filePathId(TESTDATA_DIR "/symbolscollector_simple.cpp")}, {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(
                    AllOf(Field(&SourceLocationEntry::symbolId, symbolId("function")),
                          HasLineColumn(14, 5))));
}

TEST_F(SymbolsCollector, DISABLED_ON_WINDOWS(CollectInUnsavedFile))
{
    FileContainers unsaved{{{TESTDATA_DIR, "symbolscollector_generated_file.h"},
                            "void function();",
                            {}}};
    collector.addFiles({filePathId(TESTDATA_DIR "/symbolscollector_unsaved.cpp")},  {"cc"});
    collector.addUnsavedFiles(std::move(unsaved));

    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(),
                Contains(HasSymbolName("function")));
}

TEST_F(SymbolsCollector, SourceFiles)
{
    collector.addFiles({filePathId(TESTDATA_DIR "/symbolscollector_main.cpp")}, {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceFiles(),
                UnorderedElementsAre(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp"),
                                     filePathId(TESTDATA_DIR "/symbolscollector_header1.h"),
                                     filePathId(TESTDATA_DIR "/symbolscollector_header2.h")));
}

TEST_F(SymbolsCollector, MainFileInSourceFiles)
{
    collector.addFiles({filePathId(TESTDATA_DIR "/symbolscollector_main.cpp")}, {"cc"});

    ASSERT_THAT(collector.sourceFiles(),
                ElementsAre(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp")));
}

TEST_F(SymbolsCollector, ResetMainFileInSourceFiles)
{
    collector.addFiles({filePathId(TESTDATA_DIR "/symbolscollector_main.cpp")}, {"cc"});

    ASSERT_THAT(collector.sourceFiles(),
                ElementsAre(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp")));
}

TEST_F(SymbolsCollector, DontDuplicateSourceFiles)
{
    collector.addFiles({filePathId(TESTDATA_DIR "/symbolscollector_main.cpp")}, {"cc"});
    collector.collectSymbols();

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceFiles(),
                UnorderedElementsAre(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp"),
                                     filePathId(TESTDATA_DIR "/symbolscollector_header1.h"),
                                     filePathId(TESTDATA_DIR "/symbolscollector_header2.h")));
}

TEST_F(SymbolsCollector, ClearSourceFiles)
{
    collector.addFiles({filePathId(TESTDATA_DIR "/symbolscollector_main.cpp")}, {"cc"});

    collector.clear();

    ASSERT_THAT(collector.sourceFiles(), IsEmpty());
}

TEST_F(SymbolsCollector, ClearSymbols)
{
    collector.addFiles({filePathId(TESTDATA_DIR "/symbolscollector_main.cpp")}, {"cc"});
    collector.collectSymbols();

    collector.clear();

    ASSERT_THAT(collector.symbols(), IsEmpty());
}

TEST_F(SymbolsCollector, ClearSourceLocations)
{
    collector.addFiles({filePathId(TESTDATA_DIR "/symbolscollector_main.cpp")}, {"cc"});
    collector.collectSymbols();

    collector.clear();

    ASSERT_THAT(collector.sourceLocations(), IsEmpty());
}

TEST_F(SymbolsCollector, DontCollectSymbolsAfterFilesAreCleared)
{
    collector.addFiles({filePathId(TESTDATA_DIR "/symbolscollector_main.cpp")}, {"cc"});

    collector.clear();
    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(), IsEmpty());
}

TEST_F(SymbolsCollector, DontCollectSourceFilesAfterFilesAreCleared)
{
    collector.addFiles({filePathId(TESTDATA_DIR "/symbolscollector_main.cpp")}, {"cc"});

    collector.clear();
    collector.collectSymbols();

    ASSERT_THAT(collector.sourceFiles(), IsEmpty());
}

TEST_F(SymbolsCollector, CollectUsedDefinesWithExternalDefine)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.addFiles({fileId}, {"cc", "-DCOMPILER_ARGUMENT"});

    collector.collectSymbols();

    ASSERT_THAT(collector.usedDefines(),
                ElementsAre(Eq(UsedDefine{"DEFINED", fileId}),
                            Eq(UsedDefine{"IF_DEFINE", fileId}),
                            Eq(UsedDefine{"__clang__", fileId}),
                            Eq(UsedDefine{"IF_NOT_DEFINE", fileId}),
                            Eq(UsedDefine{"MACRO_EXPANSION", fileId}),
                            Eq(UsedDefine{"COMPILER_ARGUMENT", fileId})));
}

TEST_F(SymbolsCollector, CollectUsedDefinesWithoutExternalDefine)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.addFiles({fileId}, {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.usedDefines(),
                ElementsAre(Eq(UsedDefine{"DEFINED", fileId}),
                            Eq(UsedDefine{"IF_DEFINE", fileId}),
                            Eq(UsedDefine{"__clang__", fileId}),
                            Eq(UsedDefine{"IF_NOT_DEFINE", fileId}),
                            Eq(UsedDefine{"MACRO_EXPANSION", fileId}),
                            Eq(UsedDefine{"COMPILER_ARGUMENT", fileId})));
}

TEST_F(SymbolsCollector, DontCollectHeaderGuards)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.addFiles({fileId}, {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.usedDefines(),
                Not(Contains(Eq(UsedDefine{"SYMBOLSCOLLECTOR_DEFINES_H", fileId}))));
}

TEST_F(SymbolsCollector, DontCollectDynamicLibraryExports)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.addFiles({fileId}, {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.usedDefines(),
                Not(Contains(Eq(UsedDefine{"CLASS_EXPORT", fileId}))));
}

TEST_F(SymbolsCollector, CollectMacroDefinitionSourceLocation)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.addFiles({fileId}, {"cc", "-DCOMPILER_ARGUMENT"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(IsSourceLocationEntry(symbolId("IF_NOT_DEFINE"), fileId, 4, 9, SymbolType::MacroDefinition)));
}

TEST_F(SymbolsCollector, CollectMacroUsageInIfNotDefSourceLocation)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.addFiles({fileId}, {"cc", "-DCOMPILER_ARGUMENT"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(IsSourceLocationEntry(symbolId("IF_NOT_DEFINE"), fileId, 6, 9, SymbolType::MacroUsage)));
}

TEST_F(SymbolsCollector, CollectSecondMacroUsageInIfNotDefSourceLocation)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.addFiles({fileId}, {"cc", "-DCOMPILER_ARGUMENT"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(IsSourceLocationEntry(symbolId("IF_NOT_DEFINE"), fileId, 9, 9, SymbolType::MacroUsage)));
}

TEST_F(SymbolsCollector, CollectMacroUsageCompilerArgumentSourceLocation)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.addFiles({fileId}, {"cc", "-DCOMPILER_ARGUMENT"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(IsSourceLocationEntry(symbolId("COMPILER_ARGUMENT"), fileId, 12, 9, SymbolType::MacroUsage)));
}

TEST_F(SymbolsCollector, CollectMacroUsageInIfDefSourceLocation)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.addFiles({fileId}, {"cc", "-DCOMPILER_ARGUMENT"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(IsSourceLocationEntry(symbolId("IF_DEFINE"), fileId, 17, 8, SymbolType::MacroUsage)));
}

TEST_F(SymbolsCollector, CollectMacroUsageInDefinedSourceLocation)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.addFiles({fileId}, {"cc", "-DCOMPILER_ARGUMENT"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(IsSourceLocationEntry(symbolId("DEFINED"), fileId, 22, 13, SymbolType::MacroUsage)));
}

TEST_F(SymbolsCollector, CollectMacroUsageExpansionSourceLocation)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.addFiles({fileId}, {"cc", "-DCOMPILER_ARGUMENT"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(IsSourceLocationEntry(symbolId("MACRO_EXPANSION"), fileId, 27, 10, SymbolType::MacroUsage)));
}

TEST_F(SymbolsCollector, CollectMacroUsageUndefSourceLocation)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.addFiles({fileId}, {"cc", "-DCOMPILER_ARGUMENT"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(IsSourceLocationEntry(symbolId("UN_DEFINE"), fileId, 34, 8, SymbolType::MacroUndefinition)));
}

TEST_F(SymbolsCollector, CollectMacroUsageBuiltInSourceLocation)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.addFiles({fileId}, {"cc", "-DCOMPILER_ARGUMENT"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(IsSourceLocationEntry(symbolId("__clang__"), fileId, 29, 9, SymbolType::MacroUsage)));
}

TEST_F(SymbolsCollector, CollectMacroDefinitionSymbols)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.addFiles({fileId}, {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(),
                Contains(HasSymbolName("IF_NOT_DEFINE")));
}

TEST_F(SymbolsCollector, CollectMacroBuiltInSymbols)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.addFiles({fileId}, {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(),
                Contains(HasSymbolName("__clang__")));
}

TEST_F(SymbolsCollector, CollectMacroCompilerArgumentSymbols)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.addFiles({fileId}, {"cc", "-DCOMPILER_ARGUMENT"});

    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(),
                Contains(HasSymbolName("COMPILER_ARGUMENT")));
}

}
