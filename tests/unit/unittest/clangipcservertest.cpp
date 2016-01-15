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

#include <clangipcserver.h>
#include <highlightingchangedmessage.h>
#include <highlightingmarkcontainer.h>
#include <ipcclientproxy.h>
#include <ipcserverproxy.h>
#include <requesthighlightingmessage.h>
#include <translationunitdoesnotexistexception.h>
#include <translationunitparseerrorexception.h>

#include <cmbcodecompletedmessage.h>
#include <cmbcompletecodemessage.h>
#include <cmbechomessage.h>
#include <cmbregisterprojectsforeditormessage.h>
#include <cmbregistertranslationunitsforeditormessage.h>
#include <cmbunregisterprojectsforeditormessage.h>
#include <cmbunregistertranslationunitsforeditormessage.h>
#include <highlightingchangedmessage.h>
#include <diagnosticschangedmessage.h>
#include <projectpartsdonotexistmessage.h>
#include <translationunitdoesnotexistmessage.h>
#include <updatetranslationunitsforeditormessage.h>
#include <updatevisibletranslationunitsmessage.h>

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
using testing::PrintToString;
using testing::_;

namespace {

using ClangBackEnd::RegisterTranslationUnitForEditorMessage;
using ClangBackEnd::UnregisterTranslationUnitsForEditorMessage;
using ClangBackEnd::RegisterProjectPartsForEditorMessage;
using ClangBackEnd::UnregisterProjectPartsForEditorMessage;
using ClangBackEnd::CompleteCodeMessage;
using ClangBackEnd::CodeCompletedMessage;
using ClangBackEnd::CodeCompletion;
using ClangBackEnd::FileContainer;
using ClangBackEnd::ProjectPartContainer;
using ClangBackEnd::TranslationUnitDoesNotExistMessage;
using ClangBackEnd::ProjectPartsDoNotExistMessage;
using ClangBackEnd::UpdateTranslationUnitsForEditorMessage;
using ClangBackEnd::UpdateVisibleTranslationUnitsMessage;
using ClangBackEnd::RequestHighlightingMessage;
using ClangBackEnd::HighlightingChangedMessage;
using ClangBackEnd::HighlightingMarkContainer;

MATCHER_P5(HasDirtyTranslationUnit,
           filePath,
           projectPartId,
           documentRevision,
           isNeedingReparse,
           hasNewDiagnostics,
           std::string(negation ? "isn't" : "is")
           + " translation unit with file path "+ PrintToString(filePath)
           + " and project " + PrintToString(projectPartId)
           + " and document revision " + PrintToString(documentRevision)
           + " and isNeedingReparse = " + PrintToString(isNeedingReparse)
           + " and hasNewDiagnostics = " + PrintToString(hasNewDiagnostics)
           )
{
    auto &&translationUnits = arg.translationUnitsForTestOnly();
    try {
        auto translationUnit = translationUnits.translationUnit(filePath, projectPartId);

        if (translationUnit.documentRevision() == documentRevision) {

            if (translationUnit.hasNewDiagnostics() && !hasNewDiagnostics) {
                *result_listener << "hasNewDiagnostics is true";
                return false;
            } else if (!translationUnit.hasNewDiagnostics() && hasNewDiagnostics) {
                *result_listener << "hasNewDiagnostics is false";
                return false;
            }

            if (translationUnit.isNeedingReparse() && !isNeedingReparse) {
                *result_listener << "isNeedingReparse is true";
                return false;
            } else if (!translationUnit.isNeedingReparse() && isNeedingReparse) {
                *result_listener << "isNeedingReparse is false";
                return false;
            }

            return true;
        }

        *result_listener << "revision number is " << PrintToString(translationUnit.documentRevision());
        return false;

    } catch (...) {
        *result_listener << "has no translation unit";
        return false;
    }
}


class ClangIpcServer : public ::testing::Test
{
protected:
    void SetUp() override;

    void registerFiles();
    void registerProjectPart();
    void changeProjectPartArguments();
    void changeProjectPartArgumentsToWrongValues();
    void updateVisibilty(const Utf8String &currentEditor, const Utf8String &additionalVisibleEditor);
    static const Utf8String unsavedContent(const QString &unsavedFilePath);

protected:
    MockIpcClient mockIpcClient;
    ClangBackEnd::ClangIpcServer clangServer;
    const ClangBackEnd::TranslationUnits &translationUnits = clangServer.translationUnitsForTestOnly();
    const Utf8String projectPartId = Utf8StringLiteral("pathToProjectPart.pro");
    const Utf8String functionTestFilePath = Utf8StringLiteral(TESTDATA_DIR"/complete_extractor_function.cpp");
    const Utf8String variableTestFilePath = Utf8StringLiteral(TESTDATA_DIR"/complete_extractor_variable.cpp");
    const QString unsavedTestFilePath = QStringLiteral(TESTDATA_DIR) + QStringLiteral("/complete_extractor_function_unsaved.cpp");
    const QString updatedUnsavedTestFilePath = QStringLiteral(TESTDATA_DIR) + QStringLiteral("/complete_extractor_function_unsaved_2.cpp");
    const Utf8String parseErrorTestFilePath = Utf8StringLiteral(TESTDATA_DIR"/complete_translationunit_parse_error.cpp");
};

TEST_F(ClangIpcServer, GetCodeCompletion)
{
    CompleteCodeMessage completeCodeMessage(functionTestFilePath,
                                            20,
                                            1,
                                            projectPartId);
    CodeCompletion codeCompletion(Utf8StringLiteral("Function"),
                                  34,
                                  CodeCompletion::FunctionCompletionKind);

    EXPECT_CALL(mockIpcClient, codeCompleted(Property(&CodeCompletedMessage::codeCompletions, Contains(codeCompletion))))
        .Times(1);

    clangServer.completeCode(completeCodeMessage);
}

TEST_F(ClangIpcServer, RequestHighlighting)
{
    RequestHighlightingMessage requestHighlightingMessage({variableTestFilePath, projectPartId});

    HighlightingMarkContainer highlightingMarkContainer(1, 6, 8, ClangBackEnd::HighlightingType::Function);

    EXPECT_CALL(mockIpcClient, highlightingChanged(Property(&HighlightingChangedMessage::highlightingMarks, Contains(highlightingMarkContainer))))
        .Times(1);

    clangServer.requestHighlighting(requestHighlightingMessage);
}

TEST_F(ClangIpcServer, GetCodeCompletionDependingOnArgumets)
{
    CompleteCodeMessage completeCodeMessage(variableTestFilePath,
                                            35,
                                            1,
                                            projectPartId);
    CodeCompletion codeCompletion(Utf8StringLiteral("ArgumentDefinitionVariable"),
                                  34,
                                  CodeCompletion::VariableCompletionKind);

    EXPECT_CALL(mockIpcClient, codeCompleted(Property(&CodeCompletedMessage::codeCompletions, Contains(codeCompletion))))
        .Times(1);

    changeProjectPartArguments();
    clangServer.completeCode(completeCodeMessage);
}

TEST_F(ClangIpcServer, GetTranslationUnitDoesNotExistForEditorOnNonExistingTranslationUnit)
{
    CompleteCodeMessage completeCodeMessage(Utf8StringLiteral("dontexists.cpp"),
                                            34,
                                            1,
                                            projectPartId);
    TranslationUnitDoesNotExistMessage translationUnitDoesNotExistMessage(Utf8StringLiteral("dontexists.cpp"), projectPartId);

    EXPECT_CALL(mockIpcClient, translationUnitDoesNotExist(translationUnitDoesNotExistMessage))
        .Times(1);

    clangServer.completeCode(completeCodeMessage);
}

TEST_F(ClangIpcServer, GetTranslationUnitDoesNotExistForCompletingUnregisteredFile)
{
    CompleteCodeMessage completeCodeMessage(parseErrorTestFilePath,
                                            20,
                                            1,
                                            projectPartId);
    TranslationUnitDoesNotExistMessage translationUnitDoesNotExistMessage(parseErrorTestFilePath, projectPartId);

    EXPECT_CALL(mockIpcClient, translationUnitDoesNotExist(translationUnitDoesNotExistMessage))
        .Times(1);

    clangServer.completeCode(completeCodeMessage);
}

TEST_F(ClangIpcServer, GetCodeCompletionForUnsavedFile)
{
    CompleteCodeMessage completeCodeMessage(functionTestFilePath,
                                            20,
                                            1,
                                            projectPartId);
    CodeCompletion codeCompletion(Utf8StringLiteral("Method2"),
                                  34,
                                  CodeCompletion::FunctionCompletionKind);

    EXPECT_CALL(mockIpcClient, codeCompleted(Property(&CodeCompletedMessage::codeCompletions, Contains(codeCompletion))))
        .Times(1);

    clangServer.completeCode(completeCodeMessage);
}

TEST_F(ClangIpcServer, GetNoCodeCompletionAfterRemovingUnsavedFile)
{
    clangServer.updateTranslationUnitsForEditor(UpdateTranslationUnitsForEditorMessage(
        {FileContainer(functionTestFilePath, projectPartId, Utf8StringVector(), 74)}));
    CompleteCodeMessage completeCodeMessage(functionTestFilePath,
                                            20,
                                            1,
                                            projectPartId);
    CodeCompletion codeCompletion(Utf8StringLiteral("Method2"),
                                  34,
                                  CodeCompletion::FunctionCompletionKind);

    EXPECT_CALL(mockIpcClient, codeCompleted(Property(&CodeCompletedMessage::codeCompletions, Not(Contains(codeCompletion)))))
        .Times(1);

    clangServer.completeCode(completeCodeMessage);
}

TEST_F(ClangIpcServer, GetNewCodeCompletionAfterUpdatingUnsavedFile)
{
    clangServer.updateTranslationUnitsForEditor(UpdateTranslationUnitsForEditorMessage({{functionTestFilePath,
                                                                                         projectPartId,
                                                                                         unsavedContent(updatedUnsavedTestFilePath),
                                                                                         true,
                                                                                         74}}));
    CompleteCodeMessage completeCodeMessage(functionTestFilePath,
                                            20,
                                            1,
                                            projectPartId);
    CodeCompletion codeCompletion(Utf8StringLiteral("Method3"),
                                  34,
                                  CodeCompletion::FunctionCompletionKind);

    EXPECT_CALL(mockIpcClient, codeCompleted(Property(&CodeCompletedMessage::codeCompletions, Contains(codeCompletion))))
        .Times(1);

    clangServer.completeCode(completeCodeMessage);
}

TEST_F(ClangIpcServer, GetTranslationUnitDoesNotExistForUnregisterTranslationUnitWithWrongFilePath)
{
    FileContainer fileContainer(Utf8StringLiteral("foo.cpp"), projectPartId);
    UnregisterTranslationUnitsForEditorMessage message({fileContainer});
    TranslationUnitDoesNotExistMessage translationUnitDoesNotExistMessage(fileContainer);

    EXPECT_CALL(mockIpcClient, translationUnitDoesNotExist(translationUnitDoesNotExistMessage))
        .Times(1);

    clangServer.unregisterTranslationUnitsForEditor(message);
}

TEST_F(ClangIpcServer, UnregisterTranslationUnitAndTestFailingCompletion)
{
    FileContainer fileContainer(functionTestFilePath, projectPartId);
    UnregisterTranslationUnitsForEditorMessage message({fileContainer});
    clangServer.unregisterTranslationUnitsForEditor(message);
    CompleteCodeMessage completeCodeMessage(functionTestFilePath,
                                            20,
                                            1,
                                            projectPartId);
    TranslationUnitDoesNotExistMessage translationUnitDoesNotExistMessage(fileContainer);

    EXPECT_CALL(mockIpcClient, translationUnitDoesNotExist(translationUnitDoesNotExistMessage))
        .Times(1);

    clangServer.completeCode(completeCodeMessage);
}

TEST_F(ClangIpcServer, GetProjectPartDoesNotExistUnregisterProjectPartInexistingProjectPart)
{
    Utf8StringVector inexistingProjectPartFilePath = {Utf8StringLiteral("projectpartsdoesnotexist.pro"), Utf8StringLiteral("project2doesnotexists.pro")};
    UnregisterProjectPartsForEditorMessage unregisterProjectPartsForEditorMessage(inexistingProjectPartFilePath);
    ProjectPartsDoNotExistMessage projectPartsDoNotExistMessage(inexistingProjectPartFilePath);

    EXPECT_CALL(mockIpcClient, projectPartsDoNotExist(projectPartsDoNotExistMessage))
        .Times(1);

    clangServer.unregisterProjectPartsForEditor(unregisterProjectPartsForEditorMessage);
}

TEST_F(ClangIpcServer, GetProjectPartDoesNotExistRegisterTranslationUnitWithInexistingProjectPart)
{
    Utf8String inexistingProjectPartFilePath = Utf8StringLiteral("projectpartsdoesnotexist.pro");
    RegisterTranslationUnitForEditorMessage registerFileForEditorMessage({FileContainer(variableTestFilePath, inexistingProjectPartFilePath)},
                                                                         variableTestFilePath,
                                                                         {variableTestFilePath});
    ProjectPartsDoNotExistMessage projectPartsDoNotExistMessage({inexistingProjectPartFilePath});

    EXPECT_CALL(mockIpcClient, projectPartsDoNotExist(projectPartsDoNotExistMessage))
        .Times(1);

    clangServer.registerTranslationUnitsForEditor(registerFileForEditorMessage);
}

TEST_F(ClangIpcServer, GetProjectPartDoesNotExistUnregisterTranslationUnitWithInexistingProjectPart)
{
    Utf8String inexistingProjectPartFilePath = Utf8StringLiteral("projectpartsdoesnotexist.pro");
    UnregisterTranslationUnitsForEditorMessage unregisterFileForEditorMessage({FileContainer(variableTestFilePath, inexistingProjectPartFilePath)});
    ProjectPartsDoNotExistMessage projectPartsDoNotExistMessage({inexistingProjectPartFilePath});

    EXPECT_CALL(mockIpcClient, projectPartsDoNotExist(projectPartsDoNotExistMessage))
        .Times(1);

    clangServer.unregisterTranslationUnitsForEditor(unregisterFileForEditorMessage);
}

TEST_F(ClangIpcServer, GetProjectPartDoesNotExistForCompletingProjectPartFile)
{
    Utf8String inexistingProjectPartFilePath = Utf8StringLiteral("projectpartsdoesnotexist.pro");
    CompleteCodeMessage completeCodeMessage(variableTestFilePath,
                                            20,
                                            1,
                                            inexistingProjectPartFilePath);
    ProjectPartsDoNotExistMessage projectPartsDoNotExistMessage({inexistingProjectPartFilePath});

    EXPECT_CALL(mockIpcClient, projectPartsDoNotExist(projectPartsDoNotExistMessage))
        .Times(1);

    clangServer.completeCode(completeCodeMessage);
}

TEST_F(ClangIpcServer, GetProjectPartDoesNotExistForCompletingUnregisteredFile)
{
    CompleteCodeMessage completeCodeMessage(parseErrorTestFilePath,
                                            20,
                                            1,
                                            projectPartId);
    TranslationUnitDoesNotExistMessage translationUnitDoesNotExistMessage(parseErrorTestFilePath, projectPartId);

    EXPECT_CALL(mockIpcClient, translationUnitDoesNotExist(translationUnitDoesNotExistMessage))
        .Times(1);

    clangServer.completeCode(completeCodeMessage);
}

TEST_F(ClangIpcServer, TicketNumberIsForwarded)
{
    CompleteCodeMessage completeCodeMessage(functionTestFilePath,
                                            20,
                                            1,
                                            projectPartId);
    CodeCompletion codeCompletion(Utf8StringLiteral("Function"),
                                  34,
                                  CodeCompletion::FunctionCompletionKind);

    EXPECT_CALL(mockIpcClient, codeCompleted(Property(&CodeCompletedMessage::ticketNumber, Eq(completeCodeMessage.ticketNumber()))))
        .Times(1);

    clangServer.completeCode(completeCodeMessage);
}

TEST_F(ClangIpcServer, TranslationUnitAfterCreationNeedsNoReparseAndHasNoNewDiagnostics)
{
    ASSERT_THAT(clangServer, HasDirtyTranslationUnit(functionTestFilePath, projectPartId, 0U, false, false));
}

TEST_F(ClangIpcServer, SetCurrentAndVisibleEditor)
{
    auto functionTranslationUnit = translationUnits.translationUnit(functionTestFilePath, projectPartId);
    auto variableTranslationUnit = translationUnits.translationUnit(variableTestFilePath, projectPartId);

    updateVisibilty(functionTestFilePath, variableTestFilePath);

    ASSERT_TRUE(functionTranslationUnit.isUsedByCurrentEditor());
    ASSERT_TRUE(functionTranslationUnit.isVisibleInEditor());
    ASSERT_TRUE(variableTranslationUnit.isVisibleInEditor());
}

TEST_F(ClangIpcServer, IsNotCurrentCurrentAndVisibleEditorAnymore)
{
    auto functionTranslationUnit = translationUnits.translationUnit(functionTestFilePath, projectPartId);
    auto variableTranslationUnit = translationUnits.translationUnit(variableTestFilePath, projectPartId);
    updateVisibilty(functionTestFilePath, variableTestFilePath);

    updateVisibilty(variableTestFilePath, Utf8String());

    ASSERT_FALSE(functionTranslationUnit.isUsedByCurrentEditor());
    ASSERT_FALSE(functionTranslationUnit.isVisibleInEditor());
    ASSERT_TRUE(variableTranslationUnit.isUsedByCurrentEditor());
    ASSERT_TRUE(variableTranslationUnit.isVisibleInEditor());
}

void ClangIpcServer::SetUp()
{
    clangServer.addClient(&mockIpcClient);
    registerProjectPart();
    registerFiles();
}

void ClangIpcServer::registerFiles()
{
    RegisterTranslationUnitForEditorMessage message({FileContainer(functionTestFilePath, projectPartId, unsavedContent(unsavedTestFilePath), true),
                                                     FileContainer(variableTestFilePath, projectPartId)},
                                                    functionTestFilePath,
                                                    {functionTestFilePath, variableTestFilePath});

    EXPECT_CALL(mockIpcClient, diagnosticsChanged(_)).Times(2);
    EXPECT_CALL(mockIpcClient, highlightingChanged(_)).Times(2);

    clangServer.registerTranslationUnitsForEditor(message);
}

void ClangIpcServer::registerProjectPart()
{
    RegisterProjectPartsForEditorMessage message({ProjectPartContainer(projectPartId)});

    clangServer.registerProjectPartsForEditor(message);
}

void ClangIpcServer::changeProjectPartArguments()
{
    RegisterProjectPartsForEditorMessage message({ProjectPartContainer(projectPartId, {Utf8StringLiteral("-DArgumentDefinition")})});

    clangServer.registerProjectPartsForEditor(message);
}

void ClangIpcServer::changeProjectPartArgumentsToWrongValues()
{
    RegisterProjectPartsForEditorMessage message({ProjectPartContainer(projectPartId, {Utf8StringLiteral("-blah")})});

    clangServer.registerProjectPartsForEditor(message);
}

void ClangIpcServer::updateVisibilty(const Utf8String &currentEditor, const Utf8String &additionalVisibleEditor)
{
    UpdateVisibleTranslationUnitsMessage message(currentEditor,
                                                 {currentEditor, additionalVisibleEditor});

    clangServer.updateVisibleTranslationUnits(message);
}

const Utf8String ClangIpcServer::unsavedContent(const QString &unsavedFilePath)
{
    QFile unsavedFileContentFile(unsavedFilePath);
    bool isOpen = unsavedFileContentFile.open(QIODevice::ReadOnly | QIODevice::Text);
    if (!isOpen)
        ADD_FAILURE() << "File with the unsaved content cannot be opened!";

    return Utf8String::fromByteArray(unsavedFileContentFile.readAll());
}

TEST_F(ClangIpcServer, TranslationUnitAfterUpdateNeedsReparseAndHasNewDiagnostics)
{
    const auto fileContainer = FileContainer(functionTestFilePath, projectPartId,unsavedContent(unsavedTestFilePath), true, 1);

    clangServer.updateTranslationUnitsForEditor({{fileContainer}});

    ASSERT_THAT(clangServer, HasDirtyTranslationUnit(functionTestFilePath, projectPartId, 1U, true, true));
}

}
