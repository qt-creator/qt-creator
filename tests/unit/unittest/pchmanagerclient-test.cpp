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

#include "mockpchmanagernotifier.h"
#include "mockpchmanagerserver.h"
#include "mockprecompiledheaderstorage.h"
#include "mockprogressmanager.h"
#include "mockprojectpartsstorage.h"

#include <pchmanagerclient.h>
#include <pchmanagerprojectupdater.h>

#include <filepathcaching.h>
#include <refactoringdatabaseinitializer.h>
#include <precompiledheadersupdatedmessage.h>
#include <progressmessage.h>
#include <removegeneratedfilesmessage.h>
#include <removeprojectpartsmessage.h>
#include <updategeneratedfilesmessage.h>
#include <updateprojectpartsmessage.h>

namespace {

using ClangBackEnd::PrecompiledHeadersUpdatedMessage;

class PchManagerClient : public ::testing::Test
{
protected:
    NiceMock<MockProgressManager> mockPchCreationProgressManager;
    NiceMock<MockProgressManager> mockDependencyCreationProgressManager;
    ClangPchManager::PchManagerClient client{mockPchCreationProgressManager,
                                             mockDependencyCreationProgressManager};
    NiceMock<MockPchManagerServer> mockPchManagerServer;
    NiceMock<MockProjectPartsStorage> mockProjectPartsStorage;
    NiceMock<MockPchManagerNotifier> mockPchManagerNotifier{client};
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> initializer{database};
    ClangBackEnd::FilePathCaching filePathCache{database};
    ClangPchManager::PchManagerProjectUpdater projectUpdater{mockPchManagerServer,
                                                             client,
                                                             filePathCache,
                                                             mockProjectPartsStorage};
    ClangBackEnd::ProjectPartId projectPartId{1};
    ClangBackEnd::FilePath pchFilePath{"/path/to/pch"};
    PrecompiledHeadersUpdatedMessage message{{{projectPartId, pchFilePath.clone(), 1}}};
    ClangBackEnd::ProjectPartId projectPartId2{2};
    ClangBackEnd::FilePath pchFilePath2{"/path/to/pch2"};
    PrecompiledHeadersUpdatedMessage message2{{{projectPartId2, pchFilePath2.clone(), 1}}};
};

TEST_F(PchManagerClient, NotifierAttached)
{
    MockPchManagerNotifier notifier(client);

    ASSERT_THAT(client.notifiers(), Contains(&notifier));
}

TEST_F(PchManagerClient, NotifierDetached)
{
    MockPchManagerNotifier *notifierPointer = nullptr;

    {
        MockPchManagerNotifier notifier(client);
        notifierPointer = &notifier;
    }

    ASSERT_THAT(client.notifiers(), Not(Contains(notifierPointer)));
}

TEST_F(PchManagerClient, Update)
{
    EXPECT_CALL(mockPchManagerNotifier,
                precompiledHeaderUpdated(projectPartId, pchFilePath.toQString(), Eq(1)));

    client.precompiledHeadersUpdated(message.clone());
}

TEST_F(PchManagerClient, Remove)
{
    EXPECT_CALL(mockPchManagerNotifier, precompiledHeaderRemoved(projectPartId)).Times(2);

    projectUpdater.removeProjectParts({projectPartId, projectPartId});
}

TEST_F(PchManagerClient, GetNoProjectPartPchForWrongProjectPartId)
{
    auto optional = client.projectPartPch(23);

    ASSERT_FALSE(optional);
}

TEST_F(PchManagerClient, GetProjectPartPchForProjectPartId)
{
    client.precompiledHeadersUpdated(std::move(message));

    auto optional = client.projectPartPch(projectPartId);

    ASSERT_TRUE(optional);
}

TEST_F(PchManagerClient, ProjectPartPchRemoved)
{
    client.precompiledHeadersUpdated(std::move(message));

    client.precompiledHeaderRemoved(projectPartId);

    ASSERT_FALSE(client.projectPartPch(projectPartId));
}

TEST_F(PchManagerClient, ProjectPartPchHasNoDublicateEntries)
{
    client.precompiledHeadersUpdated(message.clone());
    client.precompiledHeadersUpdated(message2.clone());

    client.precompiledHeadersUpdated(message.clone());

    ASSERT_THAT(client.projectPartPchs(), SizeIs(2));
}

TEST_F(PchManagerClient, ProjectPartPchForProjectPartIdLastModified)
{
    client.precompiledHeadersUpdated(std::move(message));

    ASSERT_THAT(client.projectPartPch(projectPartId)->lastModified, 1);
}

TEST_F(PchManagerClient, ProjectPartPchForProjectPartIdIsUpdated)
{
    client.precompiledHeadersUpdated(message.clone());
    PrecompiledHeadersUpdatedMessage updateMessage{{{projectPartId, pchFilePath.clone(), 42}}};

    client.precompiledHeadersUpdated(updateMessage.clone());

    ASSERT_THAT(client.projectPartPch(projectPartId)->lastModified, 42);
}

TEST_F(PchManagerClient, SetPchCreationProgress)
{
    EXPECT_CALL(mockPchCreationProgressManager, setProgress(10, 20));

    client.progress({ClangBackEnd::ProgressType::PrecompiledHeader, 10, 20});
}

TEST_F(PchManagerClient, SetDependencyCreationProgress)
{
    EXPECT_CALL(mockDependencyCreationProgressManager, setProgress(30, 40));

    client.progress({ClangBackEnd::ProgressType::DependencyCreation, 30, 40});
}
} // namespace
