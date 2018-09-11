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

#include "fakeprocess.h"

#include "mockclangpathwatcher.h"
#include "mockpchmanagerclient.h"
#include "testenvironment.h"

#include <refactoringdatabaseinitializer.h>
#include <filepathcaching.h>
#include <generatedfiles.h>
#include <pchcreator.h>
#include <precompiledheadersupdatedmessage.h>

#include <sqlitedatabase.h>

#include <QFileInfo>

namespace {

using ClangBackEnd::FilePathId;
using ClangBackEnd::GeneratedFiles;
using ClangBackEnd::IdPaths;
using ClangBackEnd::ProjectPartPch;
using ClangBackEnd::V2::ProjectPartContainer;
using ClangBackEnd::V2::FileContainer;
using ClangBackEnd::FilePath;
using ClangBackEnd::FilePathIds;
using ClangBackEnd::FilePathView;

using Utils::PathString;
using Utils::SmallString;

using UnitTests::EndsWith;

class PchCreator: public ::testing::Test
{
protected:
    void SetUp()
    {
        creator.setUnsavedFiles({generatedFile});
    }

    ClangBackEnd::FilePathId id(ClangBackEnd::FilePathView path)
    {
        return creator.filePathCache().filePathId(path);
    }

protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> databaseInitializer{database};
    FilePath main1Path = TESTDATA_DIR "/includecollector_main3.cpp";
    FilePath main2Path = TESTDATA_DIR "/includecollector_main2.cpp";
    FilePath header1Path = TESTDATA_DIR "/includecollector_header1.h";
    FilePath header2Path = TESTDATA_DIR "/includecollector_header2.h";
    Utils::SmallStringView generatedFileName = "includecollector_generated_file.h";
    FilePath generatedFilePath = TESTDATA_DIR "/includecollector_generated_file.h";
    TestEnvironment environment;
    FileContainer generatedFile{{TESTDATA_DIR, generatedFileName}, "#pragma once", {}};
    NiceMock<MockPchManagerClient> mockPchManagerClient;
    NiceMock<MockClangPathWatcher> mockClangPathWatcher;
    ClangBackEnd::PchCreator creator{environment, database, mockPchManagerClient, mockClangPathWatcher};
    ProjectPartContainer projectPart1{"project1",
                                      {"-I", TESTDATA_DIR, "-Wno-pragma-once-outside-header"},
                                      {{"DEFINE", "1"}},
                                      {"/includes"},
                                      {id(header1Path)},
                                      {id(main1Path)}};
    ProjectPartContainer projectPart2{"project2",
                                      {"-I", TESTDATA_DIR, "-x", "c++-header", "-Wno-pragma-once-outside-header"},
                                      {{"DEFINE", "1"}},
                                      {"/includes"},
                                      {id(header2Path)},
                                      {id(main2Path)}};
};
using PchCreatorSlowTest = PchCreator;
using PchCreatorVerySlowTest = PchCreator;

TEST_F(PchCreator, ConvertToQStringList)
{
    auto arguments = creator.convertToQStringList({"-I", TESTDATA_DIR});

    ASSERT_THAT(arguments, ElementsAre(QString("-I"), QString(TESTDATA_DIR)));
}

TEST_F(PchCreator, CreateProjectPartCommandLine)
{
    auto commandLine = creator.generateProjectPartCommandLine(projectPart1);

    ASSERT_THAT(commandLine, ElementsAre(environment.clangCompilerPath(), "-I", TESTDATA_DIR, "-Wno-pragma-once-outside-header"));
}

TEST_F(PchCreator, CreateProjectPartHeaders)
{
    auto includeIds = creator.generateProjectPartHeaders(projectPart1);

    ASSERT_THAT(includeIds, UnorderedElementsAre(header1Path, generatedFilePath));
}

TEST_F(PchCreator, CreateProjectPartSources)
{
    auto includeIds = creator.generateProjectPartSourcePaths(projectPart1);

    ASSERT_THAT(includeIds, UnorderedElementsAre(main1Path));
}

TEST_F(PchCreatorSlowTest, CreateProjectPartPchIncludes)
{
    using IncludePair = decltype(creator.generateProjectPartPchIncludes(projectPart1));

    auto includeIds = creator.generateProjectPartPchIncludes(projectPart1);

    ASSERT_THAT(includeIds,
                AllOf(
                    Field(&IncludePair::first,
                          AllOf(Contains(id(TESTDATA_DIR "/includecollector_external1.h")),
                                Contains(id(TESTDATA_DIR "/includecollector_external2.h")),
                                Contains(id(TESTDATA_DIR "/includecollector_header2.h")))),
                    Field(&IncludePair::second,
                          AllOf(Contains(id(TESTDATA_DIR "/includecollector_external1.h")),
                                Contains(id(TESTDATA_DIR "/includecollector_external2.h"))))));
}

TEST_F(PchCreatorSlowTest, CreateProjectPartPchFileContent)
{
    FilePathIds topExternalIncludes;
    std::tie(std::ignore, topExternalIncludes) = creator.generateProjectPartPchIncludes(projectPart1);

    auto content = creator.generatePchIncludeFileContent(topExternalIncludes);

    ASSERT_THAT(std::string(content),
                AllOf(HasSubstr("#include \"" TESTDATA_DIR "/includecollector_header2.h\"\n"),
                      HasSubstr("#include \"" TESTDATA_DIR "/includecollector_external1.h\"\n"),
                      HasSubstr("#include \"" TESTDATA_DIR "/includecollector_external2.h\"\n")));
}

TEST_F(PchCreatorSlowTest, CreateProjectPartPchIncludeFile)
{
    FilePathIds topExternalIncludes;
    std::tie(std::ignore, topExternalIncludes) = creator.generateProjectPartPchIncludes(projectPart1);
    auto content = creator.generatePchIncludeFileContent(topExternalIncludes);
    auto pchIncludeFilePath = creator.generateProjectPathPchHeaderFilePath(projectPart1);
    auto file = creator.generateFileWithContent(pchIncludeFilePath, content);
    file->open(QIODevice::ReadOnly);

    auto fileContent = file->readAll();

    ASSERT_THAT(fileContent.toStdString(),
                AllOf(HasSubstr("#include \"" TESTDATA_DIR "/includecollector_header2.h\"\n"),
                      HasSubstr("#include \"" TESTDATA_DIR "/includecollector_external1.h\"\n"),
                      HasSubstr("#include \"" TESTDATA_DIR "/includecollector_external2.h\"\n")));
}

TEST_F(PchCreator, CreateProjectPartPchCompilerArguments)
{
    auto arguments = creator.generateProjectPartPchCompilerArguments(projectPart1);

    ASSERT_THAT(arguments, AllOf(Contains("-x"),
                                 Contains("c++-header"),
                                 Contains("-Xclang"),
                                 Contains("-emit-pch"),
                                 Contains("-o"),
                                 Contains(EndsWith(".pch"))));
}

TEST_F(PchCreator, CreateProjectPartClangCompilerArguments)
{
    auto arguments = creator.generateProjectPartClangCompilerArguments(projectPart1);

    ASSERT_THAT(arguments, AllOf(Contains(TESTDATA_DIR),
                                 Contains("-emit-pch"),
                                 Contains("-o"),
                                 Not(Contains(environment.clangCompilerPath()))));
}

TEST_F(PchCreatorVerySlowTest, IncludesForCreatePchsForProjectParts)
{
    creator.generatePch(projectPart1);

    ASSERT_THAT(creator.takeProjectIncludes().id, "project1");
}

TEST_F(PchCreatorVerySlowTest, ProjectPartPchsSendToPchManagerClient)
{
    creator.generatePch(projectPart1);

    EXPECT_CALL(mockPchManagerClient,
                precompiledHeadersUpdated(
                    Field(&ClangBackEnd::PrecompiledHeadersUpdatedMessage::projectPartPchs,
                          ElementsAre(Eq(creator.projectPartPch())))));

    creator.doInMainThreadAfterFinished();
}

TEST_F(PchCreatorVerySlowTest, UpdateFileWatcherIncludes)
{
    creator.generatePch(projectPart1);

    EXPECT_CALL(mockClangPathWatcher,
                updateIdPaths(ElementsAre(creator.projectIncludes())));

    creator.doInMainThreadAfterFinished();
}

TEST_F(PchCreatorVerySlowTest, IdPathsForCreatePchsForProjectParts)
{
    creator.generatePch(projectPart1);

    ASSERT_THAT(creator.takeProjectIncludes(),
                AllOf(Field(&IdPaths::id, "project1"),
                      Field(&IdPaths::filePathIds, AllOf(Contains(id(TESTDATA_DIR "/includecollector_header2.h")),
                                                         Contains(id(TESTDATA_DIR "/includecollector_external1.h")),
                                                         Contains(id(TESTDATA_DIR "/includecollector_external2.h"))))));
}

TEST_F(PchCreatorVerySlowTest, ProjectPartPchForCreatesPchForProjectPart)
{
    creator.generatePch(projectPart1);

    ASSERT_THAT(creator.projectPartPch(),
                AllOf(Field(&ProjectPartPch::projectPartId, Eq("project1")),
                      Field(&ProjectPartPch::pchPath, Not(IsEmpty())),
                      Field(&ProjectPartPch::lastModified, Not(Eq(-1)))));
}

TEST_F(PchCreatorVerySlowTest, ProjectPartPchCleared)
{
    creator.generatePch(projectPart1);

    creator.clear();

    ASSERT_THAT(creator.projectPartPch(), ClangBackEnd::ProjectPartPch{});
}


TEST_F(PchCreatorVerySlowTest, ProjectIncludesCleared)
{
    creator.generatePch(projectPart1);

    creator.clear();

    ASSERT_THAT(creator.projectIncludes(), ClangBackEnd::IdPaths{});
}

TEST_F(PchCreatorVerySlowTest, FaultyProjectPartPchForCreatesNoPchForProjectPart)
{
    ProjectPartContainer faultyProjectPart{"faultyProject",
                                           {"-I", TESTDATA_DIR},
                                           {{"DEFINE", "1"}},
                                           {"/includes"},
                                           {},
                                           {id(TESTDATA_DIR "/includecollector_faulty.cpp")}};

    creator.generatePch(faultyProjectPart);

    ASSERT_THAT(creator.projectPartPch(),
                AllOf(Field(&ProjectPartPch::projectPartId, Eq("faultyProject")),
                      Field(&ProjectPartPch::pchPath, IsEmpty()),
                      Field(&ProjectPartPch::lastModified, Eq(-1))));
}

TEST_F(PchCreator, CreateProjectPartSourcesContent)
{
    auto content = creator.generateProjectPartSourcesContent(projectPart1);

    ASSERT_THAT(content, Eq("#include \"" TESTDATA_DIR "/includecollector_main3.cpp\"\n"));
}

TEST_F(PchCreator, Call)
{
    auto content = creator.generateProjectPartSourcesContent(projectPart1);

    ASSERT_THAT(content, Eq("#include \"" TESTDATA_DIR "/includecollector_main3.cpp\"\n"));
}


}
