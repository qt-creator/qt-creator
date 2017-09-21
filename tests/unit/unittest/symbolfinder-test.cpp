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
#include "mockfilepathcaching.h"

#include <clangrefactoringbackend_global.h>
#include <symbolfinder.h>

#include "filesystem-utilities.h"

using ClangBackEnd::SymbolFinder;
using ClangBackEnd::FileContent;

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

using Finder = SymbolFinder;

class SymbolFinder : public testing::Test
{
protected:
    NiceMock<MockFilePathCaching> filePathCaching;
};

using SymbolFinderSlowTest = SymbolFinder;

TEST_F(SymbolFinder, FileContentFilePath)
{
    FileContent fileContent(toNativePath("/tmp"), "data.cpp", "int variable;", {"cc", "data.cpp"});

    ASSERT_THAT(fileContent.filePath, toNativePath("/tmp/data.cpp"));
}

TEST_F(SymbolFinderSlowTest, FindName)
{
    Finder finder(1, 5, filePathCaching);
    finder.addFile(TESTDATA_DIR, "renamevariable.cpp", "int variable;", {"cc", "renamevariable.cpp"});

    finder.findSymbol();

    ASSERT_THAT(finder.takeSymbolName(), "variable");
}

TEST_F(SymbolFinderSlowTest, FindNameInUnsavedFile)
{
    Finder finder(1, 5, filePathCaching);
    finder.addFile(TESTDATA_DIR, "renamevariable.cpp", "int newVariable;", {"cc", "renamevariable.cpp"});

    finder.findSymbol();

    ASSERT_THAT(finder.takeSymbolName(), "newVariable");
}

TEST_F(SymbolFinderSlowTest, FindUsrs)
{
    Finder finder(1, 5, filePathCaching);
    finder.addFile(TESTDATA_DIR, "renamevariable.cpp", "int variable;", {"cc", "renamevariable.cpp", "-std=c++14"});

    finder.findSymbol();

    ASSERT_THAT(finder.unifiedSymbolResolutions().front(), StrEq("c:@variable"));
}

TEST_F(SymbolFinderSlowTest, VariableDeclarationSourceLocations)
{
    Finder finder(1, 5, filePathCaching);
    finder.addFile(TESTDATA_DIR, "renamevariable.cpp", "", {"cc", "renamevariable.cpp", "-std=c++14"});

    finder.findSymbol();

    ASSERT_THAT(finder.sourceLocations().sourceLocationContainers(),
                AllOf(Contains(IsSourceLocation(1, 5)),
                      Contains(IsSourceLocation(3, 9))));
}

TEST_F(SymbolFinderSlowTest, VariableUsageSourceLocations)
{
    Finder finder(3, 9, filePathCaching);
    finder.addFile(TESTDATA_DIR, "renamevariable.cpp", "", {"cc", "renamevariable.cpp", "-std=c++14"});

    finder.findSymbol();

    ASSERT_THAT(finder.sourceLocations().sourceLocationContainers(),
                AllOf(Contains(IsSourceLocation(1, 5)),
                      Contains(IsSourceLocation(3, 9))));
}

TEST_F(SymbolFinderSlowTest, TemplateMemberVariableDeclarationSourceLocations)
{
    Finder finder(8, 18, filePathCaching);
    finder.addFile(TESTDATA_DIR, "renamevariable.cpp", "", {"cc", "renamevariable.cpp", "-std=c++14"});

    finder.findSymbol();

    ASSERT_THAT(finder.sourceLocations().sourceLocationContainers(),
                AllOf(Contains(IsSourceLocation(8, 18)),
                      Contains(IsSourceLocation(15, 14)),
                      Contains(IsSourceLocation(18, 19))));
}

TEST_F(SymbolFinderSlowTest, TemplateMemberVariableUsageSourceLocations)
{
    Finder finder(15, 14, filePathCaching);
    finder.addFile(TESTDATA_DIR, "renamevariable.cpp", "", {"cc", "renamevariable.cpp", "-std=c++14"});

    finder.findSymbol();

    ASSERT_THAT(finder.sourceLocations().sourceLocationContainers(),
                AllOf(Contains(IsSourceLocation(8, 18)),
                      Contains(IsSourceLocation(15, 14)),
                      Contains(IsSourceLocation(18, 19))));
}

TEST_F(SymbolFinderSlowTest, TemplateMemberVariableUsageInLambdaSourceLocations)
{
    Finder finder(18, 19, filePathCaching);
    finder.addFile(TESTDATA_DIR, "renamevariable.cpp", "", {"cc", "renamevariable.cpp", "-std=c++14"});

    finder.findSymbol();

    ASSERT_THAT(finder.sourceLocations().sourceLocationContainers(),
                AllOf(Contains(IsSourceLocation(8, 18)),
                      Contains(IsSourceLocation(15, 14)),
                      Contains(IsSourceLocation(18, 19))));
}

TEST_F(SymbolFinderSlowTest, CursorOverMacroDefintionSymbolName)
{
    Finder finder(1, 9, filePathCaching);
    finder.addFile(TESTDATA_DIR, "symbolfinder_macro.cpp", "", {"cc", "symbolfinder_macro.cpp"});

    finder.findSymbol();

    ASSERT_THAT(finder.takeSymbolName(), "Macro");
}

TEST_F(SymbolFinderSlowTest, CursorOverMacroExpansionSymbolName)
{
    Finder finder(10, 10, filePathCaching);
    finder.addFile(TESTDATA_DIR, "symbolfinder_macro.cpp", "", {"cc", "symbolfinder_macro.cpp"});

    finder.findSymbol();

    ASSERT_THAT(finder.takeSymbolName(), "Macro");
}

TEST_F(SymbolFinderSlowTest, FindMacroDefinition)
{
    Finder finder(1, 9, filePathCaching);
    finder.addFile(TESTDATA_DIR, "symbolfinder_macro.cpp", "", {"cc", "symbolfinder_macro.cpp"});

    finder.findSymbol();

    ASSERT_THAT(finder.sourceLocations().sourceLocationContainers(),
                Contains(IsSourceLocation(1, 9)));
}

TEST_F(SymbolFinderSlowTest, FindMacroExpansion)
{
    Finder finder(1, 9, filePathCaching);
    finder.addFile(TESTDATA_DIR, "symbolfinder_macro.cpp", "", {"cc", "symbolfinder_macro.cpp"});

    finder.findSymbol();

    ASSERT_THAT(finder.sourceLocations().sourceLocationContainers(),
                Contains(IsSourceLocation(5, 17)));
}

TEST_F(SymbolFinderSlowTest, DoNotFindUndedefinedMacroExpansion)
{
    Finder finder(1, 9, filePathCaching);
    finder.addFile(TESTDATA_DIR, "symbolfinder_macro.cpp", "", {"cc", "symbolfinder_macro.cpp"});

    finder.findSymbol();

    ASSERT_THAT(finder.sourceLocations().sourceLocationContainers(),
                Not(Contains(IsSourceLocation(10, 10))));
}

TEST_F(SymbolFinderSlowTest, FindMacroDefinitionFromMacroExpansion)
{
    Finder finder(10, 10, filePathCaching);
    finder.addFile(TESTDATA_DIR, "symbolfinder_macro.cpp", "", {"cc", "symbolfinder_macro.cpp"});

    finder.findSymbol();

    ASSERT_THAT(finder.sourceLocations().sourceLocationContainers(),
                Contains(IsSourceLocation(8, 9)));
}


TEST_F(SymbolFinderSlowTest, FindMacroExpansionBeforeMacroExpansionWithCursor)
{
    Finder finder(12, 10, filePathCaching);
    finder.addFile(TESTDATA_DIR, "symbolfinder_macro.cpp", "", {"cc", "symbolfinder_macro.cpp"});

    finder.findSymbol();

    ASSERT_THAT(finder.sourceLocations().sourceLocationContainers(),
                Contains(IsSourceLocation(10, 10)));
}

TEST_F(SymbolFinderSlowTest, FindMacroExpansionAfterMacroExpansionWithCursor)
{
    Finder finder(10, 10, filePathCaching);
    finder.addFile(TESTDATA_DIR, "symbolfinder_macro.cpp", "", {"cc", "symbolfinder_macro.cpp"});

    finder.findSymbol();

    ASSERT_THAT(finder.sourceLocations().sourceLocationContainers(),
                Contains(IsSourceLocation(12, 10)));
}

}
