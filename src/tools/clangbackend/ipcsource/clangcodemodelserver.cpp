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

#include "clangcodemodelserver.h"

#include "clangdocuments.h"
#include "clangdocumentsuspenderresumer.h"
#include "clangfilesystemwatcher.h"
#include "clangtranslationunits.h"
#include "codecompleter.h"
#include "diagnosticset.h"
#include "highlightingmarks.h"
#include "clangexceptions.h"
#include "skippedsourceranges.h"

#include <clangsupport/clangsupportdebugutils.h>
#include <clangsupport/clangcodemodelservermessages.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDebug>

static bool useSupportiveTranslationUnit()
{
    static bool use = !qEnvironmentVariableIntValue("QTC_CLANG_NO_SUPPORTIVE_TRANSLATIONUNIT");
    return use;
}

namespace ClangBackEnd {

ClangCodeModelServer::ClangCodeModelServer()
    : documents(projects, unsavedFiles)
{
    updateDocumentAnnotationsTimer.setSingleShot(true);
    QObject::connect(&updateDocumentAnnotationsTimer,
                     &QTimer::timeout,
                     [this]() {
        processJobsForDirtyAndVisibleDocuments();
    });

    updateVisibleButNotCurrentDocumentsTimer.setSingleShot(true);
    QObject::connect(&updateVisibleButNotCurrentDocumentsTimer,
                     &QTimer::timeout,
                     [this]() {
        processJobsForDirtyAndVisibleButNotCurrentDocuments();
    });

    QObject::connect(documents.clangFileSystemWatcher(),
                     &ClangFileSystemWatcher::fileChanged,
                     [this](const Utf8String &filePath) {
        ClangCodeModelServer::startDocumentAnnotationsTimerIfFileIsNotOpenAsDocument(filePath);
    });
}

void ClangCodeModelServer::end()
{
    QCoreApplication::exit();
}

void ClangCodeModelServer::registerTranslationUnitsForEditor(const ClangBackEnd::RegisterTranslationUnitForEditorMessage &message)
{
    TIME_SCOPE_DURATION("ClangCodeModelServer::registerTranslationUnitsForEditor");

    try {
        auto createdDocuments = documents.create(message.fileContainers());
        for (const auto &document : createdDocuments)
            documentProcessors().create(document);
        unsavedFiles.createOrUpdate(message.fileContainers());
        documents.setUsedByCurrentEditor(message.currentEditorFilePath());
        documents.setVisibleInEditors(message.visibleEditorFilePaths());
        processSuspendResumeJobs(documents.documents());

        processInitialJobsForDocuments(createdDocuments);
    } catch (const std::exception &exception) {
        qWarning() << "Error in ClangCodeModelServer::registerTranslationUnitsForEditor:" << exception.what();
    }
}

void ClangCodeModelServer::updateTranslationUnitsForEditor(const UpdateTranslationUnitsForEditorMessage &message)
{
    TIME_SCOPE_DURATION("ClangCodeModelServer::updateTranslationUnitsForEditor");

    try {
        const auto newerFileContainers = documents.newerFileContainers(message.fileContainers());
        if (newerFileContainers.size() > 0) {
            std::vector<Document> updateDocuments = documents.update(newerFileContainers);
            unsavedFiles.createOrUpdate(newerFileContainers);

            for (Document &document : updateDocuments) {
                if (!document.isResponsivenessIncreased())
                    document.setResponsivenessIncreaseNeeded(true);
            }

            // Start the jobs on the next event loop iteration since otherwise
            // we might block the translation unit for a completion request
            // that comes right after this message.
            updateDocumentAnnotationsTimer.start(0);
        }
    } catch (const std::exception &exception) {
        qWarning() << "Error in ClangCodeModelServer::updateTranslationUnitsForEditor:" << exception.what();
    }
}

void ClangCodeModelServer::unregisterTranslationUnitsForEditor(const ClangBackEnd::UnregisterTranslationUnitsForEditorMessage &message)
{
    TIME_SCOPE_DURATION("ClangCodeModelServer::unregisterTranslationUnitsForEditor");

    try {
        for (const auto &fileContainer : message.fileContainers()) {
            const Document &document = documents.document(fileContainer);
            documentProcessors().remove(document);
        }
        documents.remove(message.fileContainers());
        unsavedFiles.remove(message.fileContainers());
    } catch (const std::exception &exception) {
        qWarning() << "Error in ClangCodeModelServer::unregisterTranslationUnitsForEditor:" << exception.what();
    }
}

void ClangCodeModelServer::registerProjectPartsForEditor(const RegisterProjectPartsForEditorMessage &message)
{
    TIME_SCOPE_DURATION("ClangCodeModelServer::registerProjectPartsForEditor");

    try {
        projects.createOrUpdate(message.projectContainers());
        std::vector<Document> affectedDocuments = documents.setDocumentsDirtyIfProjectPartChanged();

        for (Document &document : affectedDocuments) {
            document.setResponsivenessIncreaseNeeded(document.isResponsivenessIncreased());

            documentProcessors().remove(document);
            document.translationUnits().removeAll();
            document.translationUnits().createAndAppend();
            documentProcessors().create(document);
        }

        processJobsForDirtyAndVisibleDocuments();
    } catch (const std::exception &exception) {
        qWarning() << "Error in ClangCodeModelServer::registerProjectPartsForEditor:" << exception.what();
    }
}

void ClangCodeModelServer::unregisterProjectPartsForEditor(const UnregisterProjectPartsForEditorMessage &message)
{
    TIME_SCOPE_DURATION("ClangCodeModelServer::unregisterProjectPartsForEditor");

    try {
        projects.remove(message.projectPartIds());
    } catch (const std::exception &exception) {
        qWarning() << "Error in ClangCodeModelServer::unregisterProjectPartsForEditor:" << exception.what();
    }
}

void ClangCodeModelServer::registerUnsavedFilesForEditor(const RegisterUnsavedFilesForEditorMessage &message)
{
    TIME_SCOPE_DURATION("ClangCodeModelServer::registerUnsavedFilesForEditor");

    try {
        unsavedFiles.createOrUpdate(message.fileContainers());
        documents.updateDocumentsWithChangedDependencies(message.fileContainers());

        updateDocumentAnnotationsTimer.start(updateDocumentAnnotationsTimeOutInMs);
    } catch (const std::exception &exception) {
        qWarning() << "Error in ClangCodeModelServer::registerUnsavedFilesForEditor:" << exception.what();
    }
}

void ClangCodeModelServer::unregisterUnsavedFilesForEditor(const UnregisterUnsavedFilesForEditorMessage &message)
{
    TIME_SCOPE_DURATION("ClangCodeModelServer::unregisterUnsavedFilesForEditor");

    try {
        unsavedFiles.remove(message.fileContainers());
        documents.updateDocumentsWithChangedDependencies(message.fileContainers());
    } catch (const std::exception &exception) {
        qWarning() << "Error in ClangCodeModelServer::unregisterUnsavedFilesForEditor:" << exception.what();
    }
}

void ClangCodeModelServer::completeCode(const ClangBackEnd::CompleteCodeMessage &message)
{
    TIME_SCOPE_DURATION("ClangCodeModelServer::completeCode");

    try {
        Document document = documents.document(message.filePath(), message.projectPartId());
        DocumentProcessor processor = documentProcessors().processor(document);

        JobRequest jobRequest = processor.createJobRequest(JobRequest::Type::CompleteCode);
        jobRequest.line = message.line();
        jobRequest.column = message.column();
        jobRequest.funcNameStartLine = message.funcNameStartLine();
        jobRequest.funcNameStartColumn = message.funcNameStartColumn();
        jobRequest.ticketNumber = message.ticketNumber();

        processor.addJob(jobRequest);
        processor.process();
    }  catch (const std::exception &exception) {
        qWarning() << "Error in ClangCodeModelServer::completeCode:" << exception.what();
    }
}

void ClangCodeModelServer::requestDocumentAnnotations(const RequestDocumentAnnotationsMessage &message)
{
    TIME_SCOPE_DURATION("ClangCodeModelServer::requestDocumentAnnotations");

    try {
        auto document = documents.document(message.fileContainer().filePath(),
                                           message.fileContainer().projectPartId());

        DocumentProcessor processor = documentProcessors().processor(document);
        processor.addJob(JobRequest::Type::RequestDocumentAnnotations);
        processor.process();
    }  catch (const std::exception &exception) {
        qWarning() << "Error in ClangCodeModelServer::requestDocumentAnnotations:" << exception.what();
    }
}

template <class MessageType>
static void fillJobRequest(JobRequest &jobRequest, const MessageType &message)
{
    jobRequest.line = message.line();
    jobRequest.column = message.column();
    jobRequest.ticketNumber = message.ticketNumber();
    // The unsaved files might get updater later, so take the current
    // revision for the request.
    jobRequest.documentRevision = message.fileContainer().documentRevision();
}

void ClangCodeModelServer::requestReferences(const RequestReferencesMessage &message)
{
    TIME_SCOPE_DURATION("ClangCodeModelServer::requestReferences");

    try {
        const Document document = documents.document(message.fileContainer().filePath(),
                                                     message.fileContainer().projectPartId());
        DocumentProcessor processor = documentProcessors().processor(document);

        JobRequest jobRequest = processor.createJobRequest(JobRequest::Type::RequestReferences);
        fillJobRequest(jobRequest, message);
        processor.addJob(jobRequest);
        processor.process();
    }  catch (const std::exception &exception) {
        qWarning() << "Error in ClangCodeModelServer::requestReferences:" << exception.what();
    }
}

void ClangCodeModelServer::requestFollowSymbol(const RequestFollowSymbolMessage &message)
{
    TIME_SCOPE_DURATION("ClangCodeModelServer::requestFollowSymbol");

    try {
        auto projectPartId = message.fileContainer().projectPartId();
        Document document = documents.document(message.fileContainer().filePath(),
                                                     projectPartId);
        DocumentProcessor processor = documentProcessors().processor(document);

        JobRequest jobRequest = processor.createJobRequest(JobRequest::Type::FollowSymbol);
        fillJobRequest(jobRequest, message);
        jobRequest.dependentFiles = message.dependentFiles();
        processor.addJob(jobRequest);
        processor.process();
    }  catch (const std::exception &exception) {
        qWarning() << "Error in ClangCodeModelServer::requestFollowSymbol:" << exception.what();
    }
}

void ClangCodeModelServer::updateVisibleTranslationUnits(const UpdateVisibleTranslationUnitsMessage &message)
{
    TIME_SCOPE_DURATION("ClangCodeModelServer::updateVisibleTranslationUnits");

    try {
        documents.setUsedByCurrentEditor(message.currentEditorFilePath());
        documents.setVisibleInEditors(message.visibleEditorFilePaths());
        processSuspendResumeJobs(documents.documents());

        updateDocumentAnnotationsTimer.start(0);
    }  catch (const std::exception &exception) {
        qWarning() << "Error in ClangCodeModelServer::updateVisibleTranslationUnits:" << exception.what();
    }
}

const Documents &ClangCodeModelServer::documentsForTestOnly() const
{
    return documents;
}

void ClangCodeModelServer::startDocumentAnnotationsTimerIfFileIsNotOpenAsDocument(const Utf8String &filePath)
{
    if (!documents.hasDocumentWithFilePath(filePath))
        updateDocumentAnnotationsTimer.start(0);
}

QList<Jobs::RunningJob> ClangCodeModelServer::runningJobsForTestsOnly()
{
    return documentProcessors().runningJobs();
}

int ClangCodeModelServer::queueSizeForTestsOnly()
{
    return documentProcessors().queueSize();
}

bool ClangCodeModelServer::isTimerRunningForTestOnly() const
{
    return updateDocumentAnnotationsTimer.isActive();
}

void ClangCodeModelServer::processJobsForDirtyAndVisibleDocuments()
{
    processJobsForDirtyCurrentDocument();
    processTimerForVisibleButNotCurrentDocuments();
}

void ClangCodeModelServer::processJobsForDirtyCurrentDocument()
{
    auto currentDirtyDocuments = documents.filtered([](const Document &document) {
        return document.isDirty() && document.isUsedByCurrentEditor();
    });
    QTC_CHECK(currentDirtyDocuments.size() <= 1);

    addAndRunUpdateJobs(currentDirtyDocuments);
}

void ClangCodeModelServer::addAndRunUpdateJobs(std::vector<Document> documents)
{
    for (auto &document : documents) {
        DocumentProcessor processor = documentProcessors().processor(document);

        // Run the regular edit-reparse-job
        processor.addJob(JobRequest::Type::UpdateDocumentAnnotations,
                         PreferredTranslationUnit::PreviouslyParsed);
        processor.process();

        // If requested, run jobs to increase the responsiveness of the document
        if (useSupportiveTranslationUnit() && document.isResponsivenessIncreaseNeeded()) {
            QTC_CHECK(!document.isResponsivenessIncreased());
            QTC_CHECK(!processor.hasSupportiveTranslationUnit());
            document.setResponsivenessIncreaseNeeded(false);
            processor.startInitializingSupportiveTranslationUnit();
        }
    }
}

void ClangCodeModelServer::processTimerForVisibleButNotCurrentDocuments()
{
    if (documents.dirtyAndVisibleButNotCurrentDocuments().empty()) {
        updateVisibleButNotCurrentDocumentsTimer.stop();
    } else {
        updateVisibleButNotCurrentDocumentsTimer.start(
                    updateVisibleButNotCurrentDocumentsTimeOutInMs);
    }
}

void ClangCodeModelServer::processJobsForDirtyAndVisibleButNotCurrentDocuments()
{
    addAndRunUpdateJobs(documents.dirtyAndVisibleButNotCurrentDocuments());
}

void ClangCodeModelServer::processSuspendResumeJobs(const std::vector<Document> &documents)
{
    const SuspendResumeJobs suspendResumeJobs = createSuspendResumeJobs(documents);
    for (const SuspendResumeJobsEntry &entry : suspendResumeJobs) {
        DocumentProcessor processor = documentProcessors().processor(entry.document);
        processor.addJob(entry.jobRequestType, entry.preferredTranslationUnit);
        processor.process();
    }
}

void ClangCodeModelServer::processInitialJobsForDocuments(const std::vector<Document> &documents)
{
    for (const auto &document : documents) {
        DocumentProcessor processor = documentProcessors().processor(document);
        processor.addJob(JobRequest::Type::UpdateDocumentAnnotations);
        processor.addJob(JobRequest::Type::CreateInitialDocumentPreamble);
        processor.process();
    }
}

void ClangCodeModelServer::setUpdateDocumentAnnotationsTimeOutInMsForTestsOnly(int value)
{
    updateDocumentAnnotationsTimeOutInMs = value;
}

void ClangCodeModelServer::setUpdateVisibleButNotCurrentDocumentsTimeOutInMsForTestsOnly(int value)
{
    updateVisibleButNotCurrentDocumentsTimeOutInMs = value;
}

DocumentProcessors &ClangCodeModelServer::documentProcessors()
{
    if (!documentProcessors_) {
        // DocumentProcessors needs a reference to the client, but the client
        // is not known at construction time of ClangCodeModelServer, so
        // construct DocumentProcessors in a lazy manner.
        documentProcessors_.reset(new DocumentProcessors(documents, unsavedFiles, projects, *client()));
    }

    return *documentProcessors_.data();
}

} // namespace ClangBackEnd
