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

#include "mockclangcodemodelclient.h"

#include <clangcodemodelconnectionclient.h>
#include <clangcodemodelservermessages.h>
#include <readmessageblock.h>
#include <writemessageblock.h>

#include <utils/hostosinfo.h>

#include <QBuffer>
#include <QProcess>
#include <QSignalSpy>
#include <QString>
#include <QVariant>

#include <memory>
#include <vector>

using namespace ClangBackEnd;

using ::testing::Eq;
using ::testing::SizeIs;

class ClientServerOutsideProcess : public ::testing::Test
{
protected:
    void SetUp();
    void TearDown();

protected:
    NiceMock<MockClangCodeModelClient> mockClangCodeModelClient;
    ClangBackEnd::ClangCodeModelConnectionClient client{&mockClangCodeModelClient};
};

using ClientServerOutsideProcessSlowTest = ClientServerOutsideProcess;

TEST_F(ClientServerOutsideProcessSlowTest, RestartProcessAsynchronously)
{
    QSignalSpy clientSpy(&client, &ConnectionClient::connectedToLocalSocket);

    client.restartProcessAsynchronously();

    ASSERT_TRUE(clientSpy.wait(100000));
    ASSERT_TRUE(client.isProcessRunning());
    ASSERT_TRUE(client.isConnected());
}

TEST_F(ClientServerOutsideProcessSlowTest, RestartProcessAfterAliveTimeout)
{
    QSignalSpy clientSpy(&client, &ConnectionClient::connectedToLocalSocket);

    client.setProcessAliveTimerInterval(1);

    ASSERT_TRUE(clientSpy.wait(100000));
    ASSERT_THAT(clientSpy, SizeIs(1));
}

TEST_F(ClientServerOutsideProcessSlowTest, RestartProcessAfterTermination)
{
    QSignalSpy clientSpy(&client, &ConnectionClient::connectedToLocalSocket);

    client.processForTestOnly()->kill();

    ASSERT_TRUE(clientSpy.wait(100000));
    ASSERT_THAT(clientSpy, SizeIs(1));
}

TEST_F(ClientServerOutsideProcess, SendDocumentsOpenedMessage)
{
    auto filePath = Utf8StringLiteral("foo.cpp");
    ClangBackEnd::FileContainer fileContainer(filePath, Utf8StringLiteral("projectId"));
    ClangBackEnd::DocumentsOpenedMessage documentsOpenedMessage({fileContainer},
                                                                filePath,
                                                                {filePath});
    EchoMessage echoMessage(documentsOpenedMessage);

    EXPECT_CALL(mockClangCodeModelClient, echo(echoMessage));

    client.serverProxy().documentsOpened(documentsOpenedMessage);
    ASSERT_TRUE(client.waitForEcho());
}

TEST_F(ClientServerOutsideProcess, SendDocumentsClosedMessage)
{
    FileContainer fileContainer(Utf8StringLiteral("foo.cpp"), Utf8StringLiteral("projectId"));
    ClangBackEnd::DocumentsClosedMessage documentsClosedMessage({fileContainer});
    EchoMessage echoMessage(documentsClosedMessage);

    EXPECT_CALL(mockClangCodeModelClient, echo(echoMessage));

    client.serverProxy().documentsClosed(documentsClosedMessage);
    ASSERT_TRUE(client.waitForEcho());
}

TEST_F(ClientServerOutsideProcess, SendCompleteCodeMessage)
{
    RequestCompletionsMessage codeCompleteMessage(Utf8StringLiteral("foo.cpp"), 24, 33);
    EchoMessage echoMessage(codeCompleteMessage);

    EXPECT_CALL(mockClangCodeModelClient, echo(echoMessage));

    client.serverProxy().requestCompletions(codeCompleteMessage);
    ASSERT_TRUE(client.waitForEcho());
}

void ClientServerOutsideProcess::SetUp()
{
    QSignalSpy clientSpy(&client, &ConnectionClient::connectedToLocalSocket);
    client.setProcessPath(Utils::HostOsInfo::withExecutableSuffix(QStringLiteral(ECHOSERVER)));

    client.startProcessAndConnectToServerAsynchronously();

    ASSERT_TRUE(clientSpy.wait(100000));
    ASSERT_THAT(clientSpy, SizeIs(1));

    client.setProcessAliveTimerInterval(1000000);

    ASSERT_TRUE(client.isConnected());
}

void ClientServerOutsideProcess::TearDown()
{
    client.setProcessAliveTimerInterval(1000000);
    client.waitForConnected();

    ASSERT_TRUE(client.isProcessRunning());
    ASSERT_TRUE(client.isConnected());

    client.finishProcess();
}
