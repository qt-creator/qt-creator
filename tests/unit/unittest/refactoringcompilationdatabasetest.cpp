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

#include <refactoringcompilationdatabase.h>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

using testing::Contains;
using testing::IsEmpty;
using testing::Not;
using testing::PrintToString;

using ClangBackEnd::RefactoringCompilationDatabase;

namespace {

MATCHER_P3(IsCompileCommand, directory, fileName, commandLine,
           std::string(negation ? "isn't" : "is")
           + " compile command with directory "+ PrintToString(directory)
           + ", file name " + PrintToString(fileName)
           + " and command line " + PrintToString(commandLine)
           )
{
    if (arg.Directory != directory
            || arg.Filename != fileName
            || arg.CommandLine != commandLine)
        return false;

    return true;
}

TEST(RefactoringCompilationDatabase, GetAllFilesContainsTranslationUnit)
{
    RefactoringCompilationDatabase database;
    database.addFile("/tmp", "data.cpp", {"cc", "data.cpp", "-DNO_DEBUG"});

    auto filePaths = database.getAllFiles();

    ASSERT_THAT(filePaths, Contains("/tmp/data.cpp"));
}

TEST(RefactoringCompilationDatabase, CompileCommandForFilePath)
{
    RefactoringCompilationDatabase database;
    database.addFile("/tmp", "data.cpp",  {"cc", "data.cpp", "-DNO_DEBUG"});

    auto compileCommands = database.getAllCompileCommands();

    ASSERT_THAT(compileCommands, Contains(IsCompileCommand("/tmp",
                                                           "data.cpp",
                                                          std::vector<std::string>{"cc", "data.cpp", "-DNO_DEBUG"})));
}

TEST(RefactoringCompilationDatabase, NoCompileCommandForFilePath)
{
    RefactoringCompilationDatabase database;
    database.addFile("/tmp", "data.cpp",  {"cc", "data.cpp", "-DNO_DEBUG"});

    auto compileCommands = database.getAllCompileCommands();

    ASSERT_THAT(compileCommands, Not(Contains(IsCompileCommand("/tmp",
                                                               "data.cpp2",
                                                               std::vector<std::string>{"cc", "data.cpp", "-DNO_DEBUG"}))));
}

TEST(RefactoringCompilationDatabase, FilePaths)
{
    RefactoringCompilationDatabase database;
    database.addFile("/tmp", "data.cpp", {"cc", "data.cpp", "-DNO_DEBUG"});

    auto filePaths = database.getAllFiles();

    ASSERT_THAT(filePaths, Contains("/tmp/data.cpp"));
}

}
