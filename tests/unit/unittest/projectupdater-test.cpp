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

#include "mockpchmanagerclient.h"
#include "mockpchmanagernotifier.h"
#include "mockpchmanagerserver.h"
#include "mockprecompiledheaderstorage.h"

#include <pchmanagerprojectupdater.h>

#include <filepathcaching.h>
#include <pchmanagerclient.h>
#include <precompiledheaderstorage.h>
#include <precompiledheadersupdatedmessage.h>
#include <refactoringdatabaseinitializer.h>
#include <removegeneratedfilesmessage.h>
#include <removeprojectpartsmessage.h>
#include <updategeneratedfilesmessage.h>
#include <updateprojectpartsmessage.h>

#include <cpptools/compileroptionsbuilder.h>
#include <cpptools/projectpart.h>

#include <utils/algorithm.h>

namespace {

using testing::_;
using testing::ElementsAre;
using testing::SizeIs;
using testing::NiceMock;
using testing::AnyNumber;

using ClangBackEnd::CompilerMacro;
using ClangBackEnd::V2::FileContainer;
using ClangBackEnd::V2::ProjectPartContainer;
using CppTools::CompilerOptionsBuilder;
using ProjectExplorer::HeaderPath;

class ProjectUpdater : public testing::Test
{
protected:
    ClangBackEnd::FilePathId filePathId(Utils::SmallStringView path)
    {
        return filePathCache.filePathId(ClangBackEnd::FilePathView{path});
    }

    ClangBackEnd::FilePathIds filePathIds(const Utils::PathStringVector &paths)
    {
        return filePathCache.filePathIds(Utils::transform(paths, [] (const Utils::PathString &path) {
            return ClangBackEnd::FilePathView(path);
        }));
    }

    void SetUp() override
    {
        projectPart.files.push_back(header1ProjectFile);
        projectPart.files.push_back(header2ProjectFile);
        projectPart.files.push_back(source1ProjectFile);
        projectPart.files.push_back(source2ProjectFile);
        projectPart.displayName = "projectb";
        projectPart.projectMacros = {{"FOO", "2"}, {"BAR", "1"}};
        projectPartId = projectPart.id();

        projectPart2.files.push_back(header2ProjectFile);
        projectPart2.files.push_back(header1ProjectFile);
        projectPart2.files.push_back(source2ProjectFile);
        projectPart2.files.push_back(source1ProjectFile);
        projectPart2.displayName = "projectaa";
        projectPart2.projectMacros = {{"BAR", "1"}, {"FOO", "2"}};
        projectPartId2 = projectPart2.id();


        Utils::SmallStringVector arguments{ClangPchManager::ProjectUpdater::compilerArguments(
                        &projectPart)};
        Utils::SmallStringVector arguments2{ClangPchManager::ProjectUpdater::compilerArguments(
                        &projectPart2)};

        expectedContainer = {projectPartId.clone(),
                             arguments.clone(),
                             Utils::clone(compilerMacros),
                             {},
                             {filePathId(headerPaths[1])},
                             {filePathIds(sourcePaths)}};
        expectedContainer2 = {projectPartId2.clone(),
                              arguments2.clone(),
                              Utils::clone(compilerMacros),
                              {},
                              {filePathId(headerPaths[1])},
                              {filePathIds(sourcePaths)}};
    }

protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> initializer{database};
    ClangBackEnd::FilePathCaching filePathCache{database};
    ClangPchManager::PchManagerClient pchManagerClient;
    MockPchManagerNotifier mockPchManagerNotifier{pchManagerClient};
    NiceMock<MockPchManagerServer> mockPchManagerServer;
    ClangPchManager::ProjectUpdater updater{mockPchManagerServer, filePathCache};
    Utils::SmallString projectPartId;
    Utils::SmallString projectPartId2;
    Utils::PathStringVector headerPaths = {"/path/to/header1.h", "/path/to/header2.h"};
    Utils::PathStringVector sourcePaths = {"/path/to/source1.cpp", "/path/to/source2.cpp"};
    ClangBackEnd::CompilerMacros compilerMacros = {{"BAR", "1"}, {"FOO", "2"}};
    CppTools::ProjectFile header1ProjectFile{QString(headerPaths[0]), CppTools::ProjectFile::CXXHeader};
    CppTools::ProjectFile header2ProjectFile{QString(headerPaths[1]), CppTools::ProjectFile::CXXHeader};
    CppTools::ProjectFile source1ProjectFile{QString(sourcePaths[0]), CppTools::ProjectFile::CXXSource};
    CppTools::ProjectFile source2ProjectFile{QString(sourcePaths[1]), CppTools::ProjectFile::CXXSource};
    CppTools::ProjectPart projectPart;
    CppTools::ProjectPart projectPart2;
    ProjectPartContainer expectedContainer;
    ProjectPartContainer expectedContainer2;
    FileContainer generatedFile{{"/path/to", "header1.h"}, "content", {}};
    FileContainer generatedFile2{{"/path/to2", "header1.h"}, "content", {}};
    FileContainer generatedFile3{{"/path/to", "header2.h"}, "content", {}};
};

TEST_F(ProjectUpdater, CallUpdateProjectParts)
{
    std::vector<CppTools::ProjectPart*> projectParts = {&projectPart2, &projectPart};
    ClangBackEnd::UpdateProjectPartsMessage message{{expectedContainer.clone(), expectedContainer2.clone()}};
    updater.updateGeneratedFiles({generatedFile});

    EXPECT_CALL(mockPchManagerServer, updateProjectParts(message));

    updater.updateProjectParts(projectParts);
}

TEST_F(ProjectUpdater, CallUpdateGeneratedFilesWithSortedEntries)
{
    ClangBackEnd::UpdateGeneratedFilesMessage message{{generatedFile, generatedFile3, generatedFile2}};

    EXPECT_CALL(mockPchManagerServer, updateGeneratedFiles(message));

    updater.updateGeneratedFiles({generatedFile2, generatedFile3, generatedFile});
}

TEST_F(ProjectUpdater, GeneratedFilesAddedAreSorted)
{
    updater.updateGeneratedFiles({generatedFile2, generatedFile3, generatedFile});

    ASSERT_THAT(updater.generatedFiles().fileContainers(),
                ElementsAre(generatedFile, generatedFile3, generatedFile2));
}

TEST_F(ProjectUpdater, GeneratedFilesAddedToExcludePaths)
{
    updater.updateGeneratedFiles({generatedFile2, generatedFile3, generatedFile});

    ASSERT_THAT(updater.excludedPaths(), ElementsAre(generatedFile.filePath,
                                                     generatedFile3.filePath,
                                                     generatedFile2.filePath));
}

TEST_F(ProjectUpdater, CallRemoveGeneratedFiles)
{
    ClangBackEnd::RemoveGeneratedFilesMessage message{{{"/path/to/header1.h"}}};

    EXPECT_CALL(mockPchManagerServer, removeGeneratedFiles(message));

    updater.removeGeneratedFiles({{"/path/to/header1.h"}});
}

TEST_F(ProjectUpdater, GeneratedFilesRemovedFromExcludePaths)
{
    updater.updateGeneratedFiles({generatedFile});

    updater.removeGeneratedFiles({generatedFile.filePath});

    ASSERT_THAT(updater.excludedPaths(), IsEmpty());
}

TEST_F(ProjectUpdater, CallRemoveProjectParts)
{
    ClangBackEnd::RemoveProjectPartsMessage message{{projectPartId, projectPartId2}};

    EXPECT_CALL(mockPchManagerServer, removeProjectParts(message));

    updater.removeProjectParts({QString(projectPartId2), QString(projectPartId)});
}

TEST_F(ProjectUpdater, CallPrecompiledHeaderRemovedInPchManagerProjectUpdater)
{
    ClangPchManager::PchManagerProjectUpdater pchUpdater{mockPchManagerServer, pchManagerClient, filePathCache};
    ClangBackEnd::RemoveProjectPartsMessage message{{projectPartId, projectPartId2}};

    EXPECT_CALL(mockPchManagerNotifier, precompiledHeaderRemoved(projectPartId.toQString()));
    EXPECT_CALL(mockPchManagerNotifier, precompiledHeaderRemoved(projectPartId2.toQString()));

    pchUpdater.removeProjectParts({QString(projectPartId), QString(projectPartId2)});
}

TEST_F(ProjectUpdater, ConvertProjectPartToProjectPartContainer)
{
    updater.setExcludedPaths({"/path/to/header1.h"});

    auto container = updater.toProjectPartContainer(&projectPart);

    ASSERT_THAT(container, expectedContainer);
}

TEST_F(ProjectUpdater, ConvertProjectPartToProjectPartContainersHaveSameSizeLikeProjectParts)
{
    auto containers = updater.toProjectPartContainers({&projectPart, &projectPart});

    ASSERT_THAT(containers, SizeIs(2));
}

TEST_F(ProjectUpdater, CreateSortedExcludedPaths)
{
    auto excludedPaths = updater.createExcludedPaths({generatedFile2, generatedFile3, generatedFile});

    ASSERT_THAT(excludedPaths, ElementsAre(ClangBackEnd::FilePath{"/path/to/header1.h"},
                                           ClangBackEnd::FilePath{"/path/to/header2.h"},
                                           ClangBackEnd::FilePath{"/path/to2/header1.h"}));
}

TEST_F(ProjectUpdater, CreateSortedCompilerMacros)
{
    auto paths = updater.createCompilerMacros({{"DEFINE", "1"}, {"FOO", "2"}, {"BAR", "1"}});

    ASSERT_THAT(paths, ElementsAre(CompilerMacro{"BAR", "1"},
                                   CompilerMacro{"FOO", "2"},
                                   CompilerMacro{"DEFINE", "1"}));
}

TEST_F(ProjectUpdater, CreateSortedIncludeSearchPaths)
{
    ProjectExplorer::HeaderPath includePath{"/to/path1", ProjectExplorer::HeaderPathType::User};
    ProjectExplorer::HeaderPath includePath2{"/to/path2", ProjectExplorer::HeaderPathType::User};
    ProjectExplorer::HeaderPath invalidPath;
    ProjectExplorer::HeaderPath frameworkPath{"/framework/path", ProjectExplorer::HeaderPathType::Framework};

    auto paths = updater.createIncludeSearchPaths({frameworkPath, includePath2, includePath, invalidPath});

    ASSERT_THAT(paths, ElementsAre(includePath.path, includePath2.path, frameworkPath.path));
}

}

