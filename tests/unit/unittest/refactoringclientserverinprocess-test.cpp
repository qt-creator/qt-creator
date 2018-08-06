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
#include <clangrefactoringmessages.h>

#include <QBuffer>
#include <QString>
#include <QVariant>

#include <vector>

using namespace ClangBackEnd;

namespace {

using ::testing::Args;
using ::testing::Property;
using ::testing::Eq;

using ClangBackEnd::RemoveGeneratedFilesMessage;
using ClangBackEnd::RemoveProjectPartsMessage;
using ClangBackEnd::UpdateProjectPartsMessage;
using ClangBackEnd::UpdateGeneratedFilesMessage;
using ClangBackEnd::V2::FileContainer;
using ClangBackEnd::V2::ProjectPartContainer;

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
    EXPECT_CALL(mockRefactoringServer, end());

    serverProxy.end();
    scheduleServerMessages();
}

TEST_F(RefactoringClientServerInProcess, SendAliveMessage)
{
    EXPECT_CALL(mockRefactoringClient, alive());

    clientProxy.alive();
    scheduleClientMessages();
}

TEST_F(RefactoringClientServerInProcess, SendSourceLocationsForRenamingMessage)
{
    ClangBackEnd::SourceLocationsContainer container;
    ClangBackEnd::SourceLocationsForRenamingMessage message("symbolName", std::move(container), 1);

    EXPECT_CALL(mockRefactoringClient, sourceLocationsForRenamingMessage(message));

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

    EXPECT_CALL(mockRefactoringServer, requestSourceLocationsForRenamingMessage(message));

    serverProxy.requestSourceLocationsForRenamingMessage(message.clone());
    scheduleServerMessages();
}

TEST_F(RefactoringClientServerInProcess, SourceRangesAndDiagnosticsForQueryMessage)
{
    ClangBackEnd::SourceRangesContainer sourceRangesContainer;
    std::vector<ClangBackEnd::DynamicASTMatcherDiagnosticContainer> diagnosticContainers;
    ClangBackEnd::SourceRangesAndDiagnosticsForQueryMessage message(std::move(sourceRangesContainer),
                                                                    std::move(diagnosticContainers));

    EXPECT_CALL(mockRefactoringClient, sourceRangesAndDiagnosticsForQueryMessage(message));

    clientProxy.sourceRangesAndDiagnosticsForQueryMessage(message.clone());
    scheduleClientMessages();
}

TEST_F(RefactoringClientServerInProcess, SourceRangesForQueryMessage)
{
    ClangBackEnd::SourceRangesContainer sourceRangesContainer;
    ClangBackEnd::SourceRangesForQueryMessage message(std::move(sourceRangesContainer));

    EXPECT_CALL(mockRefactoringClient, sourceRangesForQueryMessage(message));

    clientProxy.sourceRangesForQueryMessage(message.clone());
    scheduleClientMessages();
}

TEST_F(RefactoringClientServerInProcess, RequestSourceRangesAndDiagnosticsForQueryMessage)
{
    RequestSourceRangesForQueryMessage message{"functionDecl()",
                                               {{{TESTDATA_DIR, "query_simplefunction.cpp"},
                                                  "void f();",
                                                 {"cc", "query_simplefunction.cpp"},
                                                 1}},
                                               {{{TESTDATA_DIR, "query_simplefunction.h"},
                                                  "void f();",
                                                 {},
                                                 1}}};

    EXPECT_CALL(mockRefactoringServer, requestSourceRangesForQueryMessage(message));

    serverProxy.requestSourceRangesForQueryMessage(message.clone());
    scheduleServerMessages();
}

TEST_F(RefactoringClientServerInProcess, RequestSourceRangesForQueryMessage)
{
    RequestSourceRangesForQueryMessage message{"functionDecl()",
                                               {{{TESTDATA_DIR, "query_simplefunction.cpp"},
                                                  "void f();",
                                                 {"cc", "query_simplefunction.cpp"},
                                                 1}},
                                               {{{TESTDATA_DIR, "query_simplefunction.h"},
                                                  "void f();",
                                                 {},
                                                 1}}};

    EXPECT_CALL(mockRefactoringServer, requestSourceRangesForQueryMessage(message));

    serverProxy.requestSourceRangesForQueryMessage(message.clone());
    scheduleServerMessages();
}

TEST_F(RefactoringClientServerInProcess, SendUpdateProjectPartsMessage)
{
    ProjectPartContainer projectPart2{"projectPartId",
                                      {"-x", "c++-header", "-Wno-pragma-once-outside-header"},
                                      {{"DEFINE", "1"}},
                                      {"/includes"},
                                      {{1, 1}},
                                      {{1, 2}}};
    UpdateProjectPartsMessage message{{projectPart2}};

    EXPECT_CALL(mockRefactoringServer, updateProjectParts(message));

    serverProxy.updateProjectParts(message.clone());
    scheduleServerMessages();
}

TEST_F(RefactoringClientServerInProcess, SendUpdateGeneratedFilesMessage)
{
    FileContainer fileContainer{{"/path/to/", "file"}, "content", {}};
    UpdateGeneratedFilesMessage message{{fileContainer}};

    EXPECT_CALL(mockRefactoringServer, updateGeneratedFiles(message));

    serverProxy.updateGeneratedFiles(message.clone());
    scheduleServerMessages();
}

TEST_F(RefactoringClientServerInProcess, SendRemoveProjectPartsMessage)
{
    RemoveProjectPartsMessage message{{"projectPartId1", "projectPartId2"}};

    EXPECT_CALL(mockRefactoringServer, removeProjectParts(message));

    serverProxy.removeProjectParts(message.clone());
    scheduleServerMessages();
}

TEST_F(RefactoringClientServerInProcess, SendRemoveGeneratedFilesMessage)
{
    RemoveGeneratedFilesMessage message{{{"/path/to/", "file"}}};

    EXPECT_CALL(mockRefactoringServer, removeGeneratedFiles(message));

    serverProxy.removeGeneratedFiles(message.clone());
    scheduleServerMessages();
}

TEST_F(RefactoringClientServerInProcess, CancelMessage)
{
    EXPECT_CALL(mockRefactoringServer, cancel());

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
