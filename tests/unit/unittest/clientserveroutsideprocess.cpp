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

#include "mockipclient.h"

#include <cmbalivemessage.h>
#include <cmbcodecompletedmessage.h>
#include <cmbcompletecodemessage.h>
#include <cmbechomessage.h>
#include <cmbendmessage.h>
#include <cmbregisterprojectsforeditormessage.h>
#include <cmbregistertranslationunitsforeditormessage.h>
#include <cmbunregisterprojectsforeditormessage.h>
#include <cmbunregistertranslationunitsforeditormessage.h>
#include <highlightingchangedmessage.h>
#include <connectionclient.h>
#include <diagnosticschangedmessage.h>
#include <projectpartsdonotexistmessage.h>
#include <readmessageblock.h>
#include <translationunitdoesnotexistmessage.h>
#include <writemessageblock.h>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

#include <QBuffer>
#include <QProcess>
#include <QSignalSpy>
#include <QString>
#include <QVariant>

#include <vector>

#ifdef Q_OS_WIN
#define QTC_HOST_EXE_SUFFIX L".exe"
#else
#define QTC_HOST_EXE_SUFFIX ""
#endif

using namespace ClangBackEnd;

using ::testing::Eq;

class ClientServerOutsideProcess : public ::testing::Test
{
protected:
    void SetUp();
    void TearDown();

    static void SetUpTestCase();
    static void TearDownTestCase();

    static MockIpcClient mockIpcClient;
    static ClangBackEnd::ConnectionClient client;
};

MockIpcClient ClientServerOutsideProcess::mockIpcClient;
ClangBackEnd::ConnectionClient ClientServerOutsideProcess::client(&ClientServerOutsideProcess::mockIpcClient);

TEST_F(ClientServerOutsideProcess, RestartProcess)
{
    client.restartProcess();

    ASSERT_TRUE(client.isProcessIsRunning());
    ASSERT_TRUE(client.isConnected());
}

TEST_F(ClientServerOutsideProcess, RestartProcessAfterAliveTimeout)
{
    QSignalSpy clientSpy(&client, SIGNAL(processRestarted()));

    client.setProcessAliveTimerInterval(1);

    ASSERT_TRUE(clientSpy.wait(100000));
}

TEST_F(ClientServerOutsideProcess, RestartProcessAfterTermination)
{
    QSignalSpy clientSpy(&client, SIGNAL(processRestarted()));

    client.processForTestOnly()->kill();

    ASSERT_TRUE(clientSpy.wait(100000));
}

TEST_F(ClientServerOutsideProcess, SendRegisterTranslationUnitForEditorMessage)
{
    auto filePath = Utf8StringLiteral("foo.cpp");
    ClangBackEnd::FileContainer fileContainer(filePath, Utf8StringLiteral("projectId"));
    ClangBackEnd::RegisterTranslationUnitForEditorMessage registerTranslationUnitForEditorMessage({fileContainer},
                                                                                                  filePath,
                                                                                                  {filePath});
    EchoMessage echoMessage(registerTranslationUnitForEditorMessage);

    EXPECT_CALL(mockIpcClient, echo(echoMessage))
            .Times(1);

    client.serverProxy().registerTranslationUnitsForEditor(registerTranslationUnitForEditorMessage);
    ASSERT_TRUE(client.waitForEcho());
}

TEST_F(ClientServerOutsideProcess, SendUnregisterTranslationUnitsForEditorMessage)
{
    FileContainer fileContainer(Utf8StringLiteral("foo.cpp"), Utf8StringLiteral("projectId"));
    ClangBackEnd::UnregisterTranslationUnitsForEditorMessage unregisterTranslationUnitsForEditorMessage ({fileContainer});
    EchoMessage echoMessage(unregisterTranslationUnitsForEditorMessage);

    EXPECT_CALL(mockIpcClient, echo(echoMessage))
            .Times(1);

    client.serverProxy().unregisterTranslationUnitsForEditor(unregisterTranslationUnitsForEditorMessage);
    ASSERT_TRUE(client.waitForEcho());
}

TEST_F(ClientServerOutsideProcess, SendCompleteCodeMessage)
{
    CompleteCodeMessage codeCompleteMessage(Utf8StringLiteral("foo.cpp"), 24, 33, Utf8StringLiteral("do what I want"));
    EchoMessage echoMessage(codeCompleteMessage);

    EXPECT_CALL(mockIpcClient, echo(echoMessage))
            .Times(1);

    client.serverProxy().completeCode(codeCompleteMessage);
    ASSERT_TRUE(client.waitForEcho());
}

TEST_F(ClientServerOutsideProcess, SendRegisterProjectPartsForEditorMessage)
{
    ClangBackEnd::ProjectPartContainer projectContainer(Utf8StringLiteral(TESTDATA_DIR"/complete.pro"));
    ClangBackEnd::RegisterProjectPartsForEditorMessage registerProjectPartsForEditorMessage({projectContainer});
    EchoMessage echoMessage(registerProjectPartsForEditorMessage);

    EXPECT_CALL(mockIpcClient, echo(echoMessage))
            .Times(1);

    client.serverProxy().registerProjectPartsForEditor(registerProjectPartsForEditorMessage);
    ASSERT_TRUE(client.waitForEcho());
}

TEST_F(ClientServerOutsideProcess, SendUnregisterProjectPartsForEditorMessage)
{
    ClangBackEnd::UnregisterProjectPartsForEditorMessage unregisterProjectPartsForEditorMessage({Utf8StringLiteral(TESTDATA_DIR"/complete.pro")});
    EchoMessage echoMessage(unregisterProjectPartsForEditorMessage);

    EXPECT_CALL(mockIpcClient, echo(echoMessage))
            .Times(1);

    client.serverProxy().unregisterProjectPartsForEditor(unregisterProjectPartsForEditorMessage);
    ASSERT_TRUE(client.waitForEcho());
}

void ClientServerOutsideProcess::SetUpTestCase()
{
    client.setProcessPath(QStringLiteral(ECHOSERVER QTC_HOST_EXE_SUFFIX));

    ASSERT_TRUE(client.connectToServer());
}

void ClientServerOutsideProcess::TearDownTestCase()
{
    client.finishProcess();
}
void ClientServerOutsideProcess::SetUp()
{
    client.setProcessAliveTimerInterval(1000000);

    ASSERT_TRUE(client.isConnected());
}

void ClientServerOutsideProcess::TearDown()
{
    ASSERT_TRUE(client.isProcessIsRunning());
    ASSERT_TRUE(client.isConnected());
}
