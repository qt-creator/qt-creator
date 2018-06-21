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

#include "mockpchgeneratornotifier.h"
#include "testenvironment.h"

#include <refactoringdatabaseinitializer.h>
#include <filepathcaching.h>
#include <pchcreator.h>
#include <pchgenerator.h>

#include <sqlitedatabase.h>

#include <QFileInfo>

namespace {

using ClangBackEnd::FilePathId;
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
    ClangBackEnd::FilePathId id(ClangBackEnd::FilePathView path)
    {
        return filePathCache.filePathId(path);
    }

protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> databaseInitializer{database};
    ClangBackEnd::FilePathCaching filePathCache{database};
    FilePath main1Path = TESTDATA_DIR "/includecollector_main3.cpp";
    FilePath main2Path = TESTDATA_DIR "/includecollector_main2.cpp";
    FilePath header1Path = TESTDATA_DIR "/includecollector_header1.h";
    FilePath header2Path = TESTDATA_DIR "/includecollector_header2.h";
    Utils::SmallStringView generatedFileName = "includecollector_generated_file.h";
    FilePath generatedFilePath = TESTDATA_DIR "/includecollector_generated_file.h";
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
    TestEnvironment environment;
    FileContainer generatedFile{{TESTDATA_DIR, generatedFileName}, "#pragma once", {}};
    NiceMock<MockPchGeneratorNotifier> mockPchGeneratorNotifier;
    ClangBackEnd::PchGenerator<FakeProcess> generator{environment, &mockPchGeneratorNotifier};
    ClangBackEnd::PchCreator creator{{projectPart1.clone(),projectPart2.clone()},
                                     environment,
                                     filePathCache,
                                     &generator,
                                     {generatedFile}};
};
using PchCreatorSlowTest = PchCreator;
using PchCreatorVerySlowTest = PchCreator;

TEST_F(PchCreator, CreateGlobalHeaderPaths)
{
    auto filePaths = creator.generateGlobalHeaderPaths();

    ASSERT_THAT(filePaths,
                UnorderedElementsAre(header1Path, header2Path, generatedFilePath));
}

TEST_F(PchCreator, CreateGlobalSourcePaths)
{
    auto filePaths = creator.generateGlobalSourcePaths();

    ASSERT_THAT(filePaths,
                UnorderedElementsAre(main1Path, main2Path));
}

TEST_F(PchCreator, CreateGlobalHeaderAndSourcePaths)
{
    auto filePaths = creator.generateGlobalHeaderAndSourcePaths();

    ASSERT_THAT(filePaths,
                UnorderedElementsAre(main1Path, main2Path, header1Path, header2Path, generatedFilePath));
}

TEST_F(PchCreator, CreateGlobalArguments)
{
    auto arguments = creator.generateGlobalArguments();

    ASSERT_THAT(arguments, ElementsAre("-I", TESTDATA_DIR, "-Wno-pragma-once-outside-header", "-I", TESTDATA_DIR, "-x" , "c++-header", "-Wno-pragma-once-outside-header"));
}

TEST_F(PchCreator, CreateGlobalCommandLine)
{
    auto arguments = creator.generateGlobalCommandLine();

    ASSERT_THAT(arguments, ElementsAre(environment.clangCompilerPath(), "-I", TESTDATA_DIR, "-Wno-pragma-once-outside-header", "-I", TESTDATA_DIR, "-x" , "c++-header", "-Wno-pragma-once-outside-header"));
}

TEST_F(PchCreatorVerySlowTest, CreateGlobalPchIncludes)
{
    auto includeIds = creator.generateGlobalPchIncludeIds();

    ASSERT_THAT(includeIds,
                AllOf(Contains(id(TESTDATA_DIR "/includecollector_external3.h")),
                      Contains(id(TESTDATA_DIR "/includecollector_external1.h")),
                      Contains(id(TESTDATA_DIR "/includecollector_external2.h"))));
}

TEST_F(PchCreatorVerySlowTest, CreateGlobalPchFileContent)
{
    auto content = creator.generateGlobalPchHeaderFileContent();

    ASSERT_THAT(std::string(content),
                AllOf(HasSubstr("#include \"" TESTDATA_DIR "/includecollector_external3.h\"\n"),
                      HasSubstr("#include \"" TESTDATA_DIR "/includecollector_external1.h\"\n"),
                      HasSubstr("#include \"" TESTDATA_DIR "/includecollector_external2.h\"\n")));
}

TEST_F(PchCreatorVerySlowTest, CreateGlobalPchHeaderFile)
{
    auto file = creator.generateGlobalPchHeaderFile();
    file->open(QIODevice::ReadOnly);

    auto content = file->readAll();

    ASSERT_THAT(content.toStdString(),
                AllOf(HasSubstr("#include \"" TESTDATA_DIR "/includecollector_external3.h\"\n"),
                      HasSubstr("#include \"" TESTDATA_DIR "/includecollector_external1.h\"\n"),
                      HasSubstr("#include \"" TESTDATA_DIR "/includecollector_external2.h\"\n")));
}

TEST_F(PchCreator, ConvertToQStringList)
{
    auto arguments = creator.convertToQStringList({"-I", TESTDATA_DIR});

    ASSERT_THAT(arguments, ElementsAre(QString("-I"), QString(TESTDATA_DIR)));
}

TEST_F(PchCreator, CreateGlobalPchCompilerArguments)
{
    auto arguments = creator.generateGlobalPchCompilerArguments();

    ASSERT_THAT(arguments, ElementsAre("-x","c++-header", "-Xclang", "-emit-pch", "-o", EndsWith(".pch"), EndsWith(".h")));
}

TEST_F(PchCreator, CreateGlobalClangCompilerArguments)
{
    auto arguments = creator.generateGlobalClangCompilerArguments();

    ASSERT_THAT(arguments, AllOf(Contains("-Wno-pragma-once-outside-header"),
                                 Contains("-emit-pch"),
                                 Contains("-o"),
                                 Not(Contains(environment.clangCompilerPath()))));
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

TEST_F(PchCreator, CreateProjectPartHeaderAndSources)
{
    auto includeIds = creator.generateProjectPartHeaderAndSourcePaths(projectPart1);

    ASSERT_THAT(includeIds, UnorderedElementsAre(main1Path, header1Path));
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
//                                 Contains("-Xclang"),
//                                 Contains("-include-pch"),
//                                 Contains("-Xclang"),
//                                 Contains(EndsWith(".pch")),
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

TEST_F(PchCreatorVerySlowTest, ProjectPartPchsExistsAfterCreation)
{
    creator.generateGlobalPch();

    creator.generateProjectPartPch(projectPart1);

    ASSERT_TRUE(QFileInfo::exists(QString(creator.generateProjectPathPchHeaderFilePath(projectPart1))));
}

TEST_F(PchCreatorVerySlowTest, DISABLED_CreatePartPchs)
{
    creator.generateGlobalPch();

    auto includePaths = creator.generateProjectPartPch(projectPart1);

    ASSERT_THAT(includePaths.id, projectPart1.projectPartId);
    ASSERT_THAT(includePaths.filePathIds,
                AllOf(Contains(FilePathId{1, 1}),
                      Contains(FilePathId{1, 2}),
                      Contains(FilePathId{1, 3})));
}

TEST_F(PchCreatorVerySlowTest, IncludesForCreatePchsForProjectParts)
{
    creator.generatePchs();

    ASSERT_THAT(creator.takeProjectsIncludes(),
                ElementsAre(Field(&IdPaths::id, "project1"),
                            Field(&IdPaths::id, "project2")));
}

TEST_F(PchCreatorVerySlowTest, ProjectPartPchsForCreatePchsForProjectParts)
{
    EXPECT_CALL(mockPchGeneratorNotifier,
                taskFinished(ClangBackEnd::TaskFinishStatus::Successfully,
                             Field(&ProjectPartPch::projectPartId, "project1")));
    EXPECT_CALL(mockPchGeneratorNotifier,
                taskFinished(ClangBackEnd::TaskFinishStatus::Successfully,
                             Field(&ProjectPartPch::projectPartId, "project2")));

    creator.generatePchs();
}

TEST_F(PchCreatorVerySlowTest, IdPathsForCreatePchsForProjectParts)
{
    creator.generatePchs();

    ASSERT_THAT(creator.takeProjectsIncludes(),
                ElementsAre(AllOf(Field(&IdPaths::id, "project1"),
                                  Field(&IdPaths::filePathIds, AllOf(Contains(id(TESTDATA_DIR "/includecollector_header2.h")),
                                                               Contains(id(TESTDATA_DIR "/includecollector_external1.h")),
                                                               Contains(id(TESTDATA_DIR "/includecollector_external2.h"))))),
                            AllOf(Field(&IdPaths::id, "project2"),
                                  Field(&IdPaths::filePathIds, AllOf(Contains(id(TESTDATA_DIR "/includecollector_external1.h")),
                                                               Contains(id(TESTDATA_DIR "/includecollector_external3.h")),
                                                               Contains(id(TESTDATA_DIR "/includecollector_header1.h")),
                                                               Contains(id(TESTDATA_DIR "/includecollector_external2.h")))))));
}

TEST_F(PchCreator, CreateProjectPartHeaderAndSourcesContent)
{
    auto content = creator.generateProjectPartHeaderAndSourcesContent(projectPart1);

    ASSERT_THAT(content, Eq("#include \"" TESTDATA_DIR "/includecollector_header1.h\"\n"
                            "#include \"" TESTDATA_DIR "/includecollector_main3.cpp\"\n"));
}


}
