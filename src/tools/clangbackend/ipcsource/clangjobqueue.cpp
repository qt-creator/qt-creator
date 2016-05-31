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
#include "clangtranslationunit.h"
#include "translationunits.h"
#include "projects.h"
#include "unsavedfiles.h"

#include <utils/algorithm.h>

namespace ClangBackEnd {

JobQueue::JobQueue(TranslationUnits &translationUnits,
                   UnsavedFiles &unsavedFiles,
                   ProjectParts &projectParts,
                   ClangCodeModelClientInterface &client)
    : m_translationUnits(translationUnits)
    , m_unsavedFiles(unsavedFiles)
    , m_projectParts(projectParts)
    , m_client(client)
{
}

void JobQueue::add(const JobRequest &job)
{
    qCDebug(jobsLog) << "Adding" << job;

    m_queue.append(job);
}

int JobQueue::size() const
{
    return m_queue.size();
}

JobRequests JobQueue::processQueue()
{
    removeOutDatedRequests();
    prioritizeRequests();
    const JobRequests jobsToRun = takeJobRequestsToRunNow();

    return jobsToRun;
}

void JobQueue::removeOutDatedRequests()
{
    JobRequests cleanedRequests;

    foreach (const JobRequest &jobRequest, m_queue) {
        try {
            if (!isJobRequestOutDated(jobRequest))
                cleanedRequests.append(jobRequest);
        } catch (const std::exception &exception) {
            qWarning() << "Error in Jobs::removeOutDatedRequests for"
                       << jobRequest << ":" << exception.what();
        }
    }

    m_queue = cleanedRequests;
}

bool JobQueue::isJobRequestOutDated(const JobRequest &jobRequest) const
{
    const JobRequest::Requirements requirements = jobRequest.requirements;
    const UnsavedFiles unsavedFiles = m_translationUnits.unsavedFiles();

    if (requirements.testFlag(JobRequest::CurrentUnsavedFiles)) {
        if (jobRequest.unsavedFilesChangeTimePoint != unsavedFiles.lastChangeTimePoint()) {
            qCDebug(jobsLog) << "Removing due to outdated unsaved files:" << jobRequest;
            return true;
        }
    }

    bool projectCheckedAndItExists = false;

    if (requirements.testFlag(JobRequest::DocumentValid)) {
        if (!m_translationUnits.hasTranslationUnit(jobRequest.filePath, jobRequest.projectPartId)) {
            qCDebug(jobsLog) << "Removing due to already closed document:" << jobRequest;
            return true;
        }

        if (!m_projectParts.hasProjectPart(jobRequest.projectPartId)) {
            qCDebug(jobsLog) << "Removing due to already closed project:" << jobRequest;
            return true;
        }
        projectCheckedAndItExists = true;

        const TranslationUnit translationUnit
                = m_translationUnits.translationUnit(jobRequest.filePath, jobRequest.projectPartId);
        if (!translationUnit.isIntact()) {
            qCDebug(jobsLog) << "Removing due to not intact translation unit:" << jobRequest;
            return true;
        }

        if (requirements.testFlag(JobRequest::CurrentDocumentRevision)) {
            if (translationUnit.documentRevision() != jobRequest.documentRevision) {
                qCDebug(jobsLog) << "Removing due to changed document revision:" << jobRequest;
                return true;
            }
        }
    }

    if (requirements.testFlag(JobRequest::CurrentProject)) {
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

static int priority(const TranslationUnit &translationUnit)
{
    int thePriority = 0;

    if (translationUnit.isUsedByCurrentEditor())
        thePriority += 1000;

    if (translationUnit.isVisibleInEditor())
        thePriority += 100;

    return thePriority;
}

void JobQueue::prioritizeRequests()
{
    const auto lessThan = [this] (const JobRequest &r1, const JobRequest &r2) {
        // TODO: Getting the TU is O(n) currently, so this might become expensive for large n.
        const TranslationUnit &t1 = m_translationUnits.translationUnit(r1.filePath, r1.projectPartId);
        const TranslationUnit &t2 = m_translationUnits.translationUnit(r2.filePath, r2.projectPartId);

        return priority(t1) > priority(t2);
    };

    std::stable_sort(m_queue.begin(), m_queue.end(), lessThan);
}

JobRequests JobQueue::takeJobRequestsToRunNow()
{
    JobRequests jobsToRun;
    QSet<DocumentId> documentsScheduledForThisRun;

    QMutableVectorIterator<JobRequest> i(m_queue);
    while (i.hasNext()) {
        const JobRequest &jobRequest = i.next();

        try {
            const TranslationUnit &translationUnit
                    = m_translationUnits.translationUnit(jobRequest.filePath,
                                                         jobRequest.projectPartId);
            const DocumentId documentId = DocumentId(jobRequest.filePath, jobRequest.projectPartId);

            if (!translationUnit.isUsedByCurrentEditor() && !translationUnit.isVisibleInEditor())
                continue;

            if (documentsScheduledForThisRun.contains(documentId))
                continue;

            if (isJobRunningForDocument(documentId))
                continue;

            documentsScheduledForThisRun.insert(documentId);
            jobsToRun += jobRequest;
            i.remove();
        } catch (const std::exception &exception) {
            qWarning() << "Error in Jobs::takeJobRequestsToRunNow for"
                       << jobRequest << ":" << exception.what();
        }
    }

    return jobsToRun;
}

bool JobQueue::isJobRunningForDocument(const JobQueue::DocumentId &documentId)
{
    if (m_isJobRunningHandler)
        return m_isJobRunningHandler(documentId.first, documentId.second);

    return false;
}

void JobQueue::setIsJobRunningHandler(const IsJobRunningHandler &isJobRunningHandler)
{
    m_isJobRunningHandler = isJobRunningHandler;
}

JobRequests JobQueue::queue() const
{
    return m_queue;
}

} // namespace ClangBackEnd
