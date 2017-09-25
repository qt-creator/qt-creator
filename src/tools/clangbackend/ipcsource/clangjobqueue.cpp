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

#include "clangiasyncjob.h"
#include "clangjobqueue.h"
#include "clangdocument.h"
#include "clangdocuments.h"
#include "clangtranslationunits.h"
#include "projects.h"
#include "unsavedfiles.h"

#include <utils/algorithm.h>

namespace ClangBackEnd {

JobQueue::JobQueue(Documents &documents, ProjectParts &projectParts)
    : m_documents(documents)
    , m_projectParts(projectParts)
{
}

bool JobQueue::add(const JobRequest &job)
{
    if (m_queue.contains(job)) {
        qCDebug(jobsLog) << "Not adding duplicate request" << job;
        return false;
    }

    if (isJobRunningForJobRequest(job)) {
        qCDebug(jobsLog) << "Not adding duplicate request for already running job" << job;
        return false;
    }

    if (!m_documents.hasDocument(job.filePath, job.projectPartId)) {
        qCDebug(jobsLog) << "Not adding / cancelling due to already closed document:" << job;
        cancelJobRequest(job);
        return false;
    }

    const Document document = m_documents.document(job.filePath, job.projectPartId);
    if (!document.isIntact()) {
        qCDebug(jobsLog) << "Not adding / cancelling due not intact document:" << job;
        cancelJobRequest(job);
        return false;
    }

    qCDebug(jobsLog) << "Adding" << job;
    m_queue.append(job);

    return true;
}

int JobQueue::size() const
{
    return m_queue.size();
}

JobRequests JobQueue::processQueue()
{
    removeExpiredRequests();
    prioritizeRequests();
    const JobRequests jobsToRun = takeJobRequestsToRunNow();

    return jobsToRun;
}

void JobQueue::removeExpiredRequests()
{
    JobRequests cleanedRequests;

    foreach (const JobRequest &jobRequest, m_queue) {
        try {
            if (!isJobRequestExpired(jobRequest))
                cleanedRequests.append(jobRequest);
        } catch (const std::exception &exception) {
            qWarning() << "Error in Jobs::removeOutDatedRequests for"
                       << jobRequest << ":" << exception.what();
        }
    }

    m_queue = cleanedRequests;
}

bool JobQueue::isJobRequestExpired(const JobRequest &jobRequest)
{
    const JobRequest::ExpirationConditions conditions = jobRequest.expirationConditions;
    const UnsavedFiles unsavedFiles = m_documents.unsavedFiles();
    using Condition = JobRequest::ExpirationCondition;

    if (conditions.testFlag(Condition::UnsavedFilesChanged)) {
        if (jobRequest.unsavedFilesChangeTimePoint != unsavedFiles.lastChangeTimePoint()) {
            qCDebug(jobsLog) << "Removing due to outdated unsaved files:" << jobRequest;
            return true;
        }
    }

    bool projectCheckedAndItExists = false;

    if (conditions.testFlag(Condition::DocumentClosed)) {
        if (!m_documents.hasDocument(jobRequest.filePath, jobRequest.projectPartId)) {
            qCDebug(jobsLog) << "Removing due to already closed document:" << jobRequest;
            return true;
        }

        if (!m_projectParts.hasProjectPart(jobRequest.projectPartId)) {
            qCDebug(jobsLog) << "Removing due to already closed project:" << jobRequest;
            return true;
        }
        projectCheckedAndItExists = true;

        const Document document
                = m_documents.document(jobRequest.filePath, jobRequest.projectPartId);
        if (!document.isIntact()) {
            qCDebug(jobsLog) << "Removing/Cancelling due to not intact document:" << jobRequest;
            cancelJobRequest(jobRequest);
            return true;
        }

        if (conditions.testFlag(Condition::DocumentRevisionChanged)) {
            if (document.documentRevision() > jobRequest.documentRevision) {
                qCDebug(jobsLog) << "Removing due to changed document revision:" << jobRequest;
                return true;
            }
        }
    }

    if (conditions.testFlag(Condition::ProjectChanged)) {
        if (!projectCheckedAndItExists && !m_projectParts.hasProjectPart(jobRequest.projectPartId)) {
            qCDebug(jobsLog) << "Removing due to already closed project:" << jobRequest;
            return true;
        }

        const ProjectPart &project = m_projectParts.project(jobRequest.projectPartId);
        if (project.lastChangeTimePoint() != jobRequest.projectChangeTimePoint) {
            qCDebug(jobsLog) << "Removing due to outdated project:" << jobRequest;
            return true;
        }
    }

    return false;
}

static int priority(const Document &document)
{
    int thePriority = 0;

    if (document.isUsedByCurrentEditor())
        thePriority += 1000;

    if (document.isVisibleInEditor())
        thePriority += 100;

    return thePriority;
}

void JobQueue::prioritizeRequests()
{
    const auto lessThan = [this] (const JobRequest &r1, const JobRequest &r2) {
        // TODO: Getting the TU is O(n) currently, so this might become expensive for large n.
        const Document &t1 = m_documents.document(r1.filePath, r1.projectPartId);
        const Document &t2 = m_documents.document(r2.filePath, r2.projectPartId);

        return priority(t1) > priority(t2);
    };

    std::stable_sort(m_queue.begin(), m_queue.end(), lessThan);
}

void JobQueue::cancelJobRequest(const JobRequest &jobRequest)
{
    if (m_cancelJobRequest)
        m_cancelJobRequest(jobRequest);
}

static bool areRunConditionsMet(const JobRequest &request, const Document &document)
{
    using Condition = JobRequest::RunCondition;
    const JobRequest::RunConditions conditions = request.runConditions;

    if (conditions.testFlag(Condition::DocumentSuspended) && !document.isSuspended()) {
        qCDebug(jobsLog) << "Not choosing due to unsuspended document:" << request;
        return false;
    }

    if (conditions.testFlag(Condition::DocumentUnsuspended) && document.isSuspended()) {
        qCDebug(jobsLog) << "Not choosing due to suspended document:" << request;
        return false;
    }

    if (conditions.testFlag(Condition::DocumentVisible) && !document.isVisibleInEditor()) {
        qCDebug(jobsLog) << "Not choosing due to invisble document:" << request;
        return false;
    }

    if (conditions.testFlag(Condition::DocumentNotVisible) && document.isVisibleInEditor()) {
        qCDebug(jobsLog) << "Not choosing due to visble document:" << request;
        return false;
    }

    if (conditions.testFlag(Condition::CurrentDocumentRevision)) {
        if (document.isDirty()) {
            // TODO: If the document is dirty due to a project update,
            // references are processes later than ideal.
            qCDebug(jobsLog) << "Not choosing due to dirty document:" << request;
            return false;
        }

        if (request.documentRevision != document.documentRevision()) {
            qCDebug(jobsLog) << "Not choosing due to revision mismatch:" << request;
            return false;
        }
    }

    return true;
}

JobRequests JobQueue::takeJobRequestsToRunNow()
{
    JobRequests jobsToRun;
    using TranslationUnitIds = QSet<Utf8String>;
    TranslationUnitIds translationUnitsScheduledForThisRun;

    QMutableVectorIterator<JobRequest> i(m_queue);
    while (i.hasNext()) {
        const JobRequest &request = i.next();

        try {
            const Document &document = m_documents.document(request.filePath,
                                                            request.projectPartId);

            if (!areRunConditionsMet(request, document))
                continue;

            const Utf8String id = document.translationUnit(request.preferredTranslationUnit).id();
            if (translationUnitsScheduledForThisRun.contains(id))
                continue;

            if (isJobRunningForTranslationUnit(id))
                continue;

            translationUnitsScheduledForThisRun.insert(id);
            jobsToRun += request;
            i.remove();
        } catch (const std::exception &exception) {
            qWarning() << "Error in Jobs::takeJobRequestsToRunNow for"
                       << request << ":" << exception.what();
        }
    }

    return jobsToRun;
}

bool JobQueue::isJobRunningForTranslationUnit(const Utf8String &translationUnitId)
{
    if (m_isJobRunningForTranslationUnitHandler)
        return m_isJobRunningForTranslationUnitHandler(translationUnitId);

    return false;
}

bool JobQueue::isJobRunningForJobRequest(const JobRequest &jobRequest)
{
    if (m_isJobRunningForJobRequestHandler)
        return m_isJobRunningForJobRequestHandler(jobRequest);

    return false;
}

void JobQueue::setIsJobRunningForTranslationUnitHandler(
        const IsJobRunningForTranslationUnitHandler &isJobRunningHandler)
{
    m_isJobRunningForTranslationUnitHandler = isJobRunningHandler;
}

void JobQueue::setIsJobRunningForJobRequestHandler(
        const JobQueue::IsJobRunningForJobRequestHandler &isJobRunningHandler)
{
    m_isJobRunningForJobRequestHandler = isJobRunningHandler;
}

void JobQueue::setCancelJobRequest(const JobQueue::CancelJobRequest &cancelJobRequest)
{
    m_cancelJobRequest = cancelJobRequest;
}

JobRequests &JobQueue::queue()
{
    return m_queue;
}

} // namespace ClangBackEnd
