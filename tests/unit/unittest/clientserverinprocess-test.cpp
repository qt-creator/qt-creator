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
#include "mockclangcodemodelserver.h"

#include <clangcodemodelclientproxy.h>
#include <clangcodemodelserverproxy.h>

#include <clangcodemodelservermessages.h>

#include <readmessageblock.h>
#include <writemessageblock.h>

#include <QBuffer>
#include <QString>
#include <QVariant>

#include <vector>

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
                                              Utf8StringLiteral("unsaved content"),
                                              true,
                                              1};
    QBuffer buffer;
    MockClangCodeModelClient mockClangCodeModelClient;
    MockClangCodeModelServer mockClangCodeModelServer;

    ClangBackEnd::ClangCodeModelServerProxy serverProxy;
    ClangBackEnd::ClangCodeModelClientProxy clientProxy;
};

TEST_F(ClientServerInProcess, SendEndMessage)
{
    EXPECT_CALL(mockClangCodeModelServer, end())
        .Times(1);

    serverProxy.end();
    scheduleServerMessages();
}

TEST_F(ClientServerInProcess, SendAliveMessage)
{
    EXPECT_CALL(mockClangCodeModelClient, alive())
        .Times(1);

    clientProxy.alive();
    scheduleClientMessages();
}

TEST_F(ClientServerInProcess, SendDocumentsOpenedMessage)
{
    ClangBackEnd::DocumentsOpenedMessage message({fileContainer}, filePath, {filePath});

    EXPECT_CALL(mockClangCodeModelServer, documentsOpened(message))
        .Times(1);

    serverProxy.documentsOpened(message);
    scheduleServerMessages();
}

TEST_F(ClientServerInProcess, SendUpdateTranslationUnitsForEditorMessage)
{
    ClangBackEnd::DocumentsChangedMessage message({fileContainer});

    EXPECT_CALL(mockClangCodeModelServer, documentsChanged(message))
        .Times(1);

    serverProxy.documentsChanged(message);
    scheduleServerMessages();
}

TEST_F(ClientServerInProcess, SendDocumentsClosedMessage)
{
    ClangBackEnd::DocumentsClosedMessage message({fileContainer});

    EXPECT_CALL(mockClangCodeModelServer, documentsClosed(message))
        .Times(1);

    serverProxy.documentsClosed(message);
    scheduleServerMessages();
}

TEST_F(ClientServerInProcess, SendRegisterUnsavedFilesForEditorMessage)
{
    ClangBackEnd::UnsavedFilesUpdatedMessage message({fileContainer});

    EXPECT_CALL(mockClangCodeModelServer, unsavedFilesUpdated(message))
        .Times(1);

    serverProxy.unsavedFilesUpdated(message);
    scheduleServerMessages();
}

TEST_F(ClientServerInProcess, SendUnregisterUnsavedFilesForEditorMessage)
{
    ClangBackEnd::UnsavedFilesRemovedMessage message({fileContainer});

    EXPECT_CALL(mockClangCodeModelServer, unsavedFilesRemoved(message))
        .Times(1);

    serverProxy.unsavedFilesRemoved(message);
    scheduleServerMessages();
}

TEST_F(ClientServerInProcess, SendCompleteCodeMessage)
{
    ClangBackEnd::RequestCompletionsMessage message(Utf8StringLiteral("foo.cpp"), 24, 33);

    EXPECT_CALL(mockClangCodeModelServer, requestCompletions(message))
        .Times(1);

    serverProxy.requestCompletions(message);
    scheduleServerMessages();
}

TEST_F(ClientServerInProcess, SendRequestAnnotationsMessage)
{
    ClangBackEnd::RequestAnnotationsMessage message(
        {Utf8StringLiteral("foo.cpp"), Utf8StringLiteral("projectId")});

    EXPECT_CALL(mockClangCodeModelServer, requestAnnotations(message))
        .Times(1);

    serverProxy.requestAnnotations(message);
    scheduleServerMessages();
}

TEST_F(ClientServerInProcess, SendCompletionsMessage)
{
    ClangBackEnd::CodeCompletions codeCompletions({Utf8StringLiteral("newFunction()")});
    ClangBackEnd::CompletionsMessage message(codeCompletions, 1);

    EXPECT_CALL(mockClangCodeModelClient, completions(message))
        .Times(1);

    clientProxy.completions(message);
    scheduleClientMessages();
}

TEST_F(ClientServerInProcess, DocumentVisibilityChangedMessage)
{
    ClangBackEnd::DocumentVisibilityChangedMessage
        message(Utf8StringLiteral(TESTDATA_DIR "/fileone.cpp"),
                {Utf8StringLiteral(TESTDATA_DIR "/fileone.cpp"),
                 Utf8StringLiteral(TESTDATA_DIR "/filetwo.cpp")});

    EXPECT_CALL(mockClangCodeModelServer, documentVisibilityChanged(message))
        .Times(1);

    serverProxy.documentVisibilityChanged(message);
    scheduleServerMessages();
}

TEST_F(ClientServerInProcess, SendAnnotationsMessage)
{
    ClangBackEnd::HighlightingTypes types;
    types.mainHighlightingType = ClangBackEnd::HighlightingType::Keyword;
    ClangBackEnd::TokenInfoContainer tokenInfo(1, 1, 1, types);
    ClangBackEnd::DiagnosticContainer diagnostic(Utf8StringLiteral("don't do that"),
                                                Utf8StringLiteral("warning"),
                                                {Utf8StringLiteral("-Wpadded"), Utf8StringLiteral("-Wno-padded")},
                                                ClangBackEnd::DiagnosticSeverity::Warning,
                                                {Utf8StringLiteral("foo.cpp"), 20u, 103u},
                                                {{{Utf8StringLiteral("foo.cpp"), 20u, 103u}, {Utf8StringLiteral("foo.cpp"), 20u, 110u}}},
                                                {},
                                                {});

    ClangBackEnd::AnnotationsMessage message(fileContainer,
                                             {diagnostic},
                                             {},
                                             {tokenInfo},
                                             QVector<SourceRangeContainer>());

    EXPECT_CALL(mockClangCodeModelClient, annotations(message))
        .Times(1);

    clientProxy.annotations(message);
    scheduleClientMessages();
}

ClientServerInProcess::ClientServerInProcess()
    : serverProxy(&mockClangCodeModelClient, &buffer),
      clientProxy(&mockClangCodeModelServer, &buffer)
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
