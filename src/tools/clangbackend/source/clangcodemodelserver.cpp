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
#include "clangupdateannotationsjob.h"
#include "codecompleter.h"
#include "diagnosticset.h"
#include "tokenprocessor.h"
#include "clangexceptions.h"
#include "skippedsourceranges.h"
#include "unsavedfile.h"

#include <clangsupport/clangsupportdebugutils.h>
#include <clangsupport/clangcodemodelservermessages.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QDir>

static Q_LOGGING_CATEGORY(serverLog, "qtc.clangbackend.server", QtWarningMsg);

static bool useSupportiveTranslationUnit()
{
    static bool use = !qEnvironmentVariableIntValue("QTC_CLANG_NO_SUPPORTIVE_TRANSLATIONUNIT");
    return use;
}

namespace ClangBackEnd {

ClangCodeModelServer::ClangCodeModelServer()
    : documents(unsavedFiles)
{
    updateAnnotationsTimer.setSingleShot(true);
    QObject::connect(&updateAnnotationsTimer,
                     &QTimer::timeout,
                     [this]() {
        processJobsForVisibleDocuments();
    });

    updateVisibleButNotCurrentDocumentsTimer.setSingleShot(true);
    QObject::connect(&updateVisibleButNotCurrentDocumentsTimer,
                     &QTimer::timeout,
                     [this]() {
        addAndRunUpdateJobs(documents.dirtyAndVisibleButNotCurrentDocuments());
    });

    QObject::connect(documents.clangFileSystemWatcher(),
                     &ClangFileSystemWatcher::fileChanged,
                     [this](const Utf8String &filePath) {
        if (!documents.hasDocument(filePath))
            updateAnnotationsTimer.start(0);
    });
}

void ClangCodeModelServer::end()
{
    QCoreApplication::exit();
}

void ClangCodeModelServer::documentsOpened(const ClangBackEnd::DocumentsOpenedMessage &message)
{
    qCDebug(serverLog) << "########## documentsOpened";
    TIME_SCOPE_DURATION("ClangCodeModelServer::documentsOpened");

    try {
        DocumentResetInfos toReset;
        QVector<FileContainer> toCreate;
        categorizeFileContainers(message.fileContainers, toCreate, toReset);

        std::vector<Document> createdDocuments = documents.create(toCreate);
        for (auto &document : createdDocuments) {
            document.setDirtyIfDependencyIsMet(document.filePath());
            DocumentProcessor processor = documentProcessors().create(document);
            processor.jobs().setJobFinishedCallback([this](const Jobs::RunningJob &a, IAsyncJob *b) {
                return onJobFinished(a, b);
            });
        }
        const std::vector<Document> resetDocuments_ = resetDocuments(toReset);

        unsavedFiles.createOrUpdate(message.fileContainers);
        documents.setUsedByCurrentEditor(message.currentEditorFilePath);
        documents.setVisibleInEditors(message.visibleEditorFilePaths);

        processSuspendResumeJobs(documents.documents());
        processJobsForVisibleDocuments();
    } catch (const std::exception &exception) {
        qWarning() << "Error in ClangCodeModelServer::documentsOpened:" << exception.what();
    }
}

void ClangCodeModelServer::documentsChanged(const DocumentsChangedMessage &message)
{
    qCDebug(serverLog) << "########## documentsChanged";
    TIME_SCOPE_DURATION("ClangCodeModelServer::documentsChanged");

    try {
        const auto newerFileContainers = documents.newerFileContainers(message.fileContainers);
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
            updateAnnotationsTimer.start(0);
        }
    } catch (const std::exception &exception) {
        qWarning() << "Error in ClangCodeModelServer::documentsChanged:" << exception.what();
    }
}

void ClangCodeModelServer::documentsClosed(const ClangBackEnd::DocumentsClosedMessage &message)
{
    qCDebug(serverLog) << "########## documentsClosed";
    TIME_SCOPE_DURATION("ClangCodeModelServer::documentsClosed");

    try {
        for (const auto &fileContainer : message.fileContainers) {
            const Document &document = documents.document(fileContainer);
            documentProcessors().remove(document);
        }
        documents.remove(message.fileContainers);
        unsavedFiles.remove(message.fileContainers);
    } catch (const std::exception &exception) {
        qWarning() << "Error in ClangCodeModelServer::documentsClosed:" << exception.what();
    }
}

void ClangCodeModelServer::unsavedFilesUpdated(const UnsavedFilesUpdatedMessage &message)
{
    qCDebug(serverLog) << "########## unsavedFilesUpdated";
    TIME_SCOPE_DURATION("ClangCodeModelServer::unsavedFilesUpdated");

    try {
        unsavedFiles.createOrUpdate(message.fileContainers);
        documents.updateDocumentsWithChangedDependencies(message.fileContainers);
        resetDocumentsWithUnresolvedIncludes(documents.documents());

        updateAnnotationsTimer.start(updateAnnotationsTimeOutInMs);
    } catch (const std::exception &exception) {
        qWarning() << "Error in ClangCodeModelServer::unsavedFilesUpdated:" << exception.what();
    }
}

void ClangCodeModelServer::unsavedFilesRemoved(const UnsavedFilesRemovedMessage &message)
{
    qCDebug(serverLog) << "########## unsavedFilesRemoved";
    TIME_SCOPE_DURATION("ClangCodeModelServer::unsavedFilesRemoved");

    try {
        unsavedFiles.remove(message.fileContainers);
        documents.updateDocumentsWithChangedDependencies(message.fileContainers);
    } catch (const std::exception &exception) {
        qWarning() << "Error in ClangCodeModelServer::unsavedFilesRemoved:" << exception.what();
    }
}

void ClangCodeModelServer::requestCompletions(const ClangBackEnd::RequestCompletionsMessage &message)
{
    qCDebug(serverLog) << "########## requestCompletions";
    TIME_SCOPE_DURATION("ClangCodeModelServer::requestCompletions");

    try {
        Document document = documents.document(message.filePath);
        DocumentProcessor processor = documentProcessors().processor(document);

        JobRequest jobRequest = processor.createJobRequest(JobRequest::Type::RequestCompletions);
        jobRequest.line = message.line;
        jobRequest.column = message.column;
        jobRequest.funcNameStartLine = message.funcNameStartLine;
        jobRequest.funcNameStartColumn = message.funcNameStartColumn;
        jobRequest.ticketNumber = message.ticketNumber;

        processor.addJob(jobRequest);
        processor.process();
    }  catch (const std::exception &exception) {
        qWarning() << "Error in ClangCodeModelServer::requestCompletions:" << exception.what();
    }
}

void ClangCodeModelServer::requestAnnotations(const RequestAnnotationsMessage &message)
{
    qCDebug(serverLog) << "########## requestAnnotations";
    TIME_SCOPE_DURATION("ClangCodeModelServer::requestAnnotations");

    try {
        auto document = documents.document(message.fileContainer.filePath);

        DocumentProcessor processor = documentProcessors().processor(document);
        processor.addJob(JobRequest::Type::RequestAnnotations);
        processor.addJob(JobRequest::Type::UpdateExtraAnnotations);
        processor.process();
    }  catch (const std::exception &exception) {
        qWarning() << "Error in ClangCodeModelServer::requestAnnotations:" << exception.what();
    }
}

template <class MessageType>
static void fillJobRequest(JobRequest &jobRequest, const MessageType &message)
{
    jobRequest.line = message.line;
    jobRequest.column = message.column;
    jobRequest.ticketNumber = message.ticketNumber;
    jobRequest.textCodecName = message.fileContainer.textCodecName;
    // The unsaved files might get updater later, so take the current
    // revision for the request.
    jobRequest.documentRevision = message.fileContainer.documentRevision;
}

void ClangCodeModelServer::requestReferences(const RequestReferencesMessage &message)
{
    qCDebug(serverLog) << "########## requestReferences";
    TIME_SCOPE_DURATION("ClangCodeModelServer::requestReferences");

    try {
        const Document document = documents.document(message.fileContainer.filePath);
        DocumentProcessor processor = documentProcessors().processor(document);

        JobRequest jobRequest = processor.createJobRequest(JobRequest::Type::RequestReferences);
        fillJobRequest(jobRequest, message);
        jobRequest.localReferences = message.local;
        processor.addJob(jobRequest);
        processor.process();
    }  catch (const std::exception &exception) {
        qWarning() << "Error in ClangCodeModelServer::requestReferences:" << exception.what();
    }
}

void ClangCodeModelServer::requestFollowSymbol(const RequestFollowSymbolMessage &message)
{
    qCDebug(serverLog) << "########## requestFollowSymbol";
    TIME_SCOPE_DURATION("ClangCodeModelServer::requestFollowSymbol");

    try {
        Document document = documents.document(message.fileContainer.filePath);
        DocumentProcessor processor = documentProcessors().processor(document);

        JobRequest jobRequest = processor.createJobRequest(JobRequest::Type::RequestFollowSymbol);
        fillJobRequest(jobRequest, message);
        processor.addJob(jobRequest);
        processor.process();
    }  catch (const std::exception &exception) {
        qWarning() << "Error in ClangCodeModelServer::requestFollowSymbol:" << exception.what();
    }
}

void ClangCodeModelServer::requestToolTip(const RequestToolTipMessage &message)
{
    TIME_SCOPE_DURATION("ClangCodeModelServer::requestToolTip");

    try {
        const Document document = documents.document(message.fileContainer.filePath);
        DocumentProcessor processor = documentProcessors().processor(document);

        JobRequest jobRequest = processor.createJobRequest(JobRequest::Type::RequestToolTip);
        fillJobRequest(jobRequest, message);

        processor.addJob(jobRequest);
        processor.process();
    }  catch (const std::exception &exception) {
        qWarning() << "Error in ClangCodeModelServer::requestToolTip:" << exception.what();
    }
}

void ClangCodeModelServer::documentVisibilityChanged(const DocumentVisibilityChangedMessage &message)
{
    qCDebug(serverLog) << "########## documentVisibilityChanged";
    TIME_SCOPE_DURATION("ClangCodeModelServer::documentVisibilityChanged");

    try {
        documents.setUsedByCurrentEditor(message.currentEditorFilePath);
        documents.setVisibleInEditors(message.visibleEditorFilePaths);
        processSuspendResumeJobs(documents.documents());

        updateAnnotationsTimer.start(0);
    }  catch (const std::exception &exception) {
        qWarning() << "Error in ClangCodeModelServer::documentVisibilityChanged:" << exception.what();
    }
}

const Documents &ClangCodeModelServer::documentsForTestOnly() const
{
    return documents;
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
    return updateAnnotationsTimer.isActive();
}

void ClangCodeModelServer::processJobsForVisibleDocuments()
{
    processJobsForCurrentDocument();
    processTimerForVisibleButNotCurrentDocuments();
}

void ClangCodeModelServer::processJobsForCurrentDocument()
{
    auto currentDocuments = documents.filtered([](const Document &document) {
        return document.isUsedByCurrentEditor() && document.isDirty();
    });
    QTC_CHECK(currentDocuments.size() <= 1);

    addAndRunUpdateJobs(currentDocuments);
}

static void addUpdateAnnotationsJobsAndProcess(DocumentProcessor &processor)
{
    processor.addJob(JobRequest::Type::UpdateAnnotations,
                     PreferredTranslationUnit::PreviouslyParsed);
    processor.addJob(JobRequest::Type::UpdateExtraAnnotations,
                     PreferredTranslationUnit::RecentlyParsed);
    processor.process();
}

void ClangCodeModelServer::addAndRunUpdateJobs(std::vector<Document> documents)
{
    for (auto &document : documents) {
        DocumentProcessor processor = documentProcessors().processor(document);

        // Run the regular edit-reparse-job
        addUpdateAnnotationsJobsAndProcess(processor);

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

void ClangCodeModelServer::processSuspendResumeJobs(const std::vector<Document> &documents)
{
    const SuspendResumeJobs suspendResumeJobs = createSuspendResumeJobs(documents);
    for (const SuspendResumeJobsEntry &entry : suspendResumeJobs) {
        DocumentProcessor processor = documentProcessors().processor(entry.document);
        processor.addJob(entry.jobRequestType, entry.preferredTranslationUnit);
        if (entry.jobRequestType == JobRequest::Type::ResumeDocument) {
            processor.addJob(JobRequest::Type::UpdateExtraAnnotations,
                             PreferredTranslationUnit::RecentlyParsed);
        }
        processor.process();
    }
}

bool ClangCodeModelServer::onJobFinished(const Jobs::RunningJob &jobRecord, IAsyncJob *job)
{
    if (jobRecord.jobRequest.type == JobRequest::Type::UpdateAnnotations) {
        const auto updateJob = static_cast<UpdateAnnotationsJob *>(job);
        return resetDocumentsWithUnresolvedIncludes({updateJob->pinnedDocument()});
    }

    return false;
}

void ClangCodeModelServer::categorizeFileContainers(const QVector<FileContainer> &fileContainers,
                                                    QVector<FileContainer> &toCreate,
                                                    DocumentResetInfos &toReset) const
{
    for (const FileContainer &fileContainer : fileContainers) {
        const std::vector<Document> matching = documents.filtered([&](const Document &document) {
            return document.filePath() == fileContainer.filePath;
        });
        if (matching.empty())
            toCreate.push_back(fileContainer);
        else
            toReset.push_back(DocumentResetInfo{*matching.begin(), fileContainer});
    }
}

std::vector<Document> ClangCodeModelServer::resetDocuments(const DocumentResetInfos &infos)
{
    std::vector<Document> newDocuments;

    for (const DocumentResetInfo &info : infos) {
        const Document &document = info.documentToRemove;
        QTC_CHECK(document.filePath() == info.fileContainer.filePath);

        documents.remove({document.fileContainer()});

        Document newDocument = *documents.create({info.fileContainer}).begin();
        newDocument.setDirtyIfDependencyIsMet(document.filePath());
        newDocument.setIsUsedByCurrentEditor(document.isUsedByCurrentEditor());
        newDocument.setIsVisibleInEditor(document.isVisibleInEditor(), document.visibleTimePoint());
        newDocument.setResponsivenessIncreaseNeeded(document.isResponsivenessIncreased());

        documentProcessors().reset(document, newDocument);

        newDocuments.push_back(newDocument);
    }

    return newDocuments;
}

static bool isDocumentWithUnresolvedIncludesFixable(const Document &document,
                                                    const UnsavedFiles &unsavedFiles)
{
    for (uint i = 0; i < unsavedFiles.count(); ++i) {
        const UnsavedFile &unsavedFile = unsavedFiles.at(i);
        const Utf8String unsavedFilePath = QDir::cleanPath(unsavedFile.filePath());

        for (const Utf8String &unresolvedPath : document.unresolvedFilePaths()) {
            const QString documentDir = QFileInfo(document.filePath()).absolutePath();
            const QString candidate = QDir::cleanPath(documentDir + "/" + unresolvedPath.toString());

            if (Utf8String(candidate) == unsavedFilePath)
                return true;

            for (const Utf8String &headerPath : document.headerPaths()) {
                Utf8String candidate = headerPath;
                candidate.append(QStringLiteral("/"));
                candidate.append(unresolvedPath);
                candidate = QDir::cleanPath(candidate);
                if (candidate == unsavedFilePath)
                    return true;
            }
        }
    }

    return false;
}

int ClangCodeModelServer::resetDocumentsWithUnresolvedIncludes(
    const std::vector<Document> &documents)
{
    DocumentResetInfos toReset;

    for (const Document &document : documents) {
        if (document.unresolvedFilePaths().isEmpty())
            continue;
        if (isDocumentWithUnresolvedIncludesFixable(document, unsavedFiles))
            toReset << DocumentResetInfo{document, document.fileContainer()};
    }

    resetDocuments(toReset);

    return toReset.size();
}

void ClangCodeModelServer::setUpdateAnnotationsTimeOutInMsForTestsOnly(int value)
{
    updateAnnotationsTimeOutInMs = value;
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
        documentProcessors_.reset(new DocumentProcessors(documents, unsavedFiles, *client()));
    }

    return *documentProcessors_.data();
}

} // namespace ClangBackEnd
