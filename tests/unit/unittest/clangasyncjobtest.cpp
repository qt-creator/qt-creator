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

#include "clangasyncjobtest.h"
#include "testutils.h"

#include <clangbackendipc/filecontainer.h>

using namespace ClangBackEnd;

void ClangAsyncJobTest::BaseSetUp(ClangBackEnd::JobRequest::Type jobRequestType,
                                  IAsyncJob &asyncJob)
{
    projects.createOrUpdate({ProjectPartContainer(projectPartId)});

    const QVector<FileContainer> fileContainer{FileContainer(filePath, projectPartId)};
    translationUnit = translationUnits.create(fileContainer).front();
    translationUnits.setVisibleInEditors({filePath});
    translationUnits.setUsedByCurrentEditor(filePath);

    jobRequest = createJobRequest(filePath, jobRequestType);
    jobContext = JobContext(jobRequest, &translationUnits, &unsavedFiles, &dummyIpcClient);
    jobContextWithMockClient = JobContext(jobRequest, &translationUnits, &unsavedFiles, &mockIpcClient);
    asyncJob.setFinishedHandler([](IAsyncJob *){});
}

JobRequest ClangAsyncJobTest::createJobRequest(const Utf8String &filePath,
                                               JobRequest::Type type) const
{
    JobRequest jobRequest;
    jobRequest.type = type;
    jobRequest.requirements = JobRequest::requirementsForType(type);
    jobRequest.filePath = filePath;
    jobRequest.projectPartId = projectPartId;
    jobRequest.unsavedFilesChangeTimePoint = unsavedFiles.lastChangeTimePoint();
    jobRequest.documentRevision = translationUnit.documentRevision();
    jobRequest.projectChangeTimePoint = projects.project(projectPartId).lastChangeTimePoint();

    return jobRequest;
}

bool ClangAsyncJobTest::waitUntilJobFinished(const ClangBackEnd::IAsyncJob &asyncJob,
                                             int timeOutInMs) const
{
    const auto isOnFinishedSlotExecuted = [&asyncJob](){ return asyncJob.isFinished(); };

    return TestUtils::processEventsUntilTrue(isOnFinishedSlotExecuted, timeOutInMs);
}
