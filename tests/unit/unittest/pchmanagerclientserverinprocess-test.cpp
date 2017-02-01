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
#include "mockpchmanagerserver.h"

#include <writemessageblock.h>
#include <pchmanagerclientproxy.h>
#include <pchmanagerserverproxy.h>
#include <precompiledheadersupdatedmessage.h>
#include <removepchprojectpartsmessage.h>
#include <updatepchprojectpartsmessage.h>

#include <QBuffer>
#include <QString>
#include <QVariant>

#include <vector>

using ClangBackEnd::UpdatePchProjectPartsMessage;
using ClangBackEnd::V2::FileContainer;
using ClangBackEnd::V2::ProjectPartContainer;
using ClangBackEnd::RemovePchProjectPartsMessage;
using ClangBackEnd::PrecompiledHeadersUpdatedMessage;

using ::testing::Args;
using ::testing::Property;
using ::testing::Eq;

namespace {

class PchManagerClientServerInProcess : public ::testing::Test
{
protected:
    PchManagerClientServerInProcess();

    void SetUp();
    void TearDown();

    void scheduleServerMessages();
    void scheduleClientMessages();

protected:
    QBuffer buffer;
    MockPchManagerClient mockPchManagerClient;
    MockPchManagerServer mockPchManagerServer;

    ClangBackEnd::PchManagerServerProxy serverProxy;
    ClangBackEnd::PchManagerClientProxy clientProxy;
};

TEST_F(PchManagerClientServerInProcess, SendEndMessage)
{
    EXPECT_CALL(mockPchManagerServer, end());

    serverProxy.end();
    scheduleServerMessages();
}

TEST_F(PchManagerClientServerInProcess, SendAliveMessage)
{

    EXPECT_CALL(mockPchManagerClient, alive());

    clientProxy.alive();
    scheduleClientMessages();
}

TEST_F(PchManagerClientServerInProcess, SendUpdatePchProjectPartsMessage)
{
    ProjectPartContainer projectPart2{"projectPartId",
                                      {"-x", "c++-header", "-Wno-pragma-once-outside-header"},
                                      {TESTDATA_DIR "/includecollector_header.h"},
                                      {TESTDATA_DIR "/includecollector_main.cpp"}};
    FileContainer fileContainer{{"/path/to/", "file"}, "content", {}};
    UpdatePchProjectPartsMessage message{{projectPart2}, {fileContainer}};

    EXPECT_CALL(mockPchManagerServer, updatePchProjectParts(message));

    serverProxy.updatePchProjectParts(message.clone());
    scheduleServerMessages();
}

TEST_F(PchManagerClientServerInProcess, SendRemovePchProjectPartsMessage)
{
    RemovePchProjectPartsMessage message{{"projectPartId1", "projectPartId2"}};

    EXPECT_CALL(mockPchManagerServer, removePchProjectParts(message));

    serverProxy.removePchProjectParts(message.clone());
    scheduleServerMessages();
}

TEST_F(PchManagerClientServerInProcess, SendPrecompiledHeaderUpdatedMessage)
{
    PrecompiledHeadersUpdatedMessage message{{{"projectPartId", "/path/to/pch"}}};


    EXPECT_CALL(mockPchManagerClient, precompiledHeadersUpdated(message));

    clientProxy.precompiledHeadersUpdated(message.clone());
    scheduleClientMessages();
}

PchManagerClientServerInProcess::PchManagerClientServerInProcess()
    : serverProxy(&mockPchManagerClient, &buffer),
      clientProxy(&mockPchManagerServer, &buffer)
{
}

void PchManagerClientServerInProcess::SetUp()
{
    buffer.open(QIODevice::ReadWrite);
}

void PchManagerClientServerInProcess::TearDown()
{
    buffer.close();
}

void PchManagerClientServerInProcess::scheduleServerMessages()
{
    buffer.seek(0);
    clientProxy.readMessages();
    buffer.buffer().clear();
}

void PchManagerClientServerInProcess::scheduleClientMessages()
{
    buffer.seek(0);
    serverProxy.readMessages();
    buffer.buffer().clear();
}

}
