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
using ClangBackEnd::FilePathCaching;
using ClangBackEnd::FilePathId;
using ClangBackEnd::FileStatus;
using ClangBackEnd::SourceDependency;
using ClangBackEnd::SourceLocationEntry;
using ClangBackEnd::SourceLocationKind;
using ClangBackEnd::SymbolEntry;
using ClangBackEnd::SymbolIndex;
using ClangBackEnd::SymbolKind;
using ClangBackEnd::SymbolTag;
using ClangBackEnd::UsedMacro;
using ClangBackEnd::V2::FileContainers;

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
    SymbolsCollector() { setFilePathCache(&filePathCache); }
    ~SymbolsCollector() { setFilePathCache({}); }

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
        return QFileInfo(QString(filePath)).lastModified().toSecsSinceEpoch();
    }

    ClangBackEnd::FileStatus fileStatus(Utils::SmallStringView filePath) const
    {
        return {filePathId(filePath), fileSize(filePath), lastModified(filePath)};
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
    ClangBackEnd::SymbolsCollector collector{filePathCache};
};

TEST_F(SymbolsCollector, CollectSymbolName)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector/simple.cpp"), {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(),
                Contains(HasSymbolName("function")));
}

TEST_F(SymbolsCollector, SymbolMatchesLocation)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector/simple.cpp"), {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(
                    AllOf(Field(&SourceLocationEntry::symbolId, symbolId("function")),
                          HasLineColumn(1, 6))));
}

TEST_F(SymbolsCollector, OtherSymboldMatchesLocation)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector/simple.cpp"), {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(
                    AllOf(Field(&SourceLocationEntry::symbolId, symbolId("function")),
                          HasLineColumn(2, 6))));
}

TEST_F(SymbolsCollector, CollectFilePath)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector/simple.cpp"), {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(AllOf(Field(&SourceLocationEntry::filePathId,
                                     filePathId(TESTDATA_DIR "/symbolscollector/simple.cpp")),
                               Field(&SourceLocationEntry::kind, SourceLocationKind::Declaration))));
}

TEST_F(SymbolsCollector, CollectLineColumn)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector/simple.cpp"), {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(
                    AllOf(HasLineColumn(1, 6),
                          Field(&SourceLocationEntry::kind, SourceLocationKind::Declaration))));
}

TEST_F(SymbolsCollector, CollectReference)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector/simple.cpp"), {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(
                    AllOf(HasLineColumn(14, 5),
                          Field(&SourceLocationEntry::kind, SourceLocationKind::DeclarationReference))));
}

TEST_F(SymbolsCollector, ReferencedSymboldMatchesLocation)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector/simple.cpp"), {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(
                    AllOf(Field(&SourceLocationEntry::symbolId, symbolId("function")),
                          HasLineColumn(14, 5))));
}

TEST_F(SymbolsCollector, DISABLED_ON_WINDOWS(CollectInUnsavedFile))
{
    FileContainers unsaved{{{TESTDATA_DIR, "symbolscollector/generated_file.h"},
                            filePathId({TESTDATA_DIR, "symbolscollector/generated_file.h"}),
                            "void function();",
                            {}}};
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector/unsaved.cpp"), {"cc"});
    collector.setUnsavedFiles(std::move(unsaved));

    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(),
                Contains(HasSymbolName("function")));
}

TEST_F(SymbolsCollector, ClearSymbols)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector/main.cpp"), {"cc"});
    collector.collectSymbols();

    collector.clear();

    ASSERT_THAT(collector.symbols(), IsEmpty());
}

TEST_F(SymbolsCollector, ClearSourceLocations)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector/main.cpp"), {"cc"});
    collector.collectSymbols();

    collector.clear();

    ASSERT_THAT(collector.sourceLocations(), IsEmpty());
}

TEST_F(SymbolsCollector, DontCollectSymbolsAfterFilesAreCleared)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector/main.cpp"), {"cc"});

    collector.clear();
    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(), IsEmpty());
}

TEST_F(SymbolsCollector, CollectMacroDefinitionSourceLocation)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector/defines.h");
    collector.setFile(fileId, {"cc", "-DCOMPILER_ARGUMENT"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(IsSourceLocationEntry(symbolId("IF_NOT_DEFINE"), fileId, 4, 9, SourceLocationKind::MacroDefinition)));
}

TEST_F(SymbolsCollector, DISABLED_CollectMacroUsageInIfNotDefSourceLocation)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector/defines.h");
    collector.setFile(fileId, {"cc", "-DCOMPILER_ARGUMENT"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(IsSourceLocationEntry(symbolId("IF_NOT_DEFINE"), fileId, 6, 9, SourceLocationKind::MacroUsage)));
}

TEST_F(SymbolsCollector, DISABLED_CollectSecondMacroUsageInIfNotDefSourceLocation)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector/defines.h");
    collector.setFile(fileId, {"cc", "-DCOMPILER_ARGUMENT"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(IsSourceLocationEntry(symbolId("IF_NOT_DEFINE"), fileId, 9, 9, SourceLocationKind::MacroUsage)));
}

TEST_F(SymbolsCollector, DISABLED_CollectMacroUsageCompilerArgumentSourceLocation)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector/defines.h");
    collector.setFile(fileId, {"cc", "-DCOMPILER_ARGUMENT"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(IsSourceLocationEntry(symbolId("COMPILER_ARGUMENT"), fileId, 12, 9, SourceLocationKind::MacroUsage)));
}

TEST_F(SymbolsCollector, DISABLED_CollectMacroUsageInIfDefSourceLocation)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector/defines.h");
    collector.setFile(fileId, {"cc", "-DCOMPILER_ARGUMENT"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(IsSourceLocationEntry(symbolId("IF_DEFINE"), fileId, 17, 8, SourceLocationKind::MacroUsage)));
}

TEST_F(SymbolsCollector, DISABLED_CollectMacroUsageInDefinedSourceLocation)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector/defines.h");
    collector.setFile(fileId, {"cc", "-DCOMPILER_ARGUMENT"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(IsSourceLocationEntry(symbolId("DEFINED"), fileId, 22, 13, SourceLocationKind::MacroUsage)));
}

TEST_F(SymbolsCollector, CollectMacroUsageExpansionSourceLocation)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector/defines.h");
    collector.setFile(fileId, {"cc", "-DCOMPILER_ARGUMENT"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(IsSourceLocationEntry(symbolId("MACRO_EXPANSION"), fileId, 27, 10, SourceLocationKind::MacroUsage)));
}

TEST_F(SymbolsCollector, CollectMacroUsageUndefSourceLocation)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector/defines.h");
    collector.setFile(fileId, {"cc", "-DCOMPILER_ARGUMENT"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(IsSourceLocationEntry(symbolId("UN_DEFINE"), fileId, 34, 8, SourceLocationKind::MacroUndefinition)));
}

TEST_F(SymbolsCollector, DISABLED_CollectMacroUsageBuiltInSourceLocation)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector/defines.h");
    collector.setFile(fileId, {"cc", "-DCOMPILER_ARGUMENT"});

    collector.collectSymbols();

    ASSERT_THAT(collector.sourceLocations(),
                Contains(IsSourceLocationEntry(symbolId("__clang__"), fileId, 29, 9, SourceLocationKind::MacroUsage)));
}

TEST_F(SymbolsCollector, CollectMacroDefinitionSymbols)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector/defines.h");
    collector.setFile(fileId, {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(),
                Contains(AllOf(HasSymbolName("IF_NOT_DEFINE"), HasSymbolKind(SymbolKind::Macro))));
}

TEST_F(SymbolsCollector, DISABLED_CollectMacroBuiltInSymbols)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector/defines.h");
    collector.setFile(fileId, {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(),
                Contains(AllOf(HasSymbolName("__clang__"), HasSymbolKind(SymbolKind::Macro))));
}

TEST_F(SymbolsCollector, DISABLED_CollectMacroCompilerArgumentSymbols)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector/defines.h");
    collector.setFile(fileId, {"cc", "-DCOMPILER_ARGUMENT"});

    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(),
                Contains(AllOf(HasSymbolName("COMPILER_ARGUMENT"), HasSymbolKind(SymbolKind::Macro))));
}

TEST_F(SymbolsCollector, IsClassSymbol)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector/symbolkind.cpp"), {"cc"});

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
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector/symbolkind.cpp"), {"cc"});

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
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector/symbolkind.cpp"), {"cc"});

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
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector/symbolkind.cpp"), {"cc"});

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
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector/symbolkind.cpp"), {"cc"});

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
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector/symbolkind.cpp"), {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(),
                Contains(
                    AllOf(
                        HasSymbolName("Function"),
                        HasSymbolKind(SymbolKind::Function))));
}

TEST_F(SymbolsCollector, IsVariableSymbol)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector/symbolkind.cpp"), {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(),
                Contains(
                    AllOf(
                        HasSymbolName("Variable"),
                        HasSymbolKind(SymbolKind::Variable))));
}

TEST_F(SymbolsCollector, IndexUnmodifiedHeaderFilesAtFirstRun)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector/unmodified.cpp"),
                      {"cc", "-I", {TESTDATA_DIR, "/symbolscollector/include"}});

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
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector/unmodified.cpp"),
                      {"cc", "-I", {TESTDATA_DIR, "/symbolscollector/include"}});
    collector.collectSymbols();
    collector.clear();
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector/unmodified2.cpp"),
                      {"cc", "-I", {TESTDATA_DIR, "/symbolscollector/include"}});

    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(),
                AllOf(Contains(HasSymbolName("HeaderFunctionReferenceInMainFile")),
                      Not(Contains(HasSymbolName("MemberReference"))),
                      Not(Contains(HasSymbolName("HeaderFunctionReference"))),
                      Not(Contains(HasSymbolName("HeaderFunction"))),
                      Not(Contains(HasSymbolName("Class"))),
                      Not(Contains(HasSymbolName("Member"))),
                      Not(Contains(HasSymbolName("HEADER_DEFINE")))));
}

TEST_F(SymbolsCollector, DontIndexUnmodifiedHeaderFilesAtTouchHeader)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector/unmodified3.cpp"),
                      {"cc", "-I", {TESTDATA_DIR, "/symbolscollector/include"}});
    collector.collectSymbols();
    collector.clear();
    touchFile(TESTDATA_DIR "/symbolscollector/include/unmodified_header2.h");
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector/unmodified3.cpp"),
                      {"cc", "-I", {TESTDATA_DIR, "/symbolscollector/include"}});

    collector.collectSymbols();

    ASSERT_THAT(collector.symbols(),
                AllOf(Contains(HasSymbolName("TouchHeaderFunction")),
                      Contains(HasSymbolName("HeaderFunctionReferenceInMainFile")),
                      Not(Contains(HasSymbolName("MemberReference"))),
                      Not(Contains(HasSymbolName("HeaderFunctionReference"))),
                      Not(Contains(HasSymbolName("HeaderFunction"))),
                      Not(Contains(HasSymbolName("Class"))),
                      Not(Contains(HasSymbolName("Member"))),
                      Not(Contains(HasSymbolName("HEADER_DEFINE")))));
}

TEST_F(SymbolsCollector, DontIndexSystemIncudes)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector/unmodified.cpp"),
                      {"cc", "-isystem", {TESTDATA_DIR, "/symbolscollector/include"}});

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

TEST_F(SymbolsCollector, CollectReturnsFalseIfThereIsError)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector/error.cpp"), {"cc"});

    bool success = collector.collectSymbols();

    ASSERT_FALSE(success);
}

TEST_F(SymbolsCollector, CollectReturnsFalseIfThereIsNoError)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector/main.cpp"), {"cc"});

    bool success = collector.collectSymbols();

    ASSERT_TRUE(success);
}

TEST_F(SymbolsCollector, ClearInputFilesAfterCollectingSymbols)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector/main2.cpp"), {"cc"});
    collector.collectSymbols();
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector/main.cpp"), {"cc"});

    collector.collectSymbols();

    ASSERT_TRUE(collector.isClean());
}

TEST_F(SymbolsCollector, ClassDeclarations)
{
    collector.setFile(filePathId(TESTDATA_DIR "/symbolscollector/class.cpp"), {"cc"});

    collector.collectSymbols();

    ASSERT_THAT(
        collector.sourceLocations(),
        AllOf(Contains(IsSourceLocationEntry(symbolId("Class"),
                                             filePathId(TESTDATA_DIR "/symbolscollector/class.cpp"),
                                             1,
                                             7,
                                             SourceLocationKind::Definition)),
              Contains(IsSourceLocationEntry(symbolId("bar"),
                                             filePathId(TESTDATA_DIR "/symbolscollector/class.cpp"),
                                             8,
                                             8,
                                             SourceLocationKind::Definition)),
              Contains(IsSourceLocationEntry(symbolId("foo"),
                                             filePathId(TESTDATA_DIR "/symbolscollector/class.cpp"),
                                             11,
                                             13,
                                             SourceLocationKind::Definition)),
              Contains(IsSourceLocationEntry(symbolId("foo"),
                                             filePathId(TESTDATA_DIR "/symbolscollector/class.cpp"),
                                             6,
                                             8,
                                             SourceLocationKind::Declaration)),
              Contains(IsSourceLocationEntry(symbolId("Class"),
                                             filePathId(TESTDATA_DIR "/symbolscollector/class.cpp"),
                                             11,
                                             6,
                                             SourceLocationKind::DeclarationReference)),
              Contains(IsSourceLocationEntry(symbolId("bar"),
                                             filePathId(TESTDATA_DIR "/symbolscollector/class.cpp"),
                                             13,
                                             5,
                                             SourceLocationKind::DeclarationReference))));
}
} // namespace
