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
#include "mockipcserver.h"

#include <ipcclientproxy.h>
#include <ipcserverproxy.h>
#include <projectpartsdonotexistmessage.h>

#include <cmbalivemessage.h>
#include <cmbcodecompletedmessage.h>
#include <cmbcompletecodemessage.h>
#include <cmbechomessage.h>
#include <cmbendmessage.h>
#include <cmbregisterprojectsforeditormessage.h>
#include <cmbregistertranslationunitsforeditormessage.h>
#include <cmbunregisterprojectsforeditormessage.h>
#include <cmbunregistertranslationunitsforeditormessage.h>
#include <diagnosticschangedmessage.h>
#include <highlightingchangedmessage.h>
#include <readmessageblock.h>
#include <registerunsavedfilesforeditormessage.h>
#include <requestdiagnosticsmessage.h>
#include <requesthighlightingmessage.h>
#include <translationunitdoesnotexistmessage.h>
#include <unregisterunsavedfilesforeditormessage.h>
#include <updatetranslationunitsforeditormessage.h>
#include <updatevisibletranslationunitsmessage.h>
#include <writemessageblock.h>

#include <QBuffer>
#include <QString>
#include <QVariant>

#include <vector>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

using namespace ClangBackEnd;

namespace {

using ::testing::Args;
using ::testing::Property;
using ::testing::Eq;

class ClientServerInProcess : public ::testing::Test
{
protected:
    ClientServerInProcess();

    void SetUp();
    void TearDown();


    void scheduleServerMessages();
    void scheduleClientMessages();

protected:
    Utf8String filePath{Utf8StringLiteral(TESTDATA_DIR"/complete_extractor_function.cpp")};
    ClangBackEnd::FileContainer fileContainer{filePath,
                                              Utf8StringLiteral("projectPartId"),
                                              Utf8StringLiteral("unsaved content"),
                                              true,
                                              1};
    QBuffer buffer;
    MockIpcClient mockIpcClient;
    MockIpcServer mockIpcServer;

    ClangBackEnd::IpcServerProxy serverProxy;
    ClangBackEnd::IpcClientProxy clientProxy;
};

TEST_F(ClientServerInProcess, SendEndMessage)
{
    EXPECT_CALL(mockIpcServer, end())
        .Times(1);

    serverProxy.end();
    scheduleServerMessages();
}

TEST_F(ClientServerInProcess, SendAliveMessage)
{
    EXPECT_CALL(mockIpcClient, alive())
        .Times(1);

    clientProxy.alive();
    scheduleClientMessages();
}

TEST_F(ClientServerInProcess, SendRegisterTranslationUnitForEditorMessage)
{
    ClangBackEnd::RegisterTranslationUnitForEditorMessage message({fileContainer},
                                                                  filePath,
                                                                  {filePath});

    EXPECT_CALL(mockIpcServer, registerTranslationUnitsForEditor(message))
        .Times(1);

    serverProxy.registerTranslationUnitsForEditor(message);
    scheduleServerMessages();
}

TEST_F(ClientServerInProcess, SendUpdateTranslationUnitsForEditorMessage)
{
    ClangBackEnd::UpdateTranslationUnitsForEditorMessage message({fileContainer});

    EXPECT_CALL(mockIpcServer, updateTranslationUnitsForEditor(message))
        .Times(1);

    serverProxy.updateTranslationUnitsForEditor(message);
    scheduleServerMessages();
}

TEST_F(ClientServerInProcess, SendUnregisterTranslationUnitsForEditorMessage)
{
    ClangBackEnd::UnregisterTranslationUnitsForEditorMessage message({fileContainer});

    EXPECT_CALL(mockIpcServer, unregisterTranslationUnitsForEditor(message))
        .Times(1);

    serverProxy.unregisterTranslationUnitsForEditor(message);
    scheduleServerMessages();
}

TEST_F(ClientServerInProcess, SendRegisterUnsavedFilesForEditorMessage)
{
    ClangBackEnd::RegisterUnsavedFilesForEditorMessage message({fileContainer});

    EXPECT_CALL(mockIpcServer, registerUnsavedFilesForEditor(message))
        .Times(1);

    serverProxy.registerUnsavedFilesForEditor(message);
    scheduleServerMessages();
}

TEST_F(ClientServerInProcess, SendUnregisterUnsavedFilesForEditorMessage)
{
    ClangBackEnd::UnregisterUnsavedFilesForEditorMessage message({fileContainer});

    EXPECT_CALL(mockIpcServer, unregisterUnsavedFilesForEditor(message))
        .Times(1);

    serverProxy.unregisterUnsavedFilesForEditor(message);
    scheduleServerMessages();
}

TEST_F(ClientServerInProcess, SendCompleteCodeMessage)
{
    ClangBackEnd::CompleteCodeMessage message(Utf8StringLiteral("foo.cpp"), 24, 33, Utf8StringLiteral("do what I want"));

    EXPECT_CALL(mockIpcServer, completeCode(message))
        .Times(1);

    serverProxy.completeCode(message);
    scheduleServerMessages();
}

TEST_F(ClientServerInProcess, SendRequestDiagnosticsMessage)
{
    ClangBackEnd::RequestDiagnosticsMessage message({Utf8StringLiteral("foo.cpp"),
                                                     Utf8StringLiteral("projectId")});

    EXPECT_CALL(mockIpcServer, requestDiagnostics(message))
        .Times(1);

    serverProxy.requestDiagnostics(message);
    scheduleServerMessages();
}

TEST_F(ClientServerInProcess, SendRequestHighlightingMessage)
{
    ClangBackEnd::RequestHighlightingMessage message({Utf8StringLiteral("foo.cpp"),
                                                     Utf8StringLiteral("projectId")});

    EXPECT_CALL(mockIpcServer, requestHighlighting(message))
        .Times(1);

    serverProxy.requestHighlighting(message);
    scheduleServerMessages();
}

TEST_F(ClientServerInProcess, SendCodeCompletedMessage)
{
    ClangBackEnd::CodeCompletions codeCompletions({Utf8StringLiteral("newFunction()")});
    ClangBackEnd::CodeCompletedMessage message(codeCompletions,
                                               ClangBackEnd::CompletionCorrection::NoCorrection,
                                               1);

    EXPECT_CALL(mockIpcClient, codeCompleted(message))
        .Times(1);

    clientProxy.codeCompleted(message);
    scheduleClientMessages();
}

TEST_F(ClientServerInProcess, SendRegisterProjectPartsForEditorMessage)
{
    ClangBackEnd::ProjectPartContainer projectContainer(Utf8StringLiteral(TESTDATA_DIR"/complete.pro"));
    ClangBackEnd::RegisterProjectPartsForEditorMessage message({projectContainer});

    EXPECT_CALL(mockIpcServer, registerProjectPartsForEditor(message))
        .Times(1);

    serverProxy.registerProjectPartsForEditor(message);
    scheduleServerMessages();
}

TEST_F(ClientServerInProcess, SendUnregisterProjectPartsForEditorMessage)
{
    ClangBackEnd::UnregisterProjectPartsForEditorMessage message({Utf8StringLiteral(TESTDATA_DIR"/complete.pro")});

    EXPECT_CALL(mockIpcServer, unregisterProjectPartsForEditor(message))
        .Times(1);

    serverProxy.unregisterProjectPartsForEditor(message);
    scheduleServerMessages();
}

TEST_F(ClientServerInProcess, UpdateVisibleTranslationUnitsMessage)
{
    ClangBackEnd::UpdateVisibleTranslationUnitsMessage message(Utf8StringLiteral(TESTDATA_DIR"/fileone.cpp"),
                                                               {Utf8StringLiteral(TESTDATA_DIR"/fileone.cpp"),
                                                                Utf8StringLiteral(TESTDATA_DIR"/filetwo.cpp")});

    EXPECT_CALL(mockIpcServer, updateVisibleTranslationUnits(message))
        .Times(1);

    serverProxy.updateVisibleTranslationUnits(message);
    scheduleServerMessages();
}

TEST_F(ClientServerInProcess, SendTranslationUnitDoesNotExistMessage)
{
    ClangBackEnd::TranslationUnitDoesNotExistMessage message(fileContainer);

    EXPECT_CALL(mockIpcClient, translationUnitDoesNotExist(message))
        .Times(1);

    clientProxy.translationUnitDoesNotExist(message);
    scheduleClientMessages();
}


TEST_F(ClientServerInProcess, SendProjectPartDoesNotExistMessage)
{
    ClangBackEnd::ProjectPartsDoNotExistMessage message({Utf8StringLiteral("projectId")});

    EXPECT_CALL(mockIpcClient, projectPartsDoNotExist(message))
        .Times(1);

    clientProxy.projectPartsDoNotExist(message);
    scheduleClientMessages();
}

TEST_F(ClientServerInProcess, SendDiagnosticsChangedMessage)
{
    ClangBackEnd::DiagnosticContainer container(Utf8StringLiteral("don't do that"),
                                                Utf8StringLiteral("warning"),
                                                {Utf8StringLiteral("-Wpadded"), Utf8StringLiteral("-Wno-padded")},
                                                ClangBackEnd::DiagnosticSeverity::Warning,
                                                {Utf8StringLiteral("foo.cpp"), 20u, 103u},
                                                {{{Utf8StringLiteral("foo.cpp"), 20u, 103u}, {Utf8StringLiteral("foo.cpp"), 20u, 110u}}},
                                                {},
                                                {});
    ClangBackEnd::DiagnosticsChangedMessage message(fileContainer,
                                                    {container});

    EXPECT_CALL(mockIpcClient, diagnosticsChanged(message))
        .Times(1);

    clientProxy.diagnosticsChanged(message);
    scheduleClientMessages();
}

TEST_F(ClientServerInProcess, SendHighlightingChangedMessage)
{
    ClangBackEnd::HighlightingMarkContainer container(1, 1, 1, ClangBackEnd::HighlightingType::Keyword);

    ClangBackEnd::HighlightingChangedMessage message(fileContainer,
                                                     {container},
                                                     QVector<SourceRangeContainer>());

    EXPECT_CALL(mockIpcClient, highlightingChanged(message))
        .Times(1);

    clientProxy.highlightingChanged(message);
    scheduleClientMessages();
}

ClientServerInProcess::ClientServerInProcess()
    : serverProxy(&mockIpcClient, &buffer),
      clientProxy(&mockIpcServer, &buffer)
{
}

void ClientServerInProcess::SetUp()
{
    buffer.open(QIODevice::ReadWrite);
}

void ClientServerInProcess::TearDown()
{
    buffer.close();
}

void ClientServerInProcess::scheduleServerMessages()
{
    buffer.seek(0);
    clientProxy.readMessages();
    buffer.buffer().clear();
}

void ClientServerInProcess::scheduleClientMessages()
{
    buffer.seek(0);
    serverProxy.readMessages();
    buffer.buffer().clear();
}

}
