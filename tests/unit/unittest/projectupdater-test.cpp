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

#include <projectupdater.h>

#include <pchmanagerclient.h>
#include <precompiledheadersupdatedmessage.h>
#include <removepchprojectpartsmessage.h>
#include <updatepchprojectpartsmessage.h>

#include <cpptools/clangcompileroptionsbuilder.h>
#include <cpptools/projectpart.h>

namespace {

using testing::_;
using testing::ElementsAre;
using testing::SizeIs;
using testing::NiceMock;
using testing::AnyNumber;

using ClangBackEnd::V2::FileContainer;
using ClangBackEnd::V2::ProjectPartContainer;
using CppTools::ClangCompilerOptionsBuilder;

class ProjectUpdater : public testing::Test
{
protected:
    void SetUp() override;

protected:
    ClangPchManager::PchManagerClient pchManagerClient;
    MockPchManagerNotifier mockPchManagerNotifier{pchManagerClient};
    NiceMock<MockPchManagerServer> mockPchManagerServer;
    ClangPchManager::ProjectUpdater updater{mockPchManagerServer, pchManagerClient};
    Utils::SmallString projectPartId{"project1"};
    Utils::SmallString projectPartId2{"project2"};
    Utils::PathStringVector headerPaths = {"/path/to/header1.h", "/path/to/header2.h"};
    Utils::PathStringVector sourcePaths = {"/path/to/source1.cpp", "/path/to/source2.cpp"};
    CppTools::ProjectFile header1ProjectFile{headerPaths[0], CppTools::ProjectFile::CXXHeader};
    CppTools::ProjectFile header2ProjectFile{headerPaths[1], CppTools::ProjectFile::CXXHeader};
    CppTools::ProjectFile source1ProjectFile{sourcePaths[0], CppTools::ProjectFile::CXXSource};
    CppTools::ProjectFile source2ProjectFile{sourcePaths[1], CppTools::ProjectFile::CXXSource};
    CppTools::ProjectPart projectPart;
    ProjectPartContainer expectedContainer;
    FileContainer generatedFile{{"/path/to", "header1.h"}, "content", {}};
};

TEST_F(ProjectUpdater, CallUpdatePchProjectParts)
{
    std::vector<CppTools::ProjectPart*> projectParts = {&projectPart, &projectPart};
    ClangBackEnd::UpdatePchProjectPartsMessage message{{expectedContainer.clone(), expectedContainer.clone()},
                                                       {generatedFile}};

    EXPECT_CALL(mockPchManagerServer, updatePchProjectParts(message));

    updater.updateProjectParts(projectParts, {generatedFile});
}

TEST_F(ProjectUpdater, CallRemovePchProjectParts)
{
    EXPECT_CALL(mockPchManagerNotifier, precompiledHeaderRemoved(_)).Times(AnyNumber());
    ClangBackEnd::RemovePchProjectPartsMessage message{{projectPartId, projectPartId2}};

    EXPECT_CALL(mockPchManagerServer, removePchProjectParts(message));

    updater.removeProjectParts({projectPartId, projectPartId2});
}

TEST_F(ProjectUpdater, CallPrecompiledHeaderRemoved)
{
    ClangBackEnd::RemovePchProjectPartsMessage message{{projectPartId, projectPartId2}};

    EXPECT_CALL(mockPchManagerNotifier, precompiledHeaderRemoved(projectPartId.toQString()));
    EXPECT_CALL(mockPchManagerNotifier, precompiledHeaderRemoved(projectPartId2.toQString()));

    updater.removeProjectParts({projectPartId, projectPartId2});
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

void ProjectUpdater::SetUp()
{
    projectPart.files.push_back(header1ProjectFile);
    projectPart.files.push_back(header2ProjectFile);
    projectPart.files.push_back(source1ProjectFile);
    projectPart.files.push_back(source2ProjectFile);
    projectPart.displayName = projectPartId;

    Utils::SmallStringVector arguments{ClangPchManager::ProjectUpdater::compilerArguments(
                    &projectPart)};

    expectedContainer = {projectPartId.clone(),
                         arguments.clone(),
                         {headerPaths[1]},
                          sourcePaths.clone()};
}
}

