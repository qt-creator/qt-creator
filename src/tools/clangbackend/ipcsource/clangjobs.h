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

#pragma once

#include "clangjobqueue.h"

#include <clangsupport/clangcodemodelclientinterface.h>

#include <QFuture>

#include <functional>

namespace ClangBackEnd {

class ClangCodeModelClientInterface;
class Documents;
class IAsyncJob;
class UnsavedFiles;

class Jobs
{
public:
    struct RunningJob {
        JobRequest jobRequest;
        Utf8String translationUnitId;
        QFuture<void> future;
    };

    using RunningJobs = QHash<IAsyncJob *, RunningJob>;
    using JobFinishedCallback = std::function<void(RunningJob)>;

public:
    Jobs(Documents &documents,
         UnsavedFiles &unsavedFiles,
         ProjectParts &projects,
         ClangCodeModelClientInterface &client);
    ~Jobs();

    JobRequest createJobRequest(const Document &document, JobRequest::Type type,
                                PreferredTranslationUnit preferredTranslationUnit
                                    = PreferredTranslationUnit::RecentlyParsed) const;

    void add(const JobRequest &job);
    void add(const Document &document,
             JobRequest::Type type,
             PreferredTranslationUnit preferredTranslationUnit
                 = PreferredTranslationUnit::RecentlyParsed);

    JobRequests process();

    void setJobFinishedCallback(const JobFinishedCallback &jobFinishedCallback);

public /*for tests*/:
    QList<RunningJob> runningJobs() const;
    JobRequests &queue();
    bool isJobRunningForTranslationUnit(const Utf8String &translationUnitId) const;
    bool isJobRunningForJobRequest(const JobRequest &jobRequest) const;

private:
    JobRequests runJobs(const JobRequests &jobRequest);
    bool runJob(const JobRequest &jobRequest);
    void onJobFinished(IAsyncJob *asyncJob);

private:
    Documents &m_documents;
    UnsavedFiles &m_unsavedFiles;
    ProjectParts &m_projectParts;
    ClangCodeModelClientInterface &m_client;

    JobQueue m_queue;
    RunningJobs m_running;
    JobFinishedCallback m_jobFinishedCallback;
};

} // namespace ClangBackEnd
