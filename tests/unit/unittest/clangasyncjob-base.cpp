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

#include "clangasyncjob-base.h"
#include "processevents-utilities.h"

#include <clangsupport/filecontainer.h>

using namespace ClangBackEnd;

void ClangAsyncJobTest::BaseSetUp(ClangBackEnd::JobRequest::Type jobRequestType,
                                  IAsyncJob &asyncJob)
{
    projects.createOrUpdate({ProjectPartContainer(projectPartId)});

    const QVector<FileContainer> fileContainer{FileContainer(filePath, projectPartId)};
    document = documents.create(fileContainer).front();
    documents.setVisibleInEditors({filePath});
    documents.setUsedByCurrentEditor(filePath);

    jobRequest = createJobRequest(filePath, jobRequestType);
    jobContext = JobContext(jobRequest, &documents, &unsavedFiles, &dummyIpcClient);
    jobContextWithMockClient = JobContext(jobRequest, &documents, &unsavedFiles, &mockIpcClient);
    asyncJob.setFinishedHandler([](IAsyncJob *){});
}

JobRequest ClangAsyncJobTest::createJobRequest(const Utf8String &filePath,
                                               JobRequest::Type type) const
{
    JobRequest jobRequest(type);
    jobRequest.filePath = filePath;
    jobRequest.projectPartId = projectPartId;
    jobRequest.unsavedFilesChangeTimePoint = unsavedFiles.lastChangeTimePoint();
    jobRequest.documentRevision = document.documentRevision();
    jobRequest.projectChangeTimePoint = projects.project(projectPartId).lastChangeTimePoint();

    return jobRequest;
}

bool ClangAsyncJobTest::waitUntilJobFinished(const ClangBackEnd::IAsyncJob &asyncJob,
                                             int timeOutInMs) const
{
    const auto isOnFinishedSlotExecuted = [&asyncJob](){ return asyncJob.isFinished(); };

    return ProcessEventUtilities::processEventsUntilTrue(isOnFinishedSlotExecuted, timeOutInMs);
}
