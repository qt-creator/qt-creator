/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "mockipclient.h"

#include <clangipcserver.h>
#include <ipcclientproxy.h>
#include <ipcserverproxy.h>
#include <translationunitdoesnotexistexception.h>
#include <translationunitparseerrorexception.h>

#include <cmbcodecompletedcommand.h>
#include <cmbcompletecodecommand.h>
#include <cmbechocommand.h>
#include <cmbregisterprojectsforcodecompletioncommand.h>
#include <cmbregistertranslationunitsforcodecompletioncommand.h>
#include <cmbunregisterprojectsforcodecompletioncommand.h>
#include <cmbunregistertranslationunitsforcodecompletioncommand.h>
#include <projectpartsdonotexistcommand.h>
#include <translationunitdoesnotexistcommand.h>

#include <QBuffer>
#include <QFile>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

using testing::Property;
using testing::Contains;
using testing::Not;
using testing::Eq;

namespace {

using ClangBackEnd::RegisterTranslationUnitForCodeCompletionCommand;
using ClangBackEnd::UnregisterTranslationUnitsForCodeCompletionCommand;
using ClangBackEnd::RegisterProjectPartsForCodeCompletionCommand;
using ClangBackEnd::UnregisterProjectPartsForCodeCompletionCommand;
using ClangBackEnd::CompleteCodeCommand;
using ClangBackEnd::CodeCompletedCommand;
using ClangBackEnd::CodeCompletion;
using ClangBackEnd::FileContainer;
using ClangBackEnd::ProjectPartContainer;
using ClangBackEnd::TranslationUnitDoesNotExistCommand;
using ClangBackEnd::ProjectPartsDoNotExistCommand;

class ClangIpcServer : public ::testing::Test
{
protected:
    void SetUp() override;

    void registerFiles();
    void registerProjectPart();
    void changeProjectPartArguments();
    void changeProjectPartArgumentsToWrongValues();
    static const Utf8String unsavedContent(const QString &unsavedFilePath);

protected:
    MockIpcClient mockIpcClient;
    ClangBackEnd::ClangIpcServer clangServer;
    const Utf8String projectPartId = Utf8StringLiteral("pathToProjectPart.pro");
    const Utf8String functionTestFilePath = Utf8StringLiteral(TESTDATA_DIR"/complete_extractor_function.cpp");
    const Utf8String variableTestFilePath = Utf8StringLiteral(TESTDATA_DIR"/complete_extractor_variable.cpp");
    const QString unsavedTestFilePath = QStringLiteral(TESTDATA_DIR) + QStringLiteral("/complete_extractor_function_unsaved.cpp");
    const QString updatedUnsavedTestFilePath = QStringLiteral(TESTDATA_DIR) + QStringLiteral("/complete_extractor_function_unsaved_2.cpp");
    const Utf8String parseErrorTestFilePath = Utf8StringLiteral(TESTDATA_DIR"/complete_translationunit_parse_error.cpp");
};


void ClangIpcServer::SetUp()
{
    clangServer.addClient(&mockIpcClient);
    registerProjectPart();
    registerFiles();
}

void ClangIpcServer::registerFiles()
{
    RegisterTranslationUnitForCodeCompletionCommand command({FileContainer(functionTestFilePath, projectPartId, unsavedContent(unsavedTestFilePath), true),
                                                   FileContainer(variableTestFilePath, projectPartId)});

    clangServer.registerTranslationUnitsForCodeCompletion(command);
}

void ClangIpcServer::registerProjectPart()
{
    RegisterProjectPartsForCodeCompletionCommand command({ProjectPartContainer(projectPartId)});

    clangServer.registerProjectPartsForCodeCompletion(command);
}

void ClangIpcServer::changeProjectPartArguments()
{
    RegisterProjectPartsForCodeCompletionCommand command({ProjectPartContainer(projectPartId, {Utf8StringLiteral("-DArgumentDefinition")})});

    clangServer.registerProjectPartsForCodeCompletion(command);
}

void ClangIpcServer::changeProjectPartArgumentsToWrongValues()
{
    RegisterProjectPartsForCodeCompletionCommand command({ProjectPartContainer(projectPartId, {Utf8StringLiteral("-blah")})});

    clangServer.registerProjectPartsForCodeCompletion(command);
}

const Utf8String ClangIpcServer::unsavedContent(const QString &unsavedFilePath)
{
    QFile unsavedFileContentFile(unsavedFilePath);
    bool isOpen = unsavedFileContentFile.open(QIODevice::ReadOnly | QIODevice::Text);
    if (!isOpen)
        ADD_FAILURE() << "File with the unsaved content cannot be opened!";

    return Utf8String::fromByteArray(unsavedFileContentFile.readAll());
}

TEST_F(ClangIpcServer, GetCodeCompletion)
{
    CompleteCodeCommand completeCodeCommand(functionTestFilePath,
                                            20,
                                            1,
                                            projectPartId);
    CodeCompletion codeCompletion(Utf8StringLiteral("Function"),
                                  34,
                                  CodeCompletion::FunctionCompletionKind);

    EXPECT_CALL(mockIpcClient, codeCompleted(Property(&CodeCompletedCommand::codeCompletions, Contains(codeCompletion))))
        .Times(1);

    clangServer.completeCode(completeCodeCommand);
}

TEST_F(ClangIpcServer, GetCodeCompletionDependingOnArgumets)
{
    CompleteCodeCommand completeCodeCommand(variableTestFilePath,
                                            35,
                                            1,
                                            projectPartId);
    CodeCompletion codeCompletion(Utf8StringLiteral("ArgumentDefinitionVariable"),
                                  34,
                                  CodeCompletion::VariableCompletionKind);

    EXPECT_CALL(mockIpcClient, codeCompleted(Property(&CodeCompletedCommand::codeCompletions, Contains(codeCompletion))))
        .Times(1);

    changeProjectPartArguments();
    clangServer.completeCode(completeCodeCommand);
}

TEST_F(ClangIpcServer, GetTranslationUnitDoesNotExistForCodeCompletionOnNonExistingTranslationUnit)
{
    CompleteCodeCommand completeCodeCommand(Utf8StringLiteral("dontexists.cpp"),
                                            34,
                                            1,
                                            projectPartId);
    TranslationUnitDoesNotExistCommand translationUnitDoesNotExistCommand(Utf8StringLiteral("dontexists.cpp"), projectPartId);

    EXPECT_CALL(mockIpcClient, translationUnitDoesNotExist(translationUnitDoesNotExistCommand))
        .Times(1);

    clangServer.completeCode(completeCodeCommand);
}

TEST_F(ClangIpcServer, GetTranslationUnitDoesNotExistForCompletingUnregisteredFile)
{
    CompleteCodeCommand completeCodeCommand(parseErrorTestFilePath,
                                            20,
                                            1,
                                            projectPartId);
    TranslationUnitDoesNotExistCommand translationUnitDoesNotExistCommand(parseErrorTestFilePath, projectPartId);

    EXPECT_CALL(mockIpcClient, translationUnitDoesNotExist(translationUnitDoesNotExistCommand))
        .Times(1);

    clangServer.completeCode(completeCodeCommand);
}

TEST_F(ClangIpcServer, GetCodeCompletionForUnsavedFile)
{
    CompleteCodeCommand completeCodeCommand(functionTestFilePath,
                                            20,
                                            1,
                                            projectPartId);
    CodeCompletion codeCompletion(Utf8StringLiteral("Method2"),
                                  34,
                                  CodeCompletion::FunctionCompletionKind);

    EXPECT_CALL(mockIpcClient, codeCompleted(Property(&CodeCompletedCommand::codeCompletions, Contains(codeCompletion))))
        .Times(1);

    clangServer.completeCode(completeCodeCommand);
}

TEST_F(ClangIpcServer, GetNoCodeCompletionAfterRemovingUnsavedFile)
{
    clangServer.registerTranslationUnitsForCodeCompletion(RegisterTranslationUnitForCodeCompletionCommand({FileContainer(functionTestFilePath, projectPartId)}));
    CompleteCodeCommand completeCodeCommand(functionTestFilePath,
                                            20,
                                            1,
                                            projectPartId);
    CodeCompletion codeCompletion(Utf8StringLiteral("Method2"),
                                  34,
                                  CodeCompletion::FunctionCompletionKind);

    EXPECT_CALL(mockIpcClient, codeCompleted(Property(&CodeCompletedCommand::codeCompletions, Not(Contains(codeCompletion)))))
        .Times(1);

    clangServer.completeCode(completeCodeCommand);
}

TEST_F(ClangIpcServer, GetNewCodeCompletionAfterUpdatingUnsavedFile)
{
    clangServer.registerTranslationUnitsForCodeCompletion(RegisterTranslationUnitForCodeCompletionCommand({FileContainer(functionTestFilePath,
                                                                                                    projectPartId,
                                                                                                    unsavedContent(updatedUnsavedTestFilePath),
                                                                                                    true)}));
    CompleteCodeCommand completeCodeCommand(functionTestFilePath,
                                            20,
                                            1,
                                            projectPartId);
    CodeCompletion codeCompletion(Utf8StringLiteral("Method3"),
                                  34,
                                  CodeCompletion::FunctionCompletionKind);

    EXPECT_CALL(mockIpcClient, codeCompleted(Property(&CodeCompletedCommand::codeCompletions, Contains(codeCompletion))))
        .Times(1);

    clangServer.completeCode(completeCodeCommand);
}

TEST_F(ClangIpcServer, GetTranslationUnitDoesNotExistForUnregisterTranslationUnitWithWrongFilePath)
{
    FileContainer fileContainer(Utf8StringLiteral("foo.cpp"), projectPartId);
    UnregisterTranslationUnitsForCodeCompletionCommand command({fileContainer});
    TranslationUnitDoesNotExistCommand translationUnitDoesNotExistCommand(fileContainer);

    EXPECT_CALL(mockIpcClient, translationUnitDoesNotExist(translationUnitDoesNotExistCommand))
        .Times(1);

    clangServer.unregisterTranslationUnitsForCodeCompletion(command);
}

TEST_F(ClangIpcServer, UnregisterTranslationUnitAndTestFailingCompletion)
{
    FileContainer fileContainer(functionTestFilePath, projectPartId);
    UnregisterTranslationUnitsForCodeCompletionCommand command({fileContainer});
    clangServer.unregisterTranslationUnitsForCodeCompletion(command);
    CompleteCodeCommand completeCodeCommand(functionTestFilePath,
                                            20,
                                            1,
                                            projectPartId);
    TranslationUnitDoesNotExistCommand translationUnitDoesNotExistCommand(fileContainer);

    EXPECT_CALL(mockIpcClient, translationUnitDoesNotExist(translationUnitDoesNotExistCommand))
        .Times(1);

    clangServer.completeCode(completeCodeCommand);
}

TEST_F(ClangIpcServer, GetProjectPartDoesNotExistUnregisterProjectPartInexistingProjectPart)
{
    Utf8StringVector inexistingProjectPartFilePath = {Utf8StringLiteral("projectpartsdoesnotexist.pro"), Utf8StringLiteral("project2doesnotexists.pro")};
    UnregisterProjectPartsForCodeCompletionCommand unregisterProjectPartsForCodeCompletionCommand(inexistingProjectPartFilePath);
    ProjectPartsDoNotExistCommand projectPartsDoNotExistCommand(inexistingProjectPartFilePath);

    EXPECT_CALL(mockIpcClient, projectPartsDoNotExist(projectPartsDoNotExistCommand))
        .Times(1);

    clangServer.unregisterProjectPartsForCodeCompletion(unregisterProjectPartsForCodeCompletionCommand);
}

TEST_F(ClangIpcServer, GetProjectPartDoesNotExistRegisterTranslationUnitWithInexistingProjectPart)
{
    Utf8String inexistingProjectPartFilePath = Utf8StringLiteral("projectpartsdoesnotexist.pro");
    RegisterTranslationUnitForCodeCompletionCommand registerFileForCodeCompletionCommand({FileContainer(variableTestFilePath, inexistingProjectPartFilePath)});
    ProjectPartsDoNotExistCommand projectPartsDoNotExistCommand({inexistingProjectPartFilePath});

    EXPECT_CALL(mockIpcClient, projectPartsDoNotExist(projectPartsDoNotExistCommand))
        .Times(1);

    clangServer.registerTranslationUnitsForCodeCompletion(registerFileForCodeCompletionCommand);
}

TEST_F(ClangIpcServer, GetProjectPartDoesNotExistUnregisterTranslationUnitWithInexistingProjectPart)
{
    Utf8String inexistingProjectPartFilePath = Utf8StringLiteral("projectpartsdoesnotexist.pro");
    UnregisterTranslationUnitsForCodeCompletionCommand unregisterFileForCodeCompletionCommand({FileContainer(variableTestFilePath, inexistingProjectPartFilePath)});
    ProjectPartsDoNotExistCommand projectPartsDoNotExistCommand({inexistingProjectPartFilePath});

    EXPECT_CALL(mockIpcClient, projectPartsDoNotExist(projectPartsDoNotExistCommand))
        .Times(1);

    clangServer.unregisterTranslationUnitsForCodeCompletion(unregisterFileForCodeCompletionCommand);
}

TEST_F(ClangIpcServer, GetProjectPartDoesNotExistForCompletingProjectPartFile)
{
    Utf8String inexistingProjectPartFilePath = Utf8StringLiteral("projectpartsdoesnotexist.pro");
    CompleteCodeCommand completeCodeCommand(variableTestFilePath,
                                            20,
                                            1,
                                            inexistingProjectPartFilePath);
    ProjectPartsDoNotExistCommand projectPartsDoNotExistCommand({inexistingProjectPartFilePath});

    EXPECT_CALL(mockIpcClient, projectPartsDoNotExist(projectPartsDoNotExistCommand))
        .Times(1);

    clangServer.completeCode(completeCodeCommand);
}

TEST_F(ClangIpcServer, GetProjectPartDoesNotExistForCompletingUnregisteredFile)
{
    CompleteCodeCommand completeCodeCommand(parseErrorTestFilePath,
                                            20,
                                            1,
                                            projectPartId);
    TranslationUnitDoesNotExistCommand translationUnitDoesNotExistCommand(parseErrorTestFilePath, projectPartId);

    EXPECT_CALL(mockIpcClient, translationUnitDoesNotExist(translationUnitDoesNotExistCommand))
        .Times(1);

    clangServer.completeCode(completeCodeCommand);
}

TEST_F(ClangIpcServer, TicketNumberIsForwarded)
{
    CompleteCodeCommand completeCodeCommand(functionTestFilePath,
                                            20,
                                            1,
                                            projectPartId);
    CodeCompletion codeCompletion(Utf8StringLiteral("Function"),
                                  34,
                                  CodeCompletion::FunctionCompletionKind);

    EXPECT_CALL(mockIpcClient, codeCompleted(Property(&CodeCompletedCommand::ticketNumber, Eq(completeCodeCommand.ticketNumber()))))
        .Times(1);

    clangServer.completeCode(completeCodeCommand);
}
}
