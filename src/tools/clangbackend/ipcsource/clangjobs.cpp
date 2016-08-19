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

Jobs::Jobs(TranslationUnits &translationUnits,
           UnsavedFiles &unsavedFiles,
           ProjectParts &projectParts,
           ClangCodeModelClientInterface &client)
    : m_translationUnits(translationUnits)
    , m_unsavedFiles(unsavedFiles)
    , m_projectParts(projectParts)
    , m_client(client)
    , m_queue(translationUnits, projectParts)
{
    m_queue.setIsJobRunningHandler([this](const Utf8String &filePath,
                                          const Utf8String &projectPartId) {
        return isJobRunning(filePath, projectPartId);
    });
}

Jobs::~Jobs()
{
    QFutureSynchronizer<void> waitForFinishedJobs;
    foreach (const RunningJob &runningJob, m_running.values())
        waitForFinishedJobs.addFuture(runningJob.future);
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
    if (IAsyncJob *asyncJob = IAsyncJob::create(jobRequest.type)) {
        JobContext context(jobRequest, &m_translationUnits, &m_unsavedFiles, &m_client);
        asyncJob->setContext(context);

        if (asyncJob->prepareAsyncRun()) {
            qCDebug(jobsLog) << "Running" << jobRequest;

            asyncJob->setFinishedHandler([this](IAsyncJob *asyncJob){ onJobFinished(asyncJob); });
            const QFuture<void> future = asyncJob->runAsync();

            m_running.insert(asyncJob, RunningJob{jobRequest, future});
            return true;
        } else {
            qCDebug(jobsLog) << "Preparation failed for " << jobRequest;
            delete asyncJob;
        }
    }

    return false;
}

void Jobs::onJobFinished(IAsyncJob *asyncJob)
{
    qCDebug(jobsLog) << "Finishing" << asyncJob->context().jobRequest;

    m_running.remove(asyncJob);
    delete asyncJob;

    process();
}

int Jobs::runningJobs() const
{
    return m_running.size();
}

JobRequests Jobs::queue() const
{
    return m_queue.queue();
}

bool Jobs::isJobRunning(const Utf8String &filePath, const Utf8String &projectPartId) const
{
    const auto hasJobRequest = [filePath, projectPartId](const RunningJob &runningJob) {
        const JobRequest &jobRequest = runningJob.jobRequest;
        return filePath == jobRequest.filePath
            && projectPartId == jobRequest.projectPartId;
    };

    return Utils::anyOf(m_running.values(), hasJobRequest);
}

} // namespace ClangBackEnd
