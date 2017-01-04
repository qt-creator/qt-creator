/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <clangrefactoringbackend_global.h>
#include <symbolfinder.h>

#include "filesystem-utilities.h"

using ClangBackEnd::SymbolFinder;
using ClangBackEnd::FileContent;

using testing::PrintToString;
using testing::AllOf;
using testing::Contains;
using testing::Not;

namespace {

MATCHER_P2(IsSourceLocation, line, column,
           std::string(negation ? "isn't" : "is")
           + "line " + PrintToString(line)
           + ", column " + PrintToString(column)
           )
{
    if (arg.line() != uint(line)
            || arg.column() != uint(column))
        return false;

    return true;
}

MATCHER_P(StrEq, text,
           std::string(negation ? "isn't" : "is")
           + " text " + PrintToString(text)
           )
{
    return std::string(arg.data(), arg.size()) == std::string(text);
}

TEST(SymbolFinder, FileContentFilePath)
{
    FileContent fileContent(toNativePath("/tmp"), "data.cpp", "int variable;", {"cc", "data.cpp"});

    ASSERT_THAT(fileContent.filePath, toNativePath("/tmp/data.cpp"));
}

TEST(SymbolFinderSlowTest, FindName)
{
    SymbolFinder finder(1, 5);
    finder.addFile(TESTDATA_DIR, "renamevariable.cpp", "int variable;", {"cc", "renamevariable.cpp"});

    finder.findSymbol();

    ASSERT_THAT(finder.takeSymbolName(), "variable");
}

TEST(SymbolFinderSlowTest, FindNameInUnsavedFile)
{
    SymbolFinder finder(1, 5);
    finder.addFile(TESTDATA_DIR, "renamevariable.cpp", "int newVariable;", {"cc", "renamevariable.cpp"});

    finder.findSymbol();

    ASSERT_THAT(finder.takeSymbolName(), "newVariable");
}

TEST(SymbolFinderSlowTest, FindUsrs)
{
    SymbolFinder finder(1, 5);
    finder.addFile(TESTDATA_DIR, "renamevariable.cpp", "int variable;", {"cc", "renamevariable.cpp", "-std=c++14"});

    finder.findSymbol();

    ASSERT_THAT(finder.unifiedSymbolResolutions().front(), StrEq("c:@variable"));
}

TEST(SymbolFinderSlowTest, VariableDeclarationSourceLocations)
{
    SymbolFinder finder(1, 5);
    finder.addFile(TESTDATA_DIR, "renamevariable.cpp", "", {"cc", "renamevariable.cpp", "-std=c++14"});

    finder.findSymbol();

    ASSERT_THAT(finder.sourceLocations().sourceLocationContainers(),
                AllOf(Contains(IsSourceLocation(1, 5)),
                      Contains(IsSourceLocation(3, 9))));
}

TEST(SymbolFinderSlowTest, VariableUsageSourceLocations)
{
    SymbolFinder finder(3, 9);
    finder.addFile(TESTDATA_DIR, "renamevariable.cpp", "", {"cc", "renamevariable.cpp", "-std=c++14"});

    finder.findSymbol();

    ASSERT_THAT(finder.sourceLocations().sourceLocationContainers(),
                AllOf(Contains(IsSourceLocation(1, 5)),
                      Contains(IsSourceLocation(3, 9))));
}

TEST(SymbolFinderSlowTest, TemplateMemberVariableDeclarationSourceLocations)
{
    SymbolFinder finder(8, 18);
    finder.addFile(TESTDATA_DIR, "renamevariable.cpp", "", {"cc", "renamevariable.cpp", "-std=c++14"});

    finder.findSymbol();

    ASSERT_THAT(finder.sourceLocations().sourceLocationContainers(),
                AllOf(Contains(IsSourceLocation(8, 18)),
                      Contains(IsSourceLocation(15, 14)),
                      Contains(IsSourceLocation(18, 19))));
}

TEST(SymbolFinderSlowTest, TemplateMemberVariableUsageSourceLocations)
{
    SymbolFinder finder(15, 14);
    finder.addFile(TESTDATA_DIR, "renamevariable.cpp", "", {"cc", "renamevariable.cpp", "-std=c++14"});

    finder.findSymbol();

    ASSERT_THAT(finder.sourceLocations().sourceLocationContainers(),
                AllOf(Contains(IsSourceLocation(8, 18)),
                      Contains(IsSourceLocation(15, 14)),
                      Contains(IsSourceLocation(18, 19))));
}

TEST(SymbolFinderSlowTest, TemplateMemberVariableUsageInLambdaSourceLocations)
{
    SymbolFinder finder(18, 19);
    finder.addFile(TESTDATA_DIR, "renamevariable.cpp", "", {"cc", "renamevariable.cpp", "-std=c++14"});

    finder.findSymbol();

    ASSERT_THAT(finder.sourceLocations().sourceLocationContainers(),
                AllOf(Contains(IsSourceLocation(8, 18)),
                      Contains(IsSourceLocation(15, 14)),
                      Contains(IsSourceLocation(18, 19))));
}

TEST(SymbolFinderSlowTest, CursorOverMacroDefintionSymbolName)
{
    SymbolFinder finder(1, 9);
    finder.addFile(TESTDATA_DIR, "symbolfinder_macro.cpp", "", {"cc", "symbolfinder_macro.cpp"});

    finder.findSymbol();

    ASSERT_THAT(finder.takeSymbolName(), "Macro");
}

TEST(SymbolFinderSlowTest, CursorOverMacroExpansionSymbolName)
{
    SymbolFinder finder(10, 10);
    finder.addFile(TESTDATA_DIR, "symbolfinder_macro.cpp", "", {"cc", "symbolfinder_macro.cpp"});

    finder.findSymbol();

    ASSERT_THAT(finder.takeSymbolName(), "Macro");
}

TEST(SymbolFinderSlowTest, FindMacroDefinition)
{
    SymbolFinder finder(1, 9);
    finder.addFile(TESTDATA_DIR, "symbolfinder_macro.cpp", "", {"cc", "symbolfinder_macro.cpp"});

    finder.findSymbol();

    ASSERT_THAT(finder.sourceLocations().sourceLocationContainers(),
                Contains(IsSourceLocation(1, 9)));
}

TEST(SymbolFinderSlowTest, FindMacroExpansion)
{
    SymbolFinder finder(1, 9);
    finder.addFile(TESTDATA_DIR, "symbolfinder_macro.cpp", "", {"cc", "symbolfinder_macro.cpp"});

    finder.findSymbol();

    ASSERT_THAT(finder.sourceLocations().sourceLocationContainers(),
                Contains(IsSourceLocation(5, 17)));
}

TEST(SymbolFinderSlowTest, DoNotFindUndedefinedMacroExpansion)
{
    SymbolFinder finder(1, 9);
    finder.addFile(TESTDATA_DIR, "symbolfinder_macro.cpp", "", {"cc", "symbolfinder_macro.cpp"});

    finder.findSymbol();

    ASSERT_THAT(finder.sourceLocations().sourceLocationContainers(),
                Not(Contains(IsSourceLocation(10, 10))));
}

TEST(SymbolFinderSlowTest, FindMacroDefinitionFromMacroExpansion)
{
    SymbolFinder finder(10, 10);
    finder.addFile(TESTDATA_DIR, "symbolfinder_macro.cpp", "", {"cc", "symbolfinder_macro.cpp"});

    finder.findSymbol();

    ASSERT_THAT(finder.sourceLocations().sourceLocationContainers(),
                Contains(IsSourceLocation(8, 9)));
}


TEST(SymbolFinderSlowTest, FindMacroExpansionBeforeMacroExpansionWithCursor)
{
    SymbolFinder finder(12, 10);
    finder.addFile(TESTDATA_DIR, "symbolfinder_macro.cpp", "", {"cc", "symbolfinder_macro.cpp"});

    finder.findSymbol();

    ASSERT_THAT(finder.sourceLocations().sourceLocationContainers(),
                Contains(IsSourceLocation(10, 10)));
}

TEST(SymbolFinderSlowTest, FindMacroExpansionAfterMacroExpansionWithCursor)
{
    SymbolFinder finder(10, 10);
    finder.addFile(TESTDATA_DIR, "symbolfinder_macro.cpp", "", {"cc", "symbolfinder_macro.cpp"});

    finder.findSymbol();

    ASSERT_THAT(finder.sourceLocations().sourceLocationContainers(),
                Contains(IsSourceLocation(12, 10)));
}

}
