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

#include <pchmanagerclient.h>
#include <pchmanagerprojectupdater.h>

#include <precompiledheadersupdatedmessage.h>
#include <removepchprojectpartsmessage.h>
#include <updatepchprojectpartsmessage.h>

namespace {

using ClangBackEnd::PrecompiledHeadersUpdatedMessage;

using testing::_;
using testing::Contains;
using testing::Not;

class PchManagerClient : public ::testing::Test
{
protected:
    MockPchManagerServer mockPchManagerServer;
    ClangPchManager::PchManagerClient client;
    MockPchManagerNotifier mockPchManagerNotifier{client};
    ClangPchManager::PchManagerProjectUpdater projectUpdater{mockPchManagerServer, client};
    Utils::SmallString projectPartId{"projectPartId"};
    Utils::SmallString pchFilePath{"/path/to/pch"};
    PrecompiledHeadersUpdatedMessage message{{{projectPartId.clone(), pchFilePath.clone()}}};
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
    EXPECT_CALL(mockPchManagerNotifier, precompiledHeaderUpdated(projectPartId.toQString(), pchFilePath.toQString()));

    client.precompiledHeadersUpdated(message.clone());
}

TEST_F(PchManagerClient, Remove)
{
    EXPECT_CALL(mockPchManagerNotifier, precompiledHeaderRemoved(projectPartId.toQString()))
        .Times(2);

    projectUpdater.removeProjectParts({QString(projectPartId.clone()),
                                       QString(projectPartId.clone())});
}

}
