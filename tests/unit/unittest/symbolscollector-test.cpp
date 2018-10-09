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
#include <filestatus.h>

#include <filepathcaching.h>
#include <refactoringdatabaseinitializer.h>

#include <QDateTime>
#include <QDir>

#ifdef _WIN32
#include <windows.h>
#else
#include <utime.h>
#endif

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
using ClangBackEnd::SourceDependency;
using ClangBackEnd::SourceLocationEntry;
using ClangBackEnd::SymbolEntry;
using ClangBackEnd::SymbolKind;
using ClangBackEnd::SymbolTag;
using ClangBackEnd::SourceLocationKind;
using ClangBackEnd::SymbolIndex;
using ClangBackEnd::UsedMacro;

using Sqlite::Database;

namespace {

MATCHER_P5(IsSourceLocationEntry, symbolId, filePathId, line, column, kind,
           std::string(negation ? "isn't" : "is")
           + PrintToString(SourceLocationEntry{symbolId, filePathId, {line, column}, kind})
           )
{
    const SourceLocationEntry &entry = arg;

    return entry.filePathId == filePathId
        && entry.lineColumn.line == line
        && entry.lineColumn.column == column
        && entry.kind == kind
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

MATCHER_P(HasSymbolKind, symbolKind,
           std::string(negation ? "hasn't" : "has")
           + " and symbol kind: "
           + PrintToString(symbolKind)
           )
{
    const SymbolEntry &entry = arg.second;

    return entry.symbolKind == symbolKind;
}

MATCHER_P(HasSymbolTag, symbolTag,
          std::string(negation ? "hasn't" : "has")
          + " and symbol tag: "
          + PrintToString(symbolTag)
          )
{
    const SymbolEntry &entry = arg.second;

    return entry.symbolTags.contains(symbolTag);
}

class SymbolsCollector : public testing::Test
{
protected:
    FilePathId filePathId(Utils::SmallStringView filePath) const
    {
        return filePathCache.filePathId(ClangBackEnd::FilePathView{filePath});
    }

    static off_t fileSize(Utils::SmallStringView filePath)
    {
        return QFileInfo(QString(filePath)).size();
    }

   static std::time_t lastModified(Utils::SmallStringView filePath)
    {
        return QFileInfo(QString(filePath)).lastModified().toTime_t();
    }

    ClangBackEnd::FileStatus fileStatus(Utils::SmallStringView filePath) const
    {
        return {filePathId(filePath), fileSize(filePath), lastModified(filePath), false};
    }

    SymbolIndex symbolId(const Utils::SmallString &symbolName)
    {
        for (const auto &entry : collector.symbols()) {
            if (entry.second.symbolName == symbolName)
                return entry.first;
        }

        return 0;
    }

    void touchFile(const char *filePath)
    {
#ifdef _WIN32
        QFile::resize(QString::fromUtf8(filePath), QFileInfo(QString::fromUtf8(filePath)).size());
#else
        utime(filePath, nullptr);
#endif
    }

protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> initializer{database};
    FilePathCaching filePathCache{database};
    ClangBackEnd::SymbolsCollector collector{database};
};

TEST_F(SymbolsCollector, CollectSymbolName)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_simple.cpp"), {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(),
                Contains(HasSymbolName("function")));
}

TEST_F(SymbolsCollector, SymbolMatchesLocation)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_simple.cpp"), {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(
                    AllOf(Field(&SourceLocationEntry::symbolId, symbolId("function")),
                          HasLineColumn(1, 6))));
}

TEST_F(SymbolsCollector, OtherSymboldMatchesLocation)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_simple.cpp"), {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(
                    AllOf(Field(&SourceLocationEntry::symbolId, symbolId("function")),
                          HasLineColumn(2, 6))));
}

TEST_F(SymbolsCollector, CollectFilePath)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_simple.cpp"), {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(
                    AllOf(Field(&SourceLocationEntry::filePathId,
                                filePathId(TESTDATA_DIR"/symbolscollector_simple.cpp")),
                          Field(&SourceLocationEntry::kind, SourceLocationKind::Declaration))));
}

TEST_F(SymbolsCollector, CollectLineColumn)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_simple.cpp"), {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(
                    AllOf(HasLineColumn(1, 6),
                          Field(&SourceLocationEntry::kind, SourceLocationKind::Declaration))));
}

TEST_F(SymbolsCollector, CollectReference)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_simple.cpp"), {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(
                    AllOf(HasLineColumn(14, 5),
                          Field(&SourceLocationEntry::kind, SourceLocationKind::DeclarationReference))));
}

TEST_F(SymbolsCollector, ReferencedSymboldMatchesLocation)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_simple.cpp"), {"cc"});

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
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_unsaved.cpp"),  {"cc"});
    collector.setUnsavedFiles(std::move(unsaved));

    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(),
                Contains(HasSymbolName("function")));
}

TEST_F(SymbolsCollector, SourceFiles)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceFiles(),
                UnorderedElementsAre(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp"),
                                     filePathId(TESTDATA_DIR "/symbolscollector_header1.h"),
                                     filePathId(TESTDATA_DIR "/symbolscollector_header2.h")));
}

TEST_F(SymbolsCollector, MainFileInSourceFiles)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc"});

    ASSERT_THAT(collector.sourceFiles(),
                ElementsAre(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp")));
}

TEST_F(SymbolsCollector, ResetMainFileInSourceFiles)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc"});

    ASSERT_THAT(collector.sourceFiles(),
                ElementsAre(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp")));
}

TEST_F(SymbolsCollector, DontDuplicateSourceFiles)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc"});
    collector.collectSymbols();

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceFiles(),
                UnorderedElementsAre(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp"),
                                     filePathId(TESTDATA_DIR "/symbolscollector_header1.h"),
                                     filePathId(TESTDATA_DIR "/symbolscollector_header2.h")));
}

TEST_F(SymbolsCollector, ClearSourceFiles)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc"});

    collector.clear();

    ASSERT_THAT(collector.sourceFiles(), IsEmpty());
}

TEST_F(SymbolsCollector, ClearSymbols)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc"});
    collector.collectSymbols();

    collector.clear();

    ASSERT_THAT(collector.symbols(), IsEmpty());
}

TEST_F(SymbolsCollector, ClearSourceLocations)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc"});
    collector.collectSymbols();

    collector.clear();

    ASSERT_THAT(collector.sourceLocations(), IsEmpty());
}

TEST_F(SymbolsCollector, ClearFileStatus)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc"});
    collector.collectSymbols();

    collector.clear();

    ASSERT_THAT(collector.fileStatuses(), IsEmpty());
}

TEST_F(SymbolsCollector, ClearUsedMacros)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_defines.h"), {"cc"});
    collector.collectSymbols();

    collector.clear();

    ASSERT_THAT(collector.usedMacros(), IsEmpty());
}

TEST_F(SymbolsCollector, ClearSourceDependencies)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_main2.cpp"), {"cc", "-I" TESTDATA_DIR});
    collector.collectSymbols();

    collector.clear();

    ASSERT_THAT(collector.sourceDependencies(), IsEmpty());
}

TEST_F(SymbolsCollector, DontCollectSymbolsAfterFilesAreCleared)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc"});

    collector.clear();
    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(), IsEmpty());
}

TEST_F(SymbolsCollector, DontCollectSourceFilesAfterFilesAreCleared)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc"});

    collector.clear();
    collector.collectSymbols();

    ASSERT_THAT(collector.sourceFiles(), IsEmpty());
}

TEST_F(SymbolsCollector, DontCollectFileStatusAfterFilesAreCleared)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc"});

    collector.clear();
    collector.collectSymbols();

    ASSERT_THAT(collector.fileStatuses(), IsEmpty());
}

TEST_F(SymbolsCollector, DontCollectUsedMacrosAfterFilesAreCleared)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc"});

    collector.clear();
    collector.collectSymbols();

    ASSERT_THAT(collector.usedMacros(), IsEmpty());
}

TEST_F(SymbolsCollector, DontCollectSourceDependenciesAfterFilesAreCleared)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc"});

    collector.clear();
    collector.collectSymbols();

    ASSERT_THAT(collector.sourceDependencies(), IsEmpty());
}

TEST_F(SymbolsCollector, CollectUsedMacrosWithExternalDefine)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.setFile(fileId, {"cc", "-DCOMPILER_ARGUMENT"});

    collector.collectSymbols();

    ASSERT_THAT(collector.usedMacros(),
                ElementsAre(Eq(UsedMacro{"DEFINED", fileId}),
                            Eq(UsedMacro{"IF_DEFINE", fileId}),
                            Eq(UsedMacro{"__clang__", fileId}),
                            Eq(UsedMacro{"CLASS_EXPORT", fileId}),
                            Eq(UsedMacro{"IF_NOT_DEFINE", fileId}),
                            Eq(UsedMacro{"MACRO_EXPANSION", fileId}),
                            Eq(UsedMacro{"COMPILER_ARGUMENT", fileId})));
}

TEST_F(SymbolsCollector, CollectUsedMacrosWithoutExternalDefine)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.setFile(fileId, {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.usedMacros(),
                ElementsAre(Eq(UsedMacro{"DEFINED", fileId}),
                            Eq(UsedMacro{"IF_DEFINE", fileId}),
                            Eq(UsedMacro{"__clang__", fileId}),
                            Eq(UsedMacro{"CLASS_EXPORT", fileId}),
                            Eq(UsedMacro{"IF_NOT_DEFINE", fileId}),
                            Eq(UsedMacro{"MACRO_EXPANSION", fileId}),
                            Eq(UsedMacro{"COMPILER_ARGUMENT", fileId})));
}

TEST_F(SymbolsCollector, DontCollectHeaderGuards)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.setFile(fileId, {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.usedMacros(),
                Not(Contains(Eq(UsedMacro{"SYMBOLSCOLLECTOR_DEFINES_H", fileId}))));
}

TEST_F(SymbolsCollector, DISABLED_DontCollectDynamicLibraryExports)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.setFile(fileId, {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.usedMacros(),
                Not(Contains(Eq(UsedMacro{"CLASS_EXPORT", fileId}))));
}

TEST_F(SymbolsCollector, CollectMacroDefinitionSourceLocation)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.setFile(fileId, {"cc", "-DCOMPILER_ARGUMENT"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(IsSourceLocationEntry(symbolId("IF_NOT_DEFINE"), fileId, 4, 9, SourceLocationKind::MacroDefinition)));
}

TEST_F(SymbolsCollector, CollectMacroUsageInIfNotDefSourceLocation)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.setFile(fileId, {"cc", "-DCOMPILER_ARGUMENT"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(IsSourceLocationEntry(symbolId("IF_NOT_DEFINE"), fileId, 6, 9, SourceLocationKind::MacroUsage)));
}

TEST_F(SymbolsCollector, CollectSecondMacroUsageInIfNotDefSourceLocation)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.setFile(fileId, {"cc", "-DCOMPILER_ARGUMENT"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(IsSourceLocationEntry(symbolId("IF_NOT_DEFINE"), fileId, 9, 9, SourceLocationKind::MacroUsage)));
}

TEST_F(SymbolsCollector, CollectMacroUsageCompilerArgumentSourceLocation)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.setFile(fileId, {"cc", "-DCOMPILER_ARGUMENT"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(IsSourceLocationEntry(symbolId("COMPILER_ARGUMENT"), fileId, 12, 9, SourceLocationKind::MacroUsage)));
}

TEST_F(SymbolsCollector, CollectMacroUsageInIfDefSourceLocation)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.setFile(fileId, {"cc", "-DCOMPILER_ARGUMENT"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(IsSourceLocationEntry(symbolId("IF_DEFINE"), fileId, 17, 8, SourceLocationKind::MacroUsage)));
}

TEST_F(SymbolsCollector, CollectMacroUsageInDefinedSourceLocation)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.setFile(fileId, {"cc", "-DCOMPILER_ARGUMENT"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(IsSourceLocationEntry(symbolId("DEFINED"), fileId, 22, 13, SourceLocationKind::MacroUsage)));
}

TEST_F(SymbolsCollector, CollectMacroUsageExpansionSourceLocation)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.setFile(fileId, {"cc", "-DCOMPILER_ARGUMENT"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(IsSourceLocationEntry(symbolId("MACRO_EXPANSION"), fileId, 27, 10, SourceLocationKind::MacroUsage)));
}

TEST_F(SymbolsCollector, CollectMacroUsageUndefSourceLocation)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.setFile(fileId, {"cc", "-DCOMPILER_ARGUMENT"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(IsSourceLocationEntry(symbolId("UN_DEFINE"), fileId, 34, 8, SourceLocationKind::MacroUndefinition)));
}

TEST_F(SymbolsCollector, CollectMacroUsageBuiltInSourceLocation)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.setFile(fileId, {"cc", "-DCOMPILER_ARGUMENT"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(IsSourceLocationEntry(symbolId("__clang__"), fileId, 29, 9, SourceLocationKind::MacroUsage)));
}

TEST_F(SymbolsCollector, CollectMacroDefinitionSymbols)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.setFile(fileId, {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(),
                Contains(AllOf(HasSymbolName("IF_NOT_DEFINE"), HasSymbolKind(SymbolKind::Macro))));
}

TEST_F(SymbolsCollector, CollectMacroBuiltInSymbols)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.setFile(fileId, {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(),
                Contains(AllOf(HasSymbolName("__clang__"), HasSymbolKind(SymbolKind::Macro))));
}

TEST_F(SymbolsCollector, CollectMacroCompilerArgumentSymbols)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.setFile(fileId, {"cc", "-DCOMPILER_ARGUMENT"});

    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(),
                Contains(AllOf(HasSymbolName("COMPILER_ARGUMENT"), HasSymbolKind(SymbolKind::Macro))));
}

TEST_F(SymbolsCollector, CollectFileStatuses)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_main.cpp");
    collector.setFile(fileId, {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.fileStatuses(),
                ElementsAre(
                    fileStatus(TESTDATA_DIR "/symbolscollector_main.cpp"),
                    fileStatus(TESTDATA_DIR "/symbolscollector_header1.h"),
                    fileStatus(TESTDATA_DIR "/symbolscollector_header2.h")));
}

TEST_F(SymbolsCollector, CollectSourceDependencies)
{
    auto mainFileId = filePathId(TESTDATA_DIR "/symbolscollector_main2.cpp");
    auto header1FileId = filePathId(TESTDATA_DIR "/symbolscollector_header1.h");
    auto header2FileId = filePathId(TESTDATA_DIR "/symbolscollector_header2.h");
    auto header3FileId = filePathId(TESTDATA_DIR "/symbolscollector_header3.h");
    collector.setFile(mainFileId, {"cc", "-I" TESTDATA_DIR});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceDependencies(),
                UnorderedElementsAre(SourceDependency(mainFileId, header1FileId),
                                     SourceDependency(mainFileId, header3FileId),
                                     SourceDependency(header3FileId, header2FileId),
                                     SourceDependency(header1FileId, header2FileId)));
}

TEST_F(SymbolsCollector, IsClassSymbol)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_symbolkind.cpp"), {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(),
                Contains(
                    AllOf(
                        HasSymbolName("Class"),
                        HasSymbolKind(SymbolKind::Record),
                        HasSymbolTag(SymbolTag::Class))));
}

TEST_F(SymbolsCollector, IsStructSymbol)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_symbolkind.cpp"), {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(),
                Contains(
                    AllOf(
                        HasSymbolName("Struct"),
                        HasSymbolKind(SymbolKind::Record),
                        HasSymbolTag(SymbolTag::Struct))));
}

TEST_F(SymbolsCollector, IsEnumerationSymbol)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_symbolkind.cpp"), {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(),
                AllOf(
                    Contains(
                        AllOf(
                            HasSymbolName("Enumeration"),
                            HasSymbolKind(SymbolKind::Enumeration))),
                    Contains(
                        AllOf(
                            HasSymbolName("ScopedEnumeration"),
                            HasSymbolKind(SymbolKind::Enumeration)))));
}

TEST_F(SymbolsCollector, IsUnionSymbol)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_symbolkind.cpp"), {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(),
                Contains(
                    AllOf(
                        HasSymbolName("Union"),
                        HasSymbolKind(SymbolKind::Record),
                        HasSymbolTag(SymbolTag::Union))));
}

TEST_F(SymbolsCollector, DISABLED_ON_NON_WINDOWS(IsMsvcInterfaceSymbol))
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_symbolkind.cpp"), {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(),
                Contains(
                    AllOf(
                        HasSymbolName("MsvcInterface"),
                        HasSymbolKind(SymbolKind::Record),
                        HasSymbolTag(SymbolTag::MsvcInterface))));
}

TEST_F(SymbolsCollector, IsFunctionSymbol)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_symbolkind.cpp"), {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(),
                Contains(
                    AllOf(
                        HasSymbolName("Function"),
                        HasSymbolKind(SymbolKind::Function))));
}

TEST_F(SymbolsCollector, IsVariableSymbol)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_symbolkind.cpp"), {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(),
                Contains(
                    AllOf(
                        HasSymbolName("Variable"),
                        HasSymbolKind(SymbolKind::Variable))));
}

TEST_F(SymbolsCollector, IndexUnmodifiedHeaderFilesAtFirstRun)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_unmodified.cpp"), {"cc", "-I", {TESTDATA_DIR, "/include"}});

    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(),
                AllOf(
                    Contains(HasSymbolName("MemberReference")),
                    Contains(HasSymbolName("HeaderFunction")),
                    Contains(HasSymbolName("HeaderFunctionReference")),
                    Contains(HasSymbolName("Class")),
                    Contains(HasSymbolName("Member")),
                    Contains(HasSymbolName("HEADER_DEFINE")),
                    Contains(HasSymbolName("FunctionLocalVariable"))));
}

TEST_F(SymbolsCollector, DontIndexUnmodifiedHeaderFilesAtSecondRun)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_unmodified.cpp"), {"cc", "-I", {TESTDATA_DIR, "/include"}});
    collector.collectSymbols();
    collector.clear();
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_unmodified2.cpp"), {"cc", "-I", {TESTDATA_DIR, "/include"}});

    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(),
                AllOf(
                    Contains(HasSymbolName("HeaderFunctionReferenceInMainFile")),
                    Not(Contains(HasSymbolName("MemberReference"))),
                    Not(Contains(HasSymbolName("HeaderFunctionReference"))),
                    Not(Contains(HasSymbolName("HeaderFunction"))),
                    Not(Contains(HasSymbolName("Class"))),
                    Not(Contains(HasSymbolName("Member")))));
}

TEST_F(SymbolsCollector, DontIndexUnmodifiedHeaderFilesAtTouchHeader)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_unmodified3.cpp"), {"cc", "-I", {TESTDATA_DIR, "/include"}});
    collector.collectSymbols();
    collector.clear();
    touchFile(TESTDATA_DIR "/include/symbolscollector_unmodified_header2.h");
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_unmodified3.cpp"), {"cc", "-I", {TESTDATA_DIR, "/include"}});

    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(),
                AllOf(
                    Contains(HasSymbolName("HeaderFunctionReferenceInMainFile")),
                    Not(Contains(HasSymbolName("MemberReference"))),
                    Not(Contains(HasSymbolName("HeaderFunctionReference"))),
                    Not(Contains(HasSymbolName("HeaderFunction"))),
                    Not(Contains(HasSymbolName("Class"))),
                    Not(Contains(HasSymbolName("Member")))));
}

TEST_F(SymbolsCollector, DontIndexSystemIncudes)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector_unmodified.cpp"), {"cc", "-isystem", {TESTDATA_DIR, "/include"}});

    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(),
                AllOf(
                    Contains(HasSymbolName("MainFileFunction")),
                    Contains(HasSymbolName("HeaderFunctionReferenceInMainFile")),
                    Not(Contains(HasSymbolName("MemberReference"))),
                    Not(Contains(HasSymbolName("HeaderFunction"))),
                    Not(Contains(HasSymbolName("Class"))),
                    Not(Contains(HasSymbolName("Member"))),
                    Not(Contains(HasSymbolName("HEADER_DEFINE"))),
                    Not(Contains(HasSymbolName("FunctionLocalVariable")))));
}
}
