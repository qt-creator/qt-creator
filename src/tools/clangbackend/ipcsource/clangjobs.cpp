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

#include "clangjobs.h"

#include "clangiasyncjob.h"

#include <QDebug>
#include <QFutureSynchronizer>
#include <QLoggingCategory>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

namespace ClangBackEnd {

Jobs::Jobs(Documents &documents,
           UnsavedFiles &unsavedFiles,
           ProjectParts &projectParts,
           ClangCodeModelClientInterface &client)
    : m_documents(documents)
    , m_unsavedFiles(unsavedFiles)
    , m_client(client)
    , m_queue(documents, projectParts)
{
    m_queue.setIsJobRunningForTranslationUnitHandler([this](const Utf8String &translationUnitId) {
        return isJobRunningForTranslationUnit(translationUnitId);
    });
    m_queue.setIsJobRunningForJobRequestHandler([this](const JobRequest &jobRequest) {
        return isJobRunningForJobRequest(jobRequest);
    });
}

Jobs::~Jobs()
{
    foreach (IAsyncJob *asyncJob, m_running.keys())
        asyncJob->preventFinalization();

    QFutureSynchronizer<void> waitForFinishedJobs;
    foreach (const RunningJob &runningJob, m_running.values())
        waitForFinishedJobs.addFuture(runningJob.future);

    foreach (IAsyncJob *asyncJob, m_running.keys())
        delete asyncJob;
}

void Jobs::add(const JobRequest &job)
{
    m_queue.add(job);
}

JobRequests Jobs::process()
{
    const JobRequests jobsToRun = m_queue.processQueue();
    const JobRequests jobsStarted = runJobs(jobsToRun);

    QTC_CHECK(jobsToRun.size() == jobsStarted.size());

    return jobsStarted;
}

JobRequests Jobs::runJobs(const JobRequests &jobsRequests)
{
    JobRequests jobsStarted;

    foreach (const JobRequest &jobRequest, jobsRequests) {
        if (runJob(jobRequest))
            jobsStarted += jobRequest;
    }

    return jobsStarted;
}

bool Jobs::runJob(const JobRequest &jobRequest)
{
    IAsyncJob *asyncJob = IAsyncJob::create(jobRequest.type);
    QTC_ASSERT(asyncJob, return false);

    JobContext context(jobRequest, &m_documents, &m_unsavedFiles, &m_client);
    asyncJob->setContext(context);

    if (const IAsyncJob::AsyncPrepareResult prepareResult = asyncJob->prepareAsyncRun()) {
        qCDebug(jobsLog) << "Running" << jobRequest
                         << "with TranslationUnit" << prepareResult.translationUnitId;

        asyncJob->setFinishedHandler([this](IAsyncJob *asyncJob){ onJobFinished(asyncJob); });
        const QFuture<void> future = asyncJob->runAsync();

        const RunningJob runningJob{jobRequest, prepareResult.translationUnitId, future};
        m_running.insert(asyncJob, runningJob);
        return true;
    } else {
        qCDebug(jobsLog) << "Preparation failed for " << jobRequest;
        delete asyncJob;
    }

    return false;
}

void Jobs::onJobFinished(IAsyncJob *asyncJob)
{
    qCDebug(jobsLog) << "Finishing" << asyncJob->context().jobRequest;

    if (m_jobFinishedCallback) {
        const RunningJob runningJob = m_running.value(asyncJob);
        m_jobFinishedCallback(runningJob);
    }

    m_running.remove(asyncJob);
    delete asyncJob;

    process();
}

void Jobs::setJobFinishedCallback(const JobFinishedCallback &jobFinishedCallback)
{
    m_jobFinishedCallback = jobFinishedCallback;
}

QList<Jobs::RunningJob> Jobs::runningJobs() const
{
    return m_running.values();
}

JobRequests Jobs::queue() const
{
    return m_queue.queue();
}

bool Jobs::isJobRunningForTranslationUnit(const Utf8String &translationUnitId) const
{
    const auto hasTranslationUnitId = [translationUnitId](const RunningJob &runningJob) {
        return runningJob.translationUnitId == translationUnitId;
    };

    return Utils::anyOf(m_running.values(), hasTranslationUnitId);
}

bool Jobs::isJobRunningForJobRequest(const JobRequest &jobRequest) const
{
    const auto hasJobRequest = [jobRequest](const RunningJob &runningJob) {
        return runningJob.jobRequest == jobRequest;
    };

    return Utils::anyOf(m_running.values(), hasJobRequest);
}

} // namespace ClangBackEnd
