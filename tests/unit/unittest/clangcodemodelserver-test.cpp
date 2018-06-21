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
#include <tokeninfocontainer.h>
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

MATCHER_P(PartlyContains, token, "")
{
    for (const auto &item: arg) {
        if (item.types == token.types && item.line == token.line && item.column == token.column
                && item.length == token.length) {
            return true;
        }
    }
    return false;
}

static constexpr int AnnotationJobsMultiplier = 2;

class ClangCodeModelServer : public ::testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;

protected:
    bool waitUntilAllJobsFinished(int timeOutInMs = 10000);

    void updateProjectPart();
    void updateProjectPart(const Utf8String &projectPartId);
    void updateProjectPartWithArguments();

    void updateProjectAndOpenDocument(const Utf8String &filePath,
                                      int expectedAnnotationsMessages = AnnotationJobsMultiplier);
    void updateProjectAndOpenDocumentAndWaitForFinished(
        const Utf8String &filePath, int expectedAnnotationsMessages = AnnotationJobsMultiplier);
    void updateProjectAndOpenDocumentsAndWaitForFinished(
        int expectedAnnotationsdMessages = 2 * AnnotationJobsMultiplier);

    void openDocument(const Utf8String &filePath,
                      int expectedAnnotationsMessages = AnnotationJobsMultiplier);
    void openDocument(const Utf8String &filePath,
                      const Utf8String &projectPartId,
                      int expectedAnnotationsMessages = AnnotationJobsMultiplier);
    void openDocuments(int expectedAnnotationsMessages);
    void openDocumentWithUnsavedContent(const Utf8String &filePath, const Utf8String &content);
    void closeDocument(const Utf8String &filePath);

    void updateUnsavedContent(const Utf8String &filePath,
                              const Utf8String &fileContent,
                              quint32 revisionNumber);
    void removeUnsavedFile(const Utf8String &filePath);

    void updateVisibilty(const Utf8String &currentEditor, const Utf8String &additionalVisibleEditor);

    void requestAnnotations(const Utf8String &filePath);
    void requestReferences(quint32 documentRevision = 0);
    void requestFollowSymbol(quint32 documentRevision = 0);
    void requestCompletions(const Utf8String &filePath,
                            uint line = 1,
                            uint column = 1,
                            const Utf8String &projectPartId = Utf8String());
    void requestCompletionsInFileA();
    void requestCompletionsInFileB();

    bool isSupportiveTranslationUnitInitialized(const Utf8String &filePath);

    DocumentProcessor documentProcessorForFile(const Utf8String &filePath);

    void expectAnnotations(int count);
    void expectCompletion(const CodeCompletion &completion);
    void expectCompletionFromFileA();
    void expectCompletionFromFileBEnabledByMacro();
    void expectCompletionFromFileAUnsavedMethodVersion1();
    void expectCompletionFromFileAUnsavedMethodVersion2();
    void expectNoCompletionWithUnsavedMethod();
    void expectReferences();
    void expectFollowSymbol();
    void expectAnnotationsForFileBWithSpecificHighlightingMark();

    static const Utf8String unsavedContent(const QString &unsavedFilePath);

protected:
    MockClangCodeModelClient mockClangCodeModelClient;
    ClangBackEnd::ClangCodeModelServer clangServer;
    const ClangBackEnd::Documents &documents = clangServer.documentsForTestOnly();
    const Utf8String projectPartId = Utf8StringLiteral("pathToProjectPart.pro");
    const Utf8String projectPartId2 = Utf8StringLiteral("otherPathToProjectPart.pro");

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
    updateProjectAndOpenDocument(filePathA);

    expectCompletionFromFileA();
    requestCompletionsInFileA();
}

TEST_F(ClangCodeModelServerSlowTest, RequestAnnotations)
{
    updateProjectAndOpenDocumentAndWaitForFinished(filePathB);

    expectAnnotationsForFileBWithSpecificHighlightingMark();
    requestAnnotations(filePathB);
}

TEST_F(ClangCodeModelServerSlowTest, RequestReferencesForCurrentDocumentRevision)
{
    updateProjectAndOpenDocumentAndWaitForFinished(filePathC);

    expectReferences();
    requestReferences();
}

TEST_F(ClangCodeModelServerSlowTest, RequestReferencesTakesRevisionFromMessage)
{
    updateProjectAndOpenDocumentAndWaitForFinished(filePathC);

    requestReferences(/*documentRevision=*/ 99);

    JobRequests &queue = documentProcessorForFile(filePathC).queue();
    ASSERT_TRUE(Utils::anyOf(queue, [](const JobRequest &request) {
        return request.documentRevision == 99;
    }));
    queue.clear(); // Avoid blocking
}

TEST_F(ClangCodeModelServerSlowTest, RequestFollowSymbolForCurrentDocumentRevision)
{
    updateProjectAndOpenDocumentAndWaitForFinished(filePathC);

    expectFollowSymbol();
    requestFollowSymbol();
}

TEST_F(ClangCodeModelServerSlowTest, RequestFollowSymbolTakesRevisionFromMessage)
{
    updateProjectAndOpenDocumentAndWaitForFinished(filePathC);

    requestFollowSymbol(/*documentRevision=*/ 99);

    JobRequests &queue = documentProcessorForFile(filePathC).queue();
    ASSERT_TRUE(Utils::anyOf(queue, [](const JobRequest &request) {
        return request.documentRevision == 99;
    }));
    queue.clear(); // Avoid blocking
}

TEST_F(ClangCodeModelServerSlowTest, NoInitialAnnotationsForClosedDocument)
{
    const int expectedAnnotationsCount = 0;
    updateProjectAndOpenDocument(filePathA, expectedAnnotationsCount);

    closeDocument(filePathA);
}

TEST_F(ClangCodeModelServerSlowTest, AnnotationsForInitiallyNotVisibleDocument)
{
    const int expectedAnnotationsCount = 2;
    updateProjectPart();
    updateVisibilty(filePathA, filePathA);
    expectAnnotations(expectedAnnotationsCount);
    clangServer.documentsOpened( // Open document while another is still visible
        DocumentsOpenedMessage({FileContainer(filePathB, projectPartId, Utf8String(), false, 1)},
                               filePathA, {filePathA}));
    clangServer.unsavedFilesUpdated( // Invalidate added jobs
        UnsavedFilesUpdatedMessage({FileContainer(Utf8StringLiteral("aFile"), Utf8String())}));

    updateVisibilty(filePathB, filePathB);
}

TEST_F(ClangCodeModelServerSlowTest, NoAnnotationsForClosedDocument)
{
    const int expectedAnnotationsCount = AnnotationJobsMultiplier; // Only for registration.
    updateProjectAndOpenDocumentAndWaitForFinished(filePathA, expectedAnnotationsCount);
    updateUnsavedContent(filePathA, Utf8String(), 1);

    closeDocument(filePathA);
}

TEST_F(ClangCodeModelServerSlowTest, NoInitialAnnotationsForOutdatedDocumentRevision)
{
    const int expectedAnnotationsCount = AnnotationJobsMultiplier; // Only for registration.
    updateProjectAndOpenDocument(filePathA, expectedAnnotationsCount);

    updateUnsavedContent(filePathA, Utf8String(), 1);
}

TEST_F(ClangCodeModelServerSlowTest, NoCompletionsForClosedDocument)
{
    const int expectedAnnotationsCount = AnnotationJobsMultiplier; // Only for registration.
    updateProjectAndOpenDocumentAndWaitForFinished(filePathA, expectedAnnotationsCount);
    requestCompletionsInFileA();

    closeDocument(filePathA);
}

TEST_F(ClangCodeModelServerSlowTest, CodeCompletionDependingOnProject)
{
    const int expectedAnnotationsCount = 2 * AnnotationJobsMultiplier; // For registration and due to project change.
    updateProjectAndOpenDocumentAndWaitForFinished(filePathB, expectedAnnotationsCount);

    expectCompletionFromFileBEnabledByMacro();
    updateProjectPartWithArguments();
    requestCompletionsInFileB();
}

TEST_F(ClangCodeModelServerSlowTest, GetCodeCompletionForUnsavedFile)
{
    updateProjectPart();
    expectAnnotations(AnnotationJobsMultiplier);
    openDocumentWithUnsavedContent(filePathA, unsavedContent(filePathAUnsavedVersion1));
    expectCompletionFromFileAUnsavedMethodVersion1();

    requestCompletionsInFileA();
}

TEST_F(ClangCodeModelServerSlowTest, GetNoCodeCompletionAfterRemovingUnsavedFile)
{
    const int expectedAnnotationsCount = 2 * AnnotationJobsMultiplier; // For registration and update/removal.
    updateProjectAndOpenDocumentAndWaitForFinished(filePathA, expectedAnnotationsCount);
    removeUnsavedFile(filePathA);

    expectNoCompletionWithUnsavedMethod();
    requestCompletionsInFileA();
}

TEST_F(ClangCodeModelServerSlowTest, GetNewCodeCompletionAfterUpdatingUnsavedFile)
{
    const int expectedAnnotationsCount = 2 * AnnotationJobsMultiplier; // For registration and update/removal.
    updateProjectAndOpenDocumentAndWaitForFinished(filePathA, expectedAnnotationsCount);
    updateUnsavedContent(filePathA, unsavedContent(filePathAUnsavedVersion2), 1);

    expectCompletionFromFileAUnsavedMethodVersion2();
    requestCompletionsInFileA();
}

TEST_F(ClangCodeModelServerSlowTest, TranslationUnitAfterCreationIsNotDirty)
{
    updateProjectAndOpenDocument(filePathA, AnnotationJobsMultiplier);

    ASSERT_THAT(clangServer, HasDirtyDocument(filePathA, projectPartId, 0U, false, false));
}

TEST_F(ClangCodeModelServerSlowTest, SetCurrentAndVisibleEditor)
{
    updateProjectAndOpenDocumentsAndWaitForFinished();
    auto functionDocument = documents.document(filePathA, projectPartId);
    auto variableDocument = documents.document(filePathB, projectPartId);

    updateVisibilty(filePathB, filePathA);

    ASSERT_TRUE(variableDocument.isUsedByCurrentEditor());
    ASSERT_TRUE(variableDocument.isVisibleInEditor());
    ASSERT_TRUE(functionDocument.isVisibleInEditor());
}

TEST_F(ClangCodeModelServerSlowTest, StartCompletionJobFirstOnEditThatTriggersCompletion)
{
    updateProjectAndOpenDocument(filePathA, 2 * AnnotationJobsMultiplier);
    ASSERT_TRUE(waitUntilAllJobsFinished());
    expectCompletionFromFileA();

    updateUnsavedContent(filePathA, unsavedContent(filePathAUnsavedVersion2), 1);
    requestCompletionsInFileA();

    const QList<Jobs::RunningJob> jobs = clangServer.runningJobsForTestsOnly();
    ASSERT_THAT(jobs.size(), Eq(1));
    ASSERT_THAT(jobs.first().jobRequest.type, Eq(JobRequest::Type::RequestCompletions));
}

TEST_F(ClangCodeModelServerSlowTest, SupportiveTranslationUnitNotInitializedAfterRegister)
{
    updateProjectAndOpenDocument(filePathA, AnnotationJobsMultiplier);

    ASSERT_TRUE(waitUntilAllJobsFinished());
    ASSERT_FALSE(isSupportiveTranslationUnitInitialized(filePathA));
}

TEST_F(ClangCodeModelServerSlowTest, SupportiveTranslationUnitIsSetupAfterFirstEdit)
{
    updateProjectAndOpenDocument(filePathA, 2 * AnnotationJobsMultiplier);
    ASSERT_TRUE(waitUntilAllJobsFinished());

    updateUnsavedContent(filePathA, unsavedContent(filePathAUnsavedVersion2), 1);

    ASSERT_TRUE(waitUntilAllJobsFinished());
    ASSERT_TRUE(isSupportiveTranslationUnitInitialized(filePathA));
}

TEST_F(ClangCodeModelServerSlowTest, DoNotRunDuplicateJobs)
{
    updateProjectAndOpenDocument(filePathA, 3 * AnnotationJobsMultiplier);
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
    updateProjectAndOpenDocument(filePathA, 4 * AnnotationJobsMultiplier);
    ASSERT_TRUE(waitUntilAllJobsFinished());

    for (unsigned revision = 1; revision <= 3; ++revision) {
        updateUnsavedContent(filePathA, unsavedContent(filePathAUnsavedVersion2), revision);
        ASSERT_TRUE(waitUntilAllJobsFinished());
    }
}

TEST_F(ClangCodeModelServerSlowTest, IsNotCurrentCurrentAndVisibleEditorAnymore)
{
    updateProjectAndOpenDocumentsAndWaitForFinished();
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
    updateProjectAndOpenDocumentAndWaitForFinished(filePathA, 2 * AnnotationJobsMultiplier);

    updateUnsavedContent(filePathA, unsavedContent(filePathAUnsavedVersion1), 1U);
    ASSERT_THAT(clangServer, HasDirtyDocument(filePathA, projectPartId, 1U, true, true));
}

TEST_F(ClangCodeModelServerSlowTest, TakeOverJobsOnProjectPartChange)
{
    updateProjectAndOpenDocumentAndWaitForFinished(filePathC, 2 * AnnotationJobsMultiplier);
    updateVisibilty(filePathB, filePathB); // Disable processing jobs
    requestReferences();

    expectReferences();

    updateProjectPartWithArguments(); // Here we do not want to loose the RequestReferences job
    updateVisibilty(filePathC, filePathC); // Enable processing jobs
}

TEST_F(ClangCodeModelServerSlowTest, TakeOverJobsOnProjectPartIdChange)
{
    updateProjectPart(projectPartId);
    updateProjectPart(projectPartId2);
    openDocument(filePathC, projectPartId, 0);
    requestReferences();

    expectReferences();

    openDocument(filePathC, projectPartId2); // Here we do not want to loose the RequestReferences job
}

void ClangCodeModelServer::SetUp()
{
    clangServer.setClient(&mockClangCodeModelClient);
    clangServer.setUpdateAnnotationsTimeOutInMsForTestsOnly(0);
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

void ClangCodeModelServer::updateProjectAndOpenDocumentsAndWaitForFinished(int expectedAnnotationsdMessages)
{
    updateProjectPart();
    openDocuments(expectedAnnotationsdMessages);

    ASSERT_TRUE(waitUntilAllJobsFinished());
}

void ClangCodeModelServer::openDocument(const Utf8String &filePath,
                                        int expectedAnnotationsMessages)
{
    openDocument(filePath, projectPartId, expectedAnnotationsMessages);
}

void ClangCodeModelServer::openDocument(const Utf8String &filePath,
                                        const Utf8String &projectPartId,
                                        int expectedAnnotationsMessages)
{
    const FileContainer fileContainer(filePath, projectPartId);
    const DocumentsOpenedMessage message({fileContainer}, filePath, {filePath});

    expectAnnotations(expectedAnnotationsMessages);

    clangServer.documentsOpened(message);
}

void ClangCodeModelServer::openDocuments(int expectedAnnotationsMessages)
{
    const FileContainer fileContainerA(filePathA, projectPartId);
    const FileContainer fileContainerB(filePathB, projectPartId);
    const DocumentsOpenedMessage message({fileContainerA, fileContainerB},
                                         filePathA,
                                         {filePathA, filePathB});

    expectAnnotations(expectedAnnotationsMessages);

    clangServer.documentsOpened(message);
}

void ClangCodeModelServer::expectAnnotations(int count)
{
    EXPECT_CALL(mockClangCodeModelClient, annotations(_)).Times(count);
}

void ClangCodeModelServer::openDocumentWithUnsavedContent(const Utf8String &filePath,
                                                          const Utf8String &unsavedContent)
{
    const FileContainer fileContainer(filePath, projectPartId, unsavedContent, true);
    const DocumentsOpenedMessage message({fileContainer}, filePath, {filePath});

    clangServer.documentsOpened(message);
}

void ClangCodeModelServer::requestCompletions(const Utf8String &filePath,
                                              uint line,
                                              uint column,
                                              const Utf8String &projectPartId)
{
    Utf8String theProjectPartId = projectPartId;
    if (theProjectPartId.isEmpty())
        theProjectPartId = this->projectPartId;

    const RequestCompletionsMessage message(filePath, line, column, theProjectPartId);

    clangServer.requestCompletions(message);
}

void ClangCodeModelServer::requestCompletionsInFileA()
{
    requestCompletions(filePathA, 20, 1);
}

void ClangCodeModelServer::requestCompletionsInFileB()
{
    requestCompletions(filePathB, 35, 1);
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
                completions(Field(&CompletionsMessage::codeCompletions,
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
                completions(Field(&CompletionsMessage::codeCompletions,
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
                    Field(&ReferencesMessage::references,
                          Eq(references))))
        .Times(1);
}

void ClangCodeModelServer::expectFollowSymbol()
{
    const ClangBackEnd::FollowSymbolResult classDefinition
            = ClangBackEnd::SourceRangeContainer({filePathC, 40, 7}, {filePathC, 40, 10});

    EXPECT_CALL(mockClangCodeModelClient,
                followSymbol(
                    Field(&FollowSymbolMessage::result,
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

void ClangCodeModelServer::requestAnnotations(const Utf8String &filePath)
{
    const RequestAnnotationsMessage message({filePath, projectPartId});

    clangServer.requestAnnotations(message);
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
    const RequestFollowSymbolMessage message{fileContainer, 43, 9};

    clangServer.requestFollowSymbol(message);
}

void ClangCodeModelServer::expectAnnotationsForFileBWithSpecificHighlightingMark()
{
    HighlightingTypes types;
    types.mainHighlightingType = ClangBackEnd::HighlightingType::Function;
    types.mixinHighlightingTypes.push_back(ClangBackEnd::HighlightingType::Declaration);
    types.mixinHighlightingTypes.push_back(ClangBackEnd::HighlightingType::FunctionDefinition);
    const TokenInfoContainer tokenInfo(1, 6, 8, types);
    EXPECT_CALL(mockClangCodeModelClient,
                annotations(
                    Field(&AnnotationsMessage::tokenInfos,
                          PartlyContains(tokenInfo)))).Times(AnnotationJobsMultiplier);
}

void ClangCodeModelServer::updateUnsavedContent(const Utf8String &filePath,
                                                const Utf8String &fileContent,
                                                quint32 revisionNumber)
{
    const FileContainer fileContainer(filePath, projectPartId, fileContent, true, revisionNumber);
    const DocumentsChangedMessage message({fileContainer});

    clangServer.documentsChanged(message);
}

void ClangCodeModelServer::removeUnsavedFile(const Utf8String &filePath)
{
    const FileContainer fileContainer(filePath, projectPartId, Utf8StringVector(), 74);
    const DocumentsChangedMessage message({fileContainer});

    clangServer.documentsChanged(message);
}

void ClangCodeModelServer::closeDocument(const Utf8String &filePath)
{
    const QVector<FileContainer> fileContainers = {FileContainer(filePath, projectPartId)};
    const DocumentsClosedMessage message(fileContainers);

    clangServer.documentsClosed(message);
}

void ClangCodeModelServer::updateProjectPart()
{
    updateProjectPart(projectPartId);
}

void ClangCodeModelServer::updateProjectPart(const Utf8String &projectPartId)
{
    ProjectPartsUpdatedMessage message({ProjectPartContainer(projectPartId)});

    clangServer.projectPartsUpdated(message);
}

void ClangCodeModelServer::updateProjectAndOpenDocument(const Utf8String &filePath,
                                                        int expectedAnnotationsMessages)
{
    updateProjectPart();
    openDocument(filePath, expectedAnnotationsMessages);
}

void ClangCodeModelServer::updateProjectAndOpenDocumentAndWaitForFinished(
    const Utf8String &filePath, int expectedAnnotationsMessages)
{
    updateProjectAndOpenDocument(filePath, expectedAnnotationsMessages);
    ASSERT_TRUE(waitUntilAllJobsFinished());
}

void ClangCodeModelServer::updateProjectPartWithArguments()
{
    const ProjectPartContainer projectPartContainer(projectPartId,
                                                    {Utf8StringLiteral("-DArgumentDefinition")});
    const ProjectPartsUpdatedMessage message({projectPartContainer});

    clangServer.projectPartsUpdated(message);
}

void ClangCodeModelServer::updateVisibilty(const Utf8String &currentEditor,
                                           const Utf8String &additionalVisibleEditor)
{
    const DocumentVisibilityChangedMessage message(currentEditor,
                                                   {currentEditor, additionalVisibleEditor});

    clangServer.documentVisibilityChanged(message);
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
