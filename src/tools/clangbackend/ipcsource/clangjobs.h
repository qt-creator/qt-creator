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

#include <clangbackendipc/clangcodemodelclientinterface.h>

#include <QFuture>

namespace ClangBackEnd {

class ClangCodeModelClientInterface;
class Documents;
class IAsyncJob;
class ProjectParts;
class UnsavedFiles;

class Jobs
{
public:
    struct RunningJob {
        JobRequest jobRequest;
        QFuture<void> future;
    };
    using RunningJobs = QHash<IAsyncJob *, RunningJob>;

public:
    Jobs(Documents &documents,
         UnsavedFiles &unsavedFiles,
         ProjectParts &projects,
         ClangCodeModelClientInterface &client);
    ~Jobs();

    void add(const JobRequest &job);

    JobRequests process();

public /*for tests*/:
    int runningJobs() const;
    JobRequests queue() const;
    bool isJobRunning(const Utf8String &filePath, const Utf8String &projectPartId) const;

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
};

} // namespace ClangBackEnd
