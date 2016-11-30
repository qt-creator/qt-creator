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

#include "mockrefactoringclient.h"
#include "mockrefactoringserver.h"

#include <writemessageblock.h>
#include <refactoringclientproxy.h>
#include <refactoringserverproxy.h>
#include <sourcelocationsforrenamingmessage.h>
#include <sourcerangesanddiagnosticsforquerymessage.h>
#include <requestsourcelocationforrenamingmessage.h>
#include <requestsourcerangesanddiagnosticsforquerymessage.h>

#include <QBuffer>
#include <QString>
#include <QVariant>

#include <vector>

using namespace ClangBackEnd;

namespace {

using ::testing::Args;
using ::testing::Property;
using ::testing::Eq;

class RefactoringClientServerInProcess : public ::testing::Test
{
protected:
    RefactoringClientServerInProcess();

    void SetUp();
    void TearDown();

    void scheduleServerMessages();
    void scheduleClientMessages();

protected:
    QBuffer buffer;
    MockRefactoringClient mockRefactoringClient;
    MockRefactoringServer mockRefactoringServer;

    ClangBackEnd::RefactoringServerProxy serverProxy;
    ClangBackEnd::RefactoringClientProxy clientProxy;
};

TEST_F(RefactoringClientServerInProcess, SendEndMessage)
{
    EXPECT_CALL(mockRefactoringServer, end())
        .Times(1);

    serverProxy.end();
    scheduleServerMessages();
}

TEST_F(RefactoringClientServerInProcess, SendAliveMessage)
{

    EXPECT_CALL(mockRefactoringClient, alive())
        .Times(1);

    clientProxy.alive();
    scheduleClientMessages();
}

TEST_F(RefactoringClientServerInProcess, SendSourceLocationsForRenamingMessage)
{
    ClangBackEnd::SourceLocationsContainer container;
    ClangBackEnd::SourceLocationsForRenamingMessage message("symbolName", std::move(container), 1);

    EXPECT_CALL(mockRefactoringClient, sourceLocationsForRenamingMessage(message))
        .Times(1);

    clientProxy.sourceLocationsForRenamingMessage(message.clone());
    scheduleClientMessages();
}

TEST_F(RefactoringClientServerInProcess, SendRequestSourceLocationsForRenamingMessage)
{
    RequestSourceLocationsForRenamingMessage message{{TESTDATA_DIR, "renamevariable.cpp"},
                                                     1,
                                                     5,
                                                     "int v;\n\nint x = v + 3;\n",
                                                     {"cc", "renamevariable.cpp"},
                                                     1};

    EXPECT_CALL(mockRefactoringServer, requestSourceLocationsForRenamingMessage(message))
        .Times(1);

    serverProxy.requestSourceLocationsForRenamingMessage(message.clone());
    scheduleServerMessages();
}

TEST_F(RefactoringClientServerInProcess, SourceRangesAndDiagnosticsForQueryMessage)
{
    ClangBackEnd::SourceRangesContainer sourceRangesContainer;
    std::vector<ClangBackEnd::DynamicASTMatcherDiagnosticContainer> diagnosticContainers;
    ClangBackEnd::SourceRangesAndDiagnosticsForQueryMessage message(std::move(sourceRangesContainer),
                                                                    std::move(diagnosticContainers));

    EXPECT_CALL(mockRefactoringClient, sourceRangesAndDiagnosticsForQueryMessage(message))
        .Times(1);

    clientProxy.sourceRangesAndDiagnosticsForQueryMessage(message.clone());
    scheduleClientMessages();
}

TEST_F(RefactoringClientServerInProcess, RequestSourceRangesAndDiagnosticsForQueryMessage)
{
    RequestSourceRangesAndDiagnosticsForQueryMessage message{"functionDecl()",
                                                             {{{TESTDATA_DIR, "query_simplefunction.cpp"},
                                                                "void f();",
                                                               {"cc", "query_simplefunction.cpp"},
                                                               1}},
                                                             {{{TESTDATA_DIR, "query_simplefunction.h"},
                                                                "void f();",
                                                               {},
                                                               1}}};

    EXPECT_CALL(mockRefactoringServer, requestSourceRangesAndDiagnosticsForQueryMessage(message))
        .Times(1);

    serverProxy.requestSourceRangesAndDiagnosticsForQueryMessage(message.clone());
    scheduleServerMessages();
}

TEST_F(RefactoringClientServerInProcess, CancelMessage)
{
    EXPECT_CALL(mockRefactoringServer, cancel())
        .Times(1);

    serverProxy.cancel();
    scheduleServerMessages();
}


RefactoringClientServerInProcess::RefactoringClientServerInProcess()
    : serverProxy(&mockRefactoringClient, &buffer),
      clientProxy(&mockRefactoringServer, &buffer)
{
}

void RefactoringClientServerInProcess::SetUp()
{
    buffer.open(QIODevice::ReadWrite);
}

void RefactoringClientServerInProcess::TearDown()
{
    buffer.close();
}

void RefactoringClientServerInProcess::scheduleServerMessages()
{
    buffer.seek(0);
    clientProxy.readMessages();
    buffer.buffer().clear();
}

void RefactoringClientServerInProcess::scheduleClientMessages()
{
    buffer.seek(0);
    serverProxy.readMessages();
    buffer.buffer().clear();
}

}
