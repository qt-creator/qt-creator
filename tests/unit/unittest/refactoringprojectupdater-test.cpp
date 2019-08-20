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

#include "mockcppmodelmanager.h"
#include "mockprogressmanager.h"
#include "mockprojectpartsstorage.h"
#include "mockrefactoringserver.h"

#include <sqlitedatabase.h>

#include <clangindexingsettingsmanager.h>
#include <clangrefactoringservermessages.h>
#include <filepathcaching.h>
#include <precompiledheadersupdatedmessage.h>
#include <refactoringdatabaseinitializer.h>

#include <pchmanagerclient.h>

#include <refactoringprojectupdater.h>

#include <projectexplorer/project.h>

#include <memory>

namespace {

using CppTools::ProjectPart;
using ClangBackEnd::UpdateProjectPartsMessage;
using ClangBackEnd::RemoveProjectPartsMessage;

MATCHER_P(IsProjectPartContainer,
          projectPartId,
          std::string(negation ? "hasn't" : "has") + " id " + PrintToString(projectPartId))
{
    const ClangBackEnd::ProjectPartContainer &container = arg;

    return  container.projectPartId == projectPartId;
}

class RefactoringProjectUpdater : public testing::Test
{
protected:
    RefactoringProjectUpdater()
    {
        ON_CALL(mockProjectPartsStorage, transactionBackend()).WillByDefault(ReturnRef(database));
    }
    ProjectPart::Ptr createProjectPart(const char *name)
    {
        ProjectPart::Ptr projectPart{new ProjectPart};
        projectPart->project = &project;
        projectPart->displayName = QString::fromUtf8(name, std::strlen(name));
        projectPartId = projectPart->id();
        return projectPart;
    }

protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> initializer{database};
    ClangBackEnd::FilePathCaching filePathCache{database};
    NiceMock<MockRefactoringServer> mockRefactoringServer;
    NiceMock<MockProgressManager> mockPchCreationProgressManager;
    NiceMock<MockProgressManager> mockDependencyCreationProgressManager;
    NiceMock<MockProjectPartsStorage> mockProjectPartsStorage;
    ClangPchManager::PchManagerClient pchManagerClient{mockPchCreationProgressManager,
                                                       mockDependencyCreationProgressManager};
    MockCppModelManager mockCppModelManager;
    ProjectExplorer::Project project;
    ClangPchManager::ClangIndexingSettingsManager settingsManager;
    ClangRefactoring::RefactoringProjectUpdater updater{mockRefactoringServer,
                                                        pchManagerClient,
                                                        mockCppModelManager,
                                                        filePathCache,
                                                        mockProjectPartsStorage,
                                                        settingsManager};
    Utils::SmallString projectPartId;
};

TEST_F(RefactoringProjectUpdater, DontUpdateProjectPartIfNoProjectPartExistsForId)
{
    InSequence s;

    EXPECT_CALL(mockProjectPartsStorage, fetchProjectPartName(Eq(3)))
        .WillOnce(Return(QString("project1")));
    EXPECT_CALL(mockCppModelManager, projectPartForId(Eq(QString("project1"))));
    EXPECT_CALL(mockRefactoringServer, updateProjectParts(_)).Times(0);

    pchManagerClient.precompiledHeadersUpdated({3});
}

TEST_F(RefactoringProjectUpdater, UpdateProjectPart)
{
    InSequence s;

    EXPECT_CALL(mockProjectPartsStorage, fetchProjectPartName(Eq(3)))
        .WillRepeatedly(Return(QString(" project1")));
    EXPECT_CALL(mockCppModelManager, projectPartForId(Eq(QString(" project1"))))
        .WillRepeatedly(Return(createProjectPart("project1")));
    EXPECT_CALL(mockRefactoringServer,
                updateProjectParts(Field(&UpdateProjectPartsMessage::projectsParts,
                                         ElementsAre(IsProjectPartContainer(3)))));

    pchManagerClient.precompiledHeadersUpdated({3});
}

TEST_F(RefactoringProjectUpdater, RemoveProjectPart)
{
    EXPECT_CALL(mockRefactoringServer,
                removeProjectParts(
                    Field(&RemoveProjectPartsMessage::projectsPartIds, ElementsAre(Eq(1)))));

    pchManagerClient.precompiledHeaderRemoved(1);
}
}
