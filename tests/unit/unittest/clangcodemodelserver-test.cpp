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
#include "processevents-utilities.h"

#include <clangcodemodelserver.h>
#include <highlightingmarkcontainer.h>
#include <clangcodemodelclientproxy.h>
#include <clangcodemodelserverproxy.h>
#include <clangtranslationunits.h>
#include <clangexceptions.h>

#include <clangcodemodelservermessages.h>

#include <utils/algorithm.h>

#include <QCoreApplication>
#include <QFile>

using testing::Property;
using testing::Contains;
using testing::Not;
using testing::Eq;
using testing::PrintToString;
using testing::_;

namespace {

using namespace ClangBackEnd;

MATCHER_P5(HasDirtyDocument,
           filePath,
           projectPartId,
           documentRevision,
           isDirty,
           hasNewDiagnostics,
           std::string(negation ? "isn't" : "is")
           + " document with file path "+ PrintToString(filePath)
           + " and project " + PrintToString(projectPartId)
           + " and document revision " + PrintToString(documentRevision)
           + " and isDirty = " + PrintToString(isDirty)
           + " and hasNewDiagnostics = " + PrintToString(hasNewDiagnostics)
           )
{
    auto &&documents = arg.documentsForTestOnly();
    try {
        auto document = documents.document(filePath, projectPartId);

        if (document.documentRevision() == documentRevision) {
            if (document.isDirty() && !isDirty) {
                *result_listener << "isNeedingReparse is true";
                return false;
            } else if (!document.isDirty() && isDirty) {
                *result_listener << "isNeedingReparse is false";
                return false;
            }

            return true;
        }

        *result_listener << "revision number is " << PrintToString(document.documentRevision());
        return false;

    } catch (...) {
        *result_listener << "has no translation unit";
        return false;
    }
}

class ClangCodeModelServer : public ::testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;

protected:
    bool waitUntilAllJobsFinished(int timeOutInMs = 10000);

    void registerProjectPart();
    void changeProjectPartArguments();

    void registerProjectAndFile(const Utf8String &filePath,
                                int expectedDocumentAnnotationsChangedMessages = 1);
    void registerProjectAndFileAndWaitForFinished(const Utf8String &filePath,
                                                  int expectedDocumentAnnotationsChangedMessages = 1);
    void registerProjectAndFilesAndWaitForFinished(int expectedDocumentAnnotationsChangedMessages = 2);
    void registerFile(const Utf8String &filePath,
                      int expectedDocumentAnnotationsChangedMessages = 1);
    void registerFiles(int expectedDocumentAnnotationsChangedMessages);
    void registerFileWithUnsavedContent(const Utf8String &filePath, const Utf8String &content);

    void updateUnsavedContent(const Utf8String &filePath,
                              const Utf8String &fileContent,
                              quint32 revisionNumber);

    void unregisterFile(const Utf8String &filePath);

    void removeUnsavedFile(const Utf8String &filePath);

    void updateVisibilty(const Utf8String &currentEditor, const Utf8String &additionalVisibleEditor);

    void requestDocumentAnnotations(const Utf8String &filePath);
    void requestReferences(quint32 documentRevision = 0);
    void requestFollowSymbol(quint32 documentRevision = 0);

    void completeCode(const Utf8String &filePath, uint line = 1, uint column = 1,
                      const Utf8String &projectPartId = Utf8String());
    void completeCodeInFileA();
    void completeCodeInFileB();

    bool isSupportiveTranslationUnitInitialized(const Utf8String &filePath);

    DocumentProcessor documentProcessorForFile(const Utf8String &filePath);

    void expectDocumentAnnotationsChanged(int count);
    void expectCompletion(const CodeCompletion &completion);
    void expectCompletionFromFileA();
    void expectCompletionFromFileBEnabledByMacro();
    void expectCompletionFromFileAUnsavedMethodVersion1();
    void expectCompletionFromFileAUnsavedMethodVersion2();
    void expectNoCompletionWithUnsavedMethod();
    void expectReferences();
    void expectFollowSymbol();
    void expectDocumentAnnotationsChangedForFileBWithSpecificHighlightingMark();

    static const Utf8String unsavedContent(const QString &unsavedFilePath);

protected:
    MockClangCodeModelClient mockClangCodeModelClient;
    ClangBackEnd::ClangCodeModelServer clangServer;
    const ClangBackEnd::Documents &documents = clangServer.documentsForTestOnly();
    const Utf8String projectPartId = Utf8StringLiteral("pathToProjectPart.pro");

    const Utf8String filePathA = Utf8StringLiteral(TESTDATA_DIR"/complete_extractor_function.cpp");
    const QString filePathAUnsavedVersion1
        = QStringLiteral(TESTDATA_DIR) + QStringLiteral("/complete_extractor_function_unsaved.cpp");
    const QString filePathAUnsavedVersion2
        = QStringLiteral(TESTDATA_DIR) + QStringLiteral("/complete_extractor_function_unsaved_2.cpp");

    const Utf8String filePathB = Utf8StringLiteral(TESTDATA_DIR"/complete_extractor_variable.cpp");
    const Utf8String filePathC = Utf8StringLiteral(TESTDATA_DIR"/references.cpp");

    const Utf8String aFilePath = Utf8StringLiteral("afile.cpp");
    const Utf8String anExistingFilePath
        = Utf8StringLiteral(TESTDATA_DIR"/complete_translationunit_parse_error.cpp");
    const Utf8String aProjectPartId = Utf8StringLiteral("aproject.pro");
};

using ClangCodeModelServerSlowTest = ClangCodeModelServer;

TEST_F(ClangCodeModelServerSlowTest, GetCodeCompletion)
{
    registerProjectAndFile(filePathA);

    expectCompletionFromFileA();
    completeCodeInFileA();
}

TEST_F(ClangCodeModelServerSlowTest, RequestDocumentAnnotations)
{
    registerProjectAndFileAndWaitForFinished(filePathB);

    expectDocumentAnnotationsChangedForFileBWithSpecificHighlightingMark();
    requestDocumentAnnotations(filePathB);
}

TEST_F(ClangCodeModelServerSlowTest, RequestReferencesForCurrentDocumentRevision)
{
    registerProjectAndFileAndWaitForFinished(filePathC);

    expectReferences();
    requestReferences();
}

TEST_F(ClangCodeModelServerSlowTest, RequestReferencesTakesRevisionFromMessage)
{
    registerProjectAndFileAndWaitForFinished(filePathC);

    requestReferences(/*documentRevision=*/ 99);

    JobRequests &queue = documentProcessorForFile(filePathC).queue();
    Utils::anyOf(queue, [](const JobRequest &request) { return request.documentRevision == 99; });
    queue.clear(); // Avoid blocking
}

TEST_F(ClangCodeModelServerSlowTest, RequestFollowSymbolForCurrentDocumentRevision)
{
    registerProjectAndFileAndWaitForFinished(filePathC);

    expectFollowSymbol();
    requestFollowSymbol();
}

TEST_F(ClangCodeModelServerSlowTest, RequestFollowSymbolTakesRevisionFromMessage)
{
    registerProjectAndFileAndWaitForFinished(filePathC);

    requestFollowSymbol(/*documentRevision=*/ 99);

    JobRequests &queue = documentProcessorForFile(filePathC).queue();
    Utils::anyOf(queue, [](const JobRequest &request) { return request.documentRevision == 99; });
    queue.clear(); // Avoid blocking
}

TEST_F(ClangCodeModelServerSlowTest, NoInitialDocumentAnnotationsForClosedDocument)
{
    const int expectedDocumentAnnotationsChangedCount = 0;
    registerProjectAndFile(filePathA, expectedDocumentAnnotationsChangedCount);

    unregisterFile(filePathA);
}

TEST_F(ClangCodeModelServerSlowTest, NoDocumentAnnotationsForClosedDocument)
{
    const int expectedDocumentAnnotationsChangedCount = 1; // Only for registration.
    registerProjectAndFileAndWaitForFinished(filePathA, expectedDocumentAnnotationsChangedCount);
    updateUnsavedContent(filePathA, Utf8String(), 1);

    unregisterFile(filePathA);
}

TEST_F(ClangCodeModelServerSlowTest, NoInitialDocumentAnnotationsForOutdatedDocumentRevision)
{
    const int expectedDocumentAnnotationsChangedCount = 1; // Only for registration.
    registerProjectAndFile(filePathA, expectedDocumentAnnotationsChangedCount);

    updateUnsavedContent(filePathA, Utf8String(), 1);
}

TEST_F(ClangCodeModelServerSlowTest, NoCompletionsForClosedDocument)
{
    const int expectedDocumentAnnotationsChangedCount = 1; // Only for registration.
    registerProjectAndFileAndWaitForFinished(filePathA, expectedDocumentAnnotationsChangedCount);
    completeCodeInFileA();

    unregisterFile(filePathA);
}

TEST_F(ClangCodeModelServerSlowTest, CodeCompletionDependingOnProject)
{
    const int expectedDocumentAnnotationsChangedCount = 2; // For registration and due to project change.
    registerProjectAndFileAndWaitForFinished(filePathB, expectedDocumentAnnotationsChangedCount);

    expectCompletionFromFileBEnabledByMacro();
    changeProjectPartArguments();
    completeCodeInFileB();
}

TEST_F(ClangCodeModelServerSlowTest, GetCodeCompletionForUnsavedFile)
{
    registerProjectPart();
    expectDocumentAnnotationsChanged(1);
    registerFileWithUnsavedContent(filePathA, unsavedContent(filePathAUnsavedVersion1));
    expectCompletionFromFileAUnsavedMethodVersion1();

    completeCodeInFileA();
}

TEST_F(ClangCodeModelServerSlowTest, GetNoCodeCompletionAfterRemovingUnsavedFile)
{
    const int expectedDocumentAnnotationsChangedCount = 2; // For registration and update/removal.
    registerProjectAndFileAndWaitForFinished(filePathA, expectedDocumentAnnotationsChangedCount);
    removeUnsavedFile(filePathA);

    expectNoCompletionWithUnsavedMethod();
    completeCodeInFileA();
}

TEST_F(ClangCodeModelServerSlowTest, GetNewCodeCompletionAfterUpdatingUnsavedFile)
{
    const int expectedDocumentAnnotationsChangedCount = 2; // For registration and update/removal.
    registerProjectAndFileAndWaitForFinished(filePathA, expectedDocumentAnnotationsChangedCount);
    updateUnsavedContent(filePathA, unsavedContent(filePathAUnsavedVersion2), 1);

    expectCompletionFromFileAUnsavedMethodVersion2();
    completeCodeInFileA();
}

TEST_F(ClangCodeModelServerSlowTest, TranslationUnitAfterCreationIsNotDirty)
{
    registerProjectAndFile(filePathA, 1);

    ASSERT_THAT(clangServer, HasDirtyDocument(filePathA, projectPartId, 0U, false, false));
}

TEST_F(ClangCodeModelServerSlowTest, SetCurrentAndVisibleEditor)
{
    registerProjectAndFilesAndWaitForFinished();
    auto functionDocument = documents.document(filePathA, projectPartId);
    auto variableDocument = documents.document(filePathB, projectPartId);

    updateVisibilty(filePathB, filePathA);

    ASSERT_TRUE(variableDocument.isUsedByCurrentEditor());
    ASSERT_TRUE(variableDocument.isVisibleInEditor());
    ASSERT_TRUE(functionDocument.isVisibleInEditor());
}

TEST_F(ClangCodeModelServerSlowTest, StartCompletionJobFirstOnEditThatTriggersCompletion)
{
    registerProjectAndFile(filePathA, 2);
    ASSERT_TRUE(waitUntilAllJobsFinished());
    expectCompletionFromFileA();

    updateUnsavedContent(filePathA, unsavedContent(filePathAUnsavedVersion2), 1);
    completeCodeInFileA();

    const QList<Jobs::RunningJob> jobs = clangServer.runningJobsForTestsOnly();
    ASSERT_THAT(jobs.size(), Eq(1));
    ASSERT_THAT(jobs.first().jobRequest.type, Eq(JobRequest::Type::CompleteCode));
}

TEST_F(ClangCodeModelServerSlowTest, SupportiveTranslationUnitNotInitializedAfterRegister)
{
    registerProjectAndFile(filePathA, 1);

    ASSERT_TRUE(waitUntilAllJobsFinished());
    ASSERT_FALSE(isSupportiveTranslationUnitInitialized(filePathA));
}

TEST_F(ClangCodeModelServerSlowTest, SupportiveTranslationUnitIsSetupAfterFirstEdit)
{
    registerProjectAndFile(filePathA, 2);
    ASSERT_TRUE(waitUntilAllJobsFinished());

    updateUnsavedContent(filePathA, unsavedContent(filePathAUnsavedVersion2), 1);

    ASSERT_TRUE(waitUntilAllJobsFinished());
    ASSERT_TRUE(isSupportiveTranslationUnitInitialized(filePathA));
}

TEST_F(ClangCodeModelServerSlowTest, DoNotRunDuplicateJobs)
{
    registerProjectAndFile(filePathA, 3);
    ASSERT_TRUE(waitUntilAllJobsFinished());
    updateUnsavedContent(filePathA, unsavedContent(filePathAUnsavedVersion2), 1);
    ASSERT_TRUE(waitUntilAllJobsFinished());
    ASSERT_TRUE(isSupportiveTranslationUnitInitialized(filePathA));
    updateUnsavedContent(filePathA, unsavedContent(filePathAUnsavedVersion2), 2);
    QCoreApplication::processEvents(); // adds + runs a job
    updateVisibilty(Utf8String(), Utf8String());

    updateVisibilty(filePathA, filePathA); // triggers adding + runnings job on next processevents()
}

TEST_F(ClangCodeModelServerSlowTest, OpenDocumentAndEdit)
{
    registerProjectAndFile(filePathA, 4);
    ASSERT_TRUE(waitUntilAllJobsFinished());

    for (unsigned revision = 1; revision <= 3; ++revision) {
        updateUnsavedContent(filePathA, unsavedContent(filePathAUnsavedVersion2), revision);
        ASSERT_TRUE(waitUntilAllJobsFinished());
    }
}

TEST_F(ClangCodeModelServerSlowTest, IsNotCurrentCurrentAndVisibleEditorAnymore)
{
    registerProjectAndFilesAndWaitForFinished();
    auto functionDocument = documents.document(filePathA, projectPartId);
    auto variableDocument = documents.document(filePathB, projectPartId);
    updateVisibilty(filePathB, filePathA);

    updateVisibilty(filePathB, Utf8String());

    ASSERT_FALSE(functionDocument.isUsedByCurrentEditor());
    ASSERT_FALSE(functionDocument.isVisibleInEditor());
    ASSERT_TRUE(variableDocument.isUsedByCurrentEditor());
    ASSERT_TRUE(variableDocument.isVisibleInEditor());
}

TEST_F(ClangCodeModelServerSlowTest, TranslationUnitAfterUpdateNeedsReparse)
{
    registerProjectAndFileAndWaitForFinished(filePathA, 2);

    updateUnsavedContent(filePathA, unsavedContent(filePathAUnsavedVersion1), 1U);
    ASSERT_THAT(clangServer, HasDirtyDocument(filePathA, projectPartId, 1U, true, true));
}

void ClangCodeModelServer::SetUp()
{
    clangServer.setClient(&mockClangCodeModelClient);
    clangServer.setUpdateDocumentAnnotationsTimeOutInMsForTestsOnly(0);
    clangServer.setUpdateVisibleButNotCurrentDocumentsTimeOutInMsForTestsOnly(0);
}

void ClangCodeModelServer::TearDown()
{
    ASSERT_TRUE(waitUntilAllJobsFinished());
}

bool ClangCodeModelServer::waitUntilAllJobsFinished(int timeOutInMs)
{
    const auto noJobsRunningAnymore = [this]() {
        return clangServer.runningJobsForTestsOnly().isEmpty()
            && clangServer.queueSizeForTestsOnly() == 0
            && !clangServer.isTimerRunningForTestOnly();
    };

    return ProcessEventUtilities::processEventsUntilTrue(noJobsRunningAnymore, timeOutInMs);
}

void ClangCodeModelServer::registerProjectAndFilesAndWaitForFinished(
        int expectedDocumentAnnotationsChangedMessages)
{
    registerProjectPart();
    registerFiles(expectedDocumentAnnotationsChangedMessages);

    ASSERT_TRUE(waitUntilAllJobsFinished());
}

void ClangCodeModelServer::registerFile(const Utf8String &filePath,
                                  int expectedDocumentAnnotationsChangedMessages)
{
    const FileContainer fileContainer(filePath, projectPartId);
    const RegisterTranslationUnitForEditorMessage message({fileContainer}, filePath, {filePath});

    expectDocumentAnnotationsChanged(expectedDocumentAnnotationsChangedMessages);

    clangServer.registerTranslationUnitsForEditor(message);
}

void ClangCodeModelServer::registerFiles(int expectedDocumentAnnotationsChangedMessages)
{
    const FileContainer fileContainerA(filePathA, projectPartId);
    const FileContainer fileContainerB(filePathB, projectPartId);
    const RegisterTranslationUnitForEditorMessage message({fileContainerA,
                                                           fileContainerB},
                                                           filePathA,
                                                           {filePathA, filePathB});

    expectDocumentAnnotationsChanged(expectedDocumentAnnotationsChangedMessages);

    clangServer.registerTranslationUnitsForEditor(message);
}

void ClangCodeModelServer::expectDocumentAnnotationsChanged(int count)
{
    EXPECT_CALL(mockClangCodeModelClient, documentAnnotationsChanged(_)).Times(count);
}

void ClangCodeModelServer::registerFileWithUnsavedContent(const Utf8String &filePath,
                                                    const Utf8String &unsavedContent)
{
    const FileContainer fileContainer(filePath, projectPartId, unsavedContent, true);
    const RegisterTranslationUnitForEditorMessage message({fileContainer}, filePath, {filePath});

    clangServer.registerTranslationUnitsForEditor(message);
}

void ClangCodeModelServer::completeCode(const Utf8String &filePath,
                                  uint line,
                                  uint column,
                                  const Utf8String &projectPartId)
{
    Utf8String theProjectPartId = projectPartId;
    if (theProjectPartId.isEmpty())
        theProjectPartId = this->projectPartId;

    const CompleteCodeMessage message(filePath, line, column, theProjectPartId);

    clangServer.completeCode(message);
}

void ClangCodeModelServer::completeCodeInFileA()
{
    completeCode(filePathA, 20, 1);
}

void ClangCodeModelServer::completeCodeInFileB()
{
    completeCode(filePathB, 35, 1);
}

bool ClangCodeModelServer::isSupportiveTranslationUnitInitialized(const Utf8String &filePath)
{
    Document document = clangServer.documentsForTestOnly().document(filePath, projectPartId);
    DocumentProcessor documentProcessor = clangServer.documentProcessors().processor(document);

    return document.translationUnits().size() == 2
        && documentProcessor.hasSupportiveTranslationUnit()
            && documentProcessor.isSupportiveTranslationUnitInitialized();
}

DocumentProcessor ClangCodeModelServer::documentProcessorForFile(const Utf8String &filePath)
{
    Document document = clangServer.documentsForTestOnly().document(filePath, projectPartId);
    DocumentProcessor documentProcessor = clangServer.documentProcessors().processor(document);

    return documentProcessor;
}

void ClangCodeModelServer::expectCompletion(const CodeCompletion &completion)
{
    EXPECT_CALL(mockClangCodeModelClient,
                codeCompleted(Property(&CodeCompletedMessage::codeCompletions,
                                       Contains(completion))))
            .Times(1);
}

void ClangCodeModelServer::expectCompletionFromFileBEnabledByMacro()
{
    const CodeCompletion completion(Utf8StringLiteral("ArgumentDefinitionVariable"),
                                    34,
                                    CodeCompletion::VariableCompletionKind);

    expectCompletion(completion);
}

void ClangCodeModelServer::expectCompletionFromFileAUnsavedMethodVersion1()
{
    const CodeCompletion completion(Utf8StringLiteral("Method2"),
                                    34,
                                    CodeCompletion::FunctionCompletionKind);

    expectCompletion(completion);
}

void ClangCodeModelServer::expectCompletionFromFileAUnsavedMethodVersion2()
{
    const CodeCompletion completion(Utf8StringLiteral("Method3"),
                                    34,
                                    CodeCompletion::FunctionCompletionKind);

    expectCompletion(completion);
}

void ClangCodeModelServer::expectNoCompletionWithUnsavedMethod()
{
    const CodeCompletion completion(Utf8StringLiteral("Method2"),
                                    34,
                                    CodeCompletion::FunctionCompletionKind);

    EXPECT_CALL(mockClangCodeModelClient,
                codeCompleted(Property(&CodeCompletedMessage::codeCompletions,
                                       Not(Contains(completion)))))
            .Times(1);
}

void ClangCodeModelServer::expectReferences()
{
    const QVector<ClangBackEnd::SourceRangeContainer> references{{
         {filePathC, 3, 9},
         {filePathC, 3, 12}
     }};

    EXPECT_CALL(mockClangCodeModelClient,
                references(
                    Property(&ReferencesMessage::references,
                             Eq(references))))
        .Times(1);
}

void ClangCodeModelServer::expectFollowSymbol()
{
    const ClangBackEnd::SourceRangeContainer classDefinition{
         {filePathC, 40, 7},
         {filePathC, 40, 10}
     };

    EXPECT_CALL(mockClangCodeModelClient,
                followSymbol(
                    Property(&FollowSymbolMessage::sourceRange,
                             Eq(classDefinition))))
        .Times(1);
}

void ClangCodeModelServer::expectCompletionFromFileA()
{
    const CodeCompletion completion(Utf8StringLiteral("Function"),
                                    34,
                                    CodeCompletion::FunctionCompletionKind);

    expectCompletion(completion);
}

void ClangCodeModelServer::requestDocumentAnnotations(const Utf8String &filePath)
{
    const RequestDocumentAnnotationsMessage message({filePath, projectPartId});

    clangServer.requestDocumentAnnotations(message);
}

void ClangCodeModelServer::requestReferences(quint32 documentRevision)
{
    const FileContainer fileContainer{filePathC, projectPartId, Utf8StringVector(),
                                      documentRevision};
    const RequestReferencesMessage message{fileContainer, 3, 9};

    clangServer.requestReferences(message);
}

void ClangCodeModelServer::requestFollowSymbol(quint32 documentRevision)
{
    const FileContainer fileContainer{filePathC, projectPartId, Utf8StringVector(),
                                      documentRevision};
    const RequestFollowSymbolMessage message{fileContainer, QVector<Utf8String>(), 43, 9};

    clangServer.requestFollowSymbol(message);
}

void ClangCodeModelServer::expectDocumentAnnotationsChangedForFileBWithSpecificHighlightingMark()
{
    HighlightingTypes types;
    types.mainHighlightingType = ClangBackEnd::HighlightingType::Function;
    types.mixinHighlightingTypes.push_back(ClangBackEnd::HighlightingType::Declaration);
    const HighlightingMarkContainer highlightingMark(1, 6, 8, types, true);

    EXPECT_CALL(mockClangCodeModelClient,
                documentAnnotationsChanged(
                    Property(&DocumentAnnotationsChangedMessage::highlightingMarks,
                             Contains(highlightingMark))))
        .Times(1);
}

void ClangCodeModelServer::updateUnsavedContent(const Utf8String &filePath,
                                              const Utf8String &fileContent,
                                              quint32 revisionNumber)
{
    const FileContainer fileContainer(filePath, projectPartId, fileContent, true, revisionNumber);
    const UpdateTranslationUnitsForEditorMessage message({fileContainer});

    clangServer.updateTranslationUnitsForEditor(message);
}

void ClangCodeModelServer::removeUnsavedFile(const Utf8String &filePath)
{
    const FileContainer fileContainer(filePath, projectPartId, Utf8StringVector(), 74);
    const UpdateTranslationUnitsForEditorMessage message({fileContainer});

    clangServer.updateTranslationUnitsForEditor(message);
}

void ClangCodeModelServer::unregisterFile(const Utf8String &filePath)
{
    const QVector<FileContainer> fileContainers = {FileContainer(filePath, projectPartId)};
    const UnregisterTranslationUnitsForEditorMessage message(fileContainers);

    clangServer.unregisterTranslationUnitsForEditor(message);
}

void ClangCodeModelServer::registerProjectPart()
{
    RegisterProjectPartsForEditorMessage message({ProjectPartContainer(projectPartId)});

    clangServer.registerProjectPartsForEditor(message);
}

void ClangCodeModelServer::registerProjectAndFile(const Utf8String &filePath,
                                                       int expectedDocumentAnnotationsChangedMessages)
{
    registerProjectPart();
    registerFile(filePath, expectedDocumentAnnotationsChangedMessages);
}

void ClangCodeModelServer::registerProjectAndFileAndWaitForFinished(
        const Utf8String &filePath,
        int expectedDocumentAnnotationsChangedMessages)
{
    registerProjectAndFile(filePath, expectedDocumentAnnotationsChangedMessages);
    ASSERT_TRUE(waitUntilAllJobsFinished());
}

void ClangCodeModelServer::changeProjectPartArguments()
{
    const ProjectPartContainer projectPartContainer(projectPartId,
                                                    {Utf8StringLiteral("-DArgumentDefinition")});
    const RegisterProjectPartsForEditorMessage message({projectPartContainer});

    clangServer.registerProjectPartsForEditor(message);
}

void ClangCodeModelServer::updateVisibilty(const Utf8String &currentEditor,
                                     const Utf8String &additionalVisibleEditor)
{
    const UpdateVisibleTranslationUnitsMessage message(currentEditor,
                                                       {currentEditor, additionalVisibleEditor});

    clangServer.updateVisibleTranslationUnits(message);
}

const Utf8String ClangCodeModelServer::unsavedContent(const QString &unsavedFilePath)
{
    QFile unsavedFileContentFile(unsavedFilePath);
    const bool isOpen = unsavedFileContentFile.open(QIODevice::ReadOnly | QIODevice::Text);
    if (!isOpen)
        ADD_FAILURE() << "File with the unsaved content cannot be opened!";

    return Utf8String::fromByteArray(unsavedFileContentFile.readAll());
}

} // anonymous
