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
#include "mockrefactoringserver.h"

#include <sqlitedatabase.h>

#include <filepathcaching.h>
#include <precompiledheadersupdatedmessage.h>
#include <refactoringdatabaseinitializer.h>
#include <clangrefactoringservermessages.h>

#include <pchmanagerclient.h>

#include <refactoringprojectupdater.h>

#include <memory>

namespace {

using CppTools::ProjectPart;
using ClangBackEnd::UpdateProjectPartsMessage;
using ClangBackEnd::RemoveProjectPartsMessage;

MATCHER_P(IsProjectPartContainer, projectPartId,
          std::string(negation ? "hasn't" : "has")
          + " name " + std::string(projectPartId))
{
    const ClangBackEnd::V2::ProjectPartContainer &container = arg;

    return  container.projectPartId == projectPartId;
}

class RefactoringProjectUpdater : public testing::Test
{
protected:
    ProjectPart::Ptr createProjectPart(const char *name)
    {
        ProjectPart::Ptr projectPart{new ProjectPart};
        projectPart->displayName = QString::fromUtf8(name, std::strlen(name));
        projectPartId = projectPart->id();
        return projectPart;
    }

    Utils::SmallString createProjectPartId(const char *name)
    {
        ProjectPart::Ptr projectPart{new ProjectPart};
        projectPart->displayName = QString::fromUtf8(name, std::strlen(name));
        return projectPart->id();
    }

    QString createProjectPartQStringId(const char *name)
    {
        ProjectPart::Ptr projectPart{new ProjectPart};
        projectPart->displayName = QString::fromUtf8(name, std::strlen(name));
        return projectPart->id();
    }

protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> initializer{database};
    ClangBackEnd::FilePathCaching filePathCache{database};
    NiceMock<MockRefactoringServer> mockRefactoringServer;
    ClangPchManager::PchManagerClient pchManagerClient;
    MockCppModelManager mockCppModelManager;
    ClangRefactoring::RefactoringProjectUpdater updater{mockRefactoringServer, pchManagerClient, mockCppModelManager, filePathCache};
    Utils::SmallString projectPartId;
};

TEST_F(RefactoringProjectUpdater, DontUpdateProjectPartIfNoProjectPartExistsForId)
{
    EXPECT_CALL(mockCppModelManager, projectPartForId(Eq(createProjectPartQStringId("project1"))));

    pchManagerClient.precompiledHeadersUpdated({{{createProjectPartId("project1"), "/path/to/pch", 12}}});
}

TEST_F(RefactoringProjectUpdater, UpdateProjectPart)
{
    EXPECT_CALL(mockCppModelManager, projectPartForId(Eq(createProjectPartQStringId("project1")))).WillRepeatedly(Return(createProjectPart("project1")));
    EXPECT_CALL(mockRefactoringServer, updateProjectParts(
                    Field(&UpdateProjectPartsMessage::projectsParts,
                          ElementsAre(IsProjectPartContainer(createProjectPartId("project1"))))));

    pchManagerClient.precompiledHeadersUpdated({{{createProjectPartId("project1"), "/path/to/pch", 12}}});
}

TEST_F(RefactoringProjectUpdater, RemoveProjectPart)
{
    EXPECT_CALL(mockRefactoringServer, removeProjectParts(
                    Field(&RemoveProjectPartsMessage::projectsPartIds,
                          ElementsAre(Eq("project1")))));

    pchManagerClient.precompiledHeaderRemoved({"project1"});
}

TEST_F(RefactoringProjectUpdater, UpdateGeneratedFiles)
{
    EXPECT_CALL(mockRefactoringServer, removeProjectParts(
                    Field(&RemoveProjectPartsMessage::projectsPartIds,
                          ElementsAre(Eq("project1")))));

    pchManagerClient.precompiledHeaderRemoved({"project1"});
}

}
