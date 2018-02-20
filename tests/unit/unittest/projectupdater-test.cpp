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
#include <removeprojectpartsmessage.h>
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
using CppTools::ProjectPartHeaderPath;

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
        projectPart.displayName = QString(projectPartId);
        projectPart.projectMacros.push_back({"DEFINE", "1"});


        Utils::SmallStringVector arguments{ClangPchManager::ProjectUpdater::compilerArguments(
                        &projectPart)};

        expectedContainer = {projectPartId.clone(),
                             arguments.clone(),
                             Utils::clone(compilerMacros),
                             {},
                             {filePathId(headerPaths[1])},
                             {filePathIds(sourcePaths)}};
    }

protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> initializer{database};
    ClangBackEnd::FilePathCaching filePathCache{database};
    MockPrecompiledHeaderStorage mockPrecompiledHeaderStorage;
    ClangPchManager::PchManagerClient pchManagerClient{mockPrecompiledHeaderStorage};
    MockPchManagerNotifier mockPchManagerNotifier{pchManagerClient};
    NiceMock<MockPchManagerServer> mockPchManagerServer;
    ClangPchManager::ProjectUpdater updater{mockPchManagerServer, filePathCache};
    Utils::SmallString projectPartId{"project1"};
    Utils::SmallString projectPartId2{"project2"};
    Utils::PathStringVector headerPaths = {"/path/to/header1.h", "/path/to/header2.h"};
    Utils::PathStringVector sourcePaths = {"/path/to/source1.cpp", "/path/to/source2.cpp"};
    ClangBackEnd::CompilerMacros compilerMacros = {{"DEFINE", "1"}};
    CppTools::ProjectFile header1ProjectFile{QString(headerPaths[0]), CppTools::ProjectFile::CXXHeader};
    CppTools::ProjectFile header2ProjectFile{QString(headerPaths[1]), CppTools::ProjectFile::CXXHeader};
    CppTools::ProjectFile source1ProjectFile{QString(sourcePaths[0]), CppTools::ProjectFile::CXXSource};
    CppTools::ProjectFile source2ProjectFile{QString(sourcePaths[1]), CppTools::ProjectFile::CXXSource};
    CppTools::ProjectPart projectPart;
    ProjectPartContainer expectedContainer;
    FileContainer generatedFile{{"/path/to", "header1.h"}, "content", {}};
};

TEST_F(ProjectUpdater, CallUpdateProjectParts)
{
    std::vector<CppTools::ProjectPart*> projectParts = {&projectPart, &projectPart};
    ClangBackEnd::UpdateProjectPartsMessage message{{expectedContainer.clone(), expectedContainer.clone()},
                                                    {generatedFile}};

    EXPECT_CALL(mockPchManagerServer, updateProjectParts(message));

    updater.updateProjectParts(projectParts, {generatedFile});
}

TEST_F(ProjectUpdater, CallRemoveProjectParts)
{
    ClangBackEnd::RemoveProjectPartsMessage message{{projectPartId, projectPartId2}};

    EXPECT_CALL(mockPchManagerServer, removeProjectParts(message));

    updater.removeProjectParts({QString(projectPartId), QString(projectPartId2)});
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

TEST_F(ProjectUpdater, CreateExcludedPaths)
{
    auto excludedPaths = updater.createExcludedPaths({generatedFile});

    ASSERT_THAT(excludedPaths, ElementsAre("/path/to/header1.h"));
}

TEST_F(ProjectUpdater, CreateCompilerMacros)
{
    auto paths = updater.createCompilerMacros({{"DEFINE", "1"}});

    ASSERT_THAT(paths, ElementsAre(CompilerMacro{"DEFINE", "1"}));
}

TEST_F(ProjectUpdater, CreateIncludeSearchPaths)
{
    ProjectPartHeaderPath includePath{"/to/path", ProjectPartHeaderPath::IncludePath};
    ProjectPartHeaderPath invalidPath;
    ProjectPartHeaderPath frameworkPath{"/framework/path", ProjectPartHeaderPath::FrameworkPath};

    auto paths = updater.createIncludeSearchPaths({includePath, invalidPath, frameworkPath});

    ASSERT_THAT(paths, ElementsAre(includePath.path, frameworkPath.path));
}

}

