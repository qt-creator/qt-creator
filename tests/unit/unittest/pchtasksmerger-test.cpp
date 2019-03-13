/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "mockpchtaskqueue.h"

#include <pchtasksmerger.h>

namespace {

using ClangBackEnd::CompilerMacro;
using ClangBackEnd::CompilerMacros;
using ClangBackEnd::IncludeSearchPath;
using ClangBackEnd::IncludeSearchPathType;
using ClangBackEnd::PchTask;
using ClangBackEnd::PchTaskSet;
using Merger = ClangBackEnd::PchTasksMerger;
using Id = ClangBackEnd::FilePathId;
class PchTasksMerger : public testing::Test
{
protected:
    PchTask clone(PchTask entry) const
    {
        // entry.toolChainArguments = toolChainArguments;

        return entry;
    }

protected:
    NiceMock<MockPchTaskQueue> mockPchTaskQueue;
    ClangBackEnd::PchTasksMerger merger{mockPchTaskQueue};
    PchTask systemTask1{1,
                        {1, 2},
                        {1, 2, 3},
                        {{"YI", "1", 1}, {"SAN", "3", 3}},
                        {"YI", "LIANG"},
                        {"--yi"},
                        {{"/system/path", 2, IncludeSearchPathType::System},
                         {"/builtin/path2", 3, IncludeSearchPathType::BuiltIn},
                         {"/framework/path", 1, IncludeSearchPathType::System}},
                        {{"/to/path1", 1, IncludeSearchPathType::User},
                         {"/to/path2", 2, IncludeSearchPathType::User}}};
    PchTask projectTask1{1,
                         {11, 12},
                         {11, 12},
                         {{"SE", "4", 4}, {"WU", "5", 5}},
                         {"ER", "SAN"},
                         {"--yi"},
                         {{"/system/path", 1, IncludeSearchPathType::System},
                          {"/builtin/path", 2, IncludeSearchPathType::BuiltIn}},
                         {{"/to/path1", 1, IncludeSearchPathType::User},
                          {"/to/path2", 2, IncludeSearchPathType::User}}};
    PchTask systemTask2{2,
                        {11, 12},
                        {11, 12, 13},
                        {{"SE", "4", 4}, {"WU", "5", 5}},
                        {"ER", "SAN"},
                        {"--yi"},
                        {{"/system/path", 2, IncludeSearchPathType::System},
                         {"/builtin/path", 3, IncludeSearchPathType::BuiltIn},
                         {"/framework/path", 1, IncludeSearchPathType::System}},
                        {{"/to/path1", 1, IncludeSearchPathType::User},
                         {"/to/path2", 2, IncludeSearchPathType::User}}};
    PchTask projectTask2{2,
                         {11, 12},
                         {11, 12},
                         {{"SE", "4", 4}, {"WU", "5", 5}},
                         {"ER", "SAN"},
                         {"--yi"},
                         {{"/system/path", 2, IncludeSearchPathType::System},
                          {"/builtin/path", 3, IncludeSearchPathType::BuiltIn},
                          {"/framework/path", 1, IncludeSearchPathType::System}},
                         {{"/to/path1", 1, IncludeSearchPathType::User},
                          {"/to/path2", 2, IncludeSearchPathType::User}}};
    PchTask systemTask3{3,
                        {1, 2},
                        {1, 2},
                        {{"YI", "2", 1}, {"SAN", "3", 3}},
                        {"YI", "LIANG"},
                        {"--yi"},
                        {{"/system/path", 2, IncludeSearchPathType::System},
                         {"/builtin/path", 3, IncludeSearchPathType::BuiltIn},
                         {"/framework/path", 1, IncludeSearchPathType::System}},
                        {{"/to/path1", 1, IncludeSearchPathType::User},
                         {"/to/path2", 2, IncludeSearchPathType::User}}};
    Utils::SmallStringVector toolChainArguments = {"toolChainArguments"};
};

TEST_F(PchTasksMerger, AddProjectTasks)
{
    InSequence s;

    EXPECT_CALL(mockPchTaskQueue, addProjectPchTasks(ElementsAre(projectTask1, projectTask2)));
    EXPECT_CALL(mockPchTaskQueue, processEntries());

    merger.mergeTasks({{clone(systemTask1), clone(projectTask1)},
                       {clone(systemTask1), clone(projectTask2)}},
                      std::move(toolChainArguments));
}

TEST_F(PchTasksMerger, AddSystemTasks)
{
    InSequence s;

    EXPECT_CALL(mockPchTaskQueue, addSystemPchTasks(ElementsAre(_, systemTask3)));
    EXPECT_CALL(mockPchTaskQueue, processEntries());

    merger.mergeTasks({{clone(systemTask1), clone(projectTask1)},
                       {clone(systemTask2), clone(projectTask2)},
                       {clone(systemTask3), clone(projectTask2)}},
                      std::move(toolChainArguments));
}

TEST_F(PchTasksMerger, AddSystemOnlyOneTask)
{
    InSequence s;

    EXPECT_CALL(mockPchTaskQueue, addSystemPchTasks(ElementsAre(systemTask1)));
    EXPECT_CALL(mockPchTaskQueue, processEntries());

    merger.mergeTasks({{clone(systemTask1), clone(projectTask1)}},
                      std::move(toolChainArguments));
}

TEST_F(PchTasksMerger, RemoveTasks)
{
    EXPECT_CALL(mockPchTaskQueue, removePchTasks(ElementsAre(1, 2)));

    merger.removePchTasks({1, 2});
}

TEST_F(PchTasksMerger, MergeMacros)
{
    CompilerMacros compilerMacros1{{"ER", "2", 2}, {"SE", "4", 1}, {"YI"}, {"SAN", "3", 3}};
    CompilerMacros compilerMacros2{{"BA"}, {"ER", "2", 2}, {"YI", "1", 1}, {"SAN", "3", 3}};

    auto macros = Merger::mergeMacros(compilerMacros1, compilerMacros2);

    ASSERT_THAT(macros,
                ElementsAre(CompilerMacro{"BA"},
                            CompilerMacro{"ER", "2", 2},
                            CompilerMacro{"SE", "4", 1},
                            CompilerMacro{"YI", "1", 1},
                            CompilerMacro{"YI"},
                            CompilerMacro{"SAN", "3", 3}));
}

TEST_F(PchTasksMerger, MacrosCanBeMerged)
{
    CompilerMacros compilerMacros1{{"ER", "2", 2}, {"QI"}, {"SE", "4", 1}, {"SAN", "3", 3}};
    CompilerMacros compilerMacros2{{"BA"}, {"ER", "2", 2}, {"YI", "1", 1}, {"SAN", "3", 3}};

    auto canBeMerged = !Merger::hasDuplicates(Merger::mergeMacros(compilerMacros1, compilerMacros2));

    ASSERT_TRUE(canBeMerged);
}

TEST_F(PchTasksMerger, MacrosCannotBeMergedBecauseDifferentValue)
{
    CompilerMacros compilerMacros1{{"ER", "2", 2}, {"SE", "4", 1}, {"SAN", "3", 3}};
    CompilerMacros compilerMacros2{{"ER", "1", 2}, {"YI", "1", 1}, {"SAN", "3", 3}};

    auto canBeMerged = !Merger::hasDuplicates(Merger::mergeMacros(compilerMacros1, compilerMacros2));

    ASSERT_FALSE(canBeMerged);
}

TEST_F(PchTasksMerger, MacrosCannotBeMergedBecauseUndefinedMacro)
{
    CompilerMacros compilerMacros1{{"ER", "2", 2}, {"SE", "4", 1}, {"YI"}, {"SAN", "3", 3}};
    CompilerMacros compilerMacros2{{"ER", "2", 2}, {"YI", "1", 1}, {"SAN", "3", 3}};

    auto canBeMerged = !Merger::hasDuplicates(Merger::mergeMacros(compilerMacros1, compilerMacros2));

    ASSERT_FALSE(canBeMerged);
}

TEST_F(PchTasksMerger, CanMergePchTasks)
{
    auto canBeMerged = Merger::mergePchTasks(systemTask1, systemTask2);

    ASSERT_TRUE(canBeMerged);
}

TEST_F(PchTasksMerger, CannotMergePchTasks)
{
    auto canBeMerged = Merger::mergePchTasks(systemTask1, systemTask3);

    ASSERT_FALSE(canBeMerged);
}

TEST_F(PchTasksMerger, IsMergedIsSet)
{
    Merger::mergePchTasks(systemTask1, systemTask2);

    ASSERT_TRUE(systemTask2.isMerged);
}

TEST_F(PchTasksMerger, IsMergedIsNotSet)
{
    Merger::mergePchTasks(systemTask1, systemTask3);

    ASSERT_FALSE(systemTask3.isMerged);
}

TEST_F(PchTasksMerger, DontMergeLanguage)
{
    systemTask2.language = Utils::Language::C;

    Merger::mergePchTasks(systemTask1, systemTask2);

    ASSERT_FALSE(systemTask2.isMerged);
}

TEST_F(PchTasksMerger, MergeCompilerMacros)
{
    Merger::mergePchTasks(systemTask1, systemTask2);

    ASSERT_THAT(systemTask1.compilerMacros,
                ElementsAre(CompilerMacro{"SE", "4", 4},
                            CompilerMacro{"WU", "5", 5},
                            CompilerMacro{"YI", "1", 1},
                            CompilerMacro{"SAN", "3", 3}));
}

TEST_F(PchTasksMerger, DontMergeCompilerMacros)
{
    Merger::mergePchTasks(systemTask1, systemTask3);

    ASSERT_THAT(systemTask1.compilerMacros,
                ElementsAre(CompilerMacro{"YI", "1", 1}, CompilerMacro{"SAN", "3", 3}));
}

TEST_F(PchTasksMerger, MergeIncludes)
{
    Merger::mergePchTasks(systemTask1, systemTask2);

    ASSERT_THAT(systemTask1.includes, ElementsAre(1, 2, 11, 12));
}

TEST_F(PchTasksMerger, DontMergeIncludes)
{
    Merger::mergePchTasks(systemTask1, systemTask3);

    ASSERT_THAT(systemTask1.includes, ElementsAre(1, 2));
}

TEST_F(PchTasksMerger, MergeAllIncludes)
{
    Merger::mergePchTasks(systemTask1, systemTask2);

    ASSERT_THAT(systemTask1.sources, ElementsAre(1, 2, 3, 11, 12, 13));
}

TEST_F(PchTasksMerger, DontAllMergeIncludes)
{
    Merger::mergePchTasks(systemTask1, systemTask3);

    ASSERT_THAT(systemTask1.sources, ElementsAre(1, 2, 3));
}

TEST_F(PchTasksMerger, MergeProjectIds)
{
    Merger::mergePchTasks(systemTask1, systemTask2);

    ASSERT_THAT(systemTask1.projectPartIds, ElementsAre(1, 2));
}

TEST_F(PchTasksMerger, DontMergeProjectIds)
{
    Merger::mergePchTasks(systemTask1, systemTask3);

    ASSERT_THAT(systemTask1.projectPartIds, ElementsAre(1));
}

TEST_F(PchTasksMerger, MergeIncludeSearchPaths)
{
    Merger::mergePchTasks(systemTask1, systemTask2);

    ASSERT_THAT(systemTask1.systemIncludeSearchPaths,
                ElementsAre(IncludeSearchPath{"/system/path", 2, IncludeSearchPathType::System},
                            IncludeSearchPath{"/builtin/path", 3, IncludeSearchPathType::BuiltIn},
                            IncludeSearchPath{"/builtin/path2", 3, IncludeSearchPathType::BuiltIn},
                            IncludeSearchPath{"/framework/path", 1, IncludeSearchPathType::System}));
}

TEST_F(PchTasksMerger, DontMergeIncludeSearchPaths)
{
    Merger::mergePchTasks(systemTask1, systemTask3);

    ASSERT_THAT(systemTask1.systemIncludeSearchPaths,
                ElementsAre(IncludeSearchPath{"/system/path", 2, IncludeSearchPathType::System},
                            IncludeSearchPath{"/builtin/path2", 3, IncludeSearchPathType::BuiltIn},
                            IncludeSearchPath{"/framework/path", 1, IncludeSearchPathType::System}));
}

TEST_F(PchTasksMerger, ChooseLanguageVersionFromFirstTask)
{
    systemTask2.languageVersion = Utils::LanguageVersion::CXX17;

    Merger::mergePchTasks(systemTask1, systemTask2);

    ASSERT_THAT(systemTask1.languageVersion, Utils::LanguageVersion::CXX17);
}

TEST_F(PchTasksMerger, ChooseLanguageVersionFromSecondTask)
{
    systemTask1.languageVersion = Utils::LanguageVersion::CXX17;

    Merger::mergePchTasks(systemTask1, systemTask2);

    ASSERT_THAT(systemTask1.languageVersion, Utils::LanguageVersion::CXX17);
}

TEST_F(PchTasksMerger, DontChooseLanguageVersion)
{
    systemTask3.languageVersion = Utils::LanguageVersion::CXX17;

    Merger::mergePchTasks(systemTask1, systemTask3);

    ASSERT_THAT(systemTask1.languageVersion, Utils::LanguageVersion::CXX98);
}

TEST_F(PchTasksMerger, FirstTaskIsNotMergedAgain)
{
    systemTask1.isMerged = true;

    Merger::mergePchTasks(systemTask1, systemTask2);

    ASSERT_FALSE(systemTask2.isMerged);
}

TEST_F(PchTasksMerger, DontMergeAlreadyMergedFirstTask)
{
    systemTask1.isMerged = true;

    bool isMerged = Merger::mergePchTasks(systemTask1, systemTask2);

    ASSERT_FALSE(isMerged);
}

TEST_F(PchTasksMerger, SecondTaskIsNotMergedAgain)
{
    systemTask2.isMerged = true;

    Merger::mergePchTasks(systemTask1, systemTask2);

    ASSERT_THAT(systemTask1.includes, ElementsAre(1, 2));
}

TEST_F(PchTasksMerger, DontMergeAlreadyMergedSecondTask)
{
    systemTask2.isMerged = true;

    bool isMerged = Merger::mergePchTasks(systemTask1, systemTask2);

    ASSERT_FALSE(isMerged);
}

} // namespace
