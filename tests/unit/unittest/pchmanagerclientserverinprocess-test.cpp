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
#include <progressmessage.h>
#include <removegeneratedfilesmessage.h>
#include <removeprojectpartsmessage.h>
#include <updategeneratedfilesmessage.h>
#include <updateprojectpartsmessage.h>

#include <QBuffer>
#include <QString>
#include <QVariant>

#include <vector>

using ClangBackEnd::UpdateGeneratedFilesMessage;
using ClangBackEnd::UpdateProjectPartsMessage;
using ClangBackEnd::RemoveGeneratedFilesMessage;
using ClangBackEnd::RemoveProjectPartsMessage;
using ClangBackEnd::V2::FileContainer;
using ClangBackEnd::ProjectPartContainer;
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

TEST_F(PchManagerClientServerInProcess, SendUpdateProjectPartsMessage)
{
    ProjectPartContainer projectPart{1,
                                     {"-x", "c++-header", "-Wno-pragma-once-outside-header"},
                                     {{"DEFINE", "1", 1}},
                                     {{"/includes", 1, ClangBackEnd::IncludeSearchPathType::BuiltIn}},
                                     {{"/project/includes", 1, ClangBackEnd::IncludeSearchPathType::User}},
                                     {{1, 1}},
                                     {{1, 2}},
                                     Utils::Language::C,
                                     Utils::LanguageVersion::C11,
                                     Utils::LanguageExtension::All};
    UpdateProjectPartsMessage message{{projectPart}, {"-m32"}};

    EXPECT_CALL(mockPchManagerServer, updateProjectParts(message));

    serverProxy.updateProjectParts(message.clone());
    scheduleServerMessages();
}

TEST_F(PchManagerClientServerInProcess, SendUpdateGeneratedFilesMessage)
{
    FileContainer fileContainer{{"/path/to/", "file"}, "content", {}};
    UpdateGeneratedFilesMessage message{{fileContainer}};

    EXPECT_CALL(mockPchManagerServer, updateGeneratedFiles(message));

    serverProxy.updateGeneratedFiles(message.clone());
    scheduleServerMessages();
}

TEST_F(PchManagerClientServerInProcess, SendRemoveProjectPartsMessage)
{
    RemoveProjectPartsMessage message{{1, 2}};

    EXPECT_CALL(mockPchManagerServer, removeProjectParts(message));

    serverProxy.removeProjectParts(message.clone());
    scheduleServerMessages();
}

TEST_F(PchManagerClientServerInProcess, SendRemoveGeneratedFilesMessage)
{
    RemoveGeneratedFilesMessage message{{{"/path/to/", "file"}}};

    EXPECT_CALL(mockPchManagerServer, removeGeneratedFiles(message));

    serverProxy.removeGeneratedFiles(message.clone());
    scheduleServerMessages();
}

TEST_F(PchManagerClientServerInProcess, SendPrecompiledHeaderUpdatedMessage)
{
    PrecompiledHeadersUpdatedMessage message{{{1, "/path/to/pch", 1}}};

    EXPECT_CALL(mockPchManagerClient, precompiledHeadersUpdated(message));

    clientProxy.precompiledHeadersUpdated(message.clone());
    scheduleClientMessages();
}

TEST_F(PchManagerClientServerInProcess, SendProgressMessage)
{
    ClangBackEnd::ProgressMessage message{ClangBackEnd::ProgressType::PrecompiledHeader, 10, 50};


    EXPECT_CALL(mockPchManagerClient, progress(message));

    clientProxy.progress(message.clone());
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
