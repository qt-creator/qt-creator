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

#include <clangrequestdocumentannotationsjob.h>

using namespace ClangBackEnd;

using testing::_;

namespace {

class RequestDocumentAnnotationsJob : public ClangAsyncJobTest
{
protected:
    void SetUp() override { BaseSetUp(JobRequest::Type::RequestDocumentAnnotations, job); }

protected:
    ClangBackEnd::RequestDocumentAnnotationsJob job;
};

TEST_F(RequestDocumentAnnotationsJob, PrepareAsyncRun)
{
    job.setContext(jobContext);

    ASSERT_TRUE(job.prepareAsyncRun());
}

TEST_F(RequestDocumentAnnotationsJob, RunAsync)
{
    job.setContext(jobContext);
    job.prepareAsyncRun();

    job.runAsync();

    ASSERT_TRUE(waitUntilJobFinished(job));
}

TEST_F(RequestDocumentAnnotationsJob, SendAnnotations)
{
    job.setContext(jobContextWithMockClient);
    job.prepareAsyncRun();
    EXPECT_CALL(mockIpcClient, documentAnnotationsChanged(_)).Times(1);

    job.runAsync();

    ASSERT_TRUE(waitUntilJobFinished(job));
}

TEST_F(RequestDocumentAnnotationsJob, DontSendAnnotationsIfDocumentWasClosed)
{
    job.setContext(jobContextWithMockClient);
    job.prepareAsyncRun();
    EXPECT_CALL(mockIpcClient, documentAnnotationsChanged(_)).Times(0);

    job.runAsync();
    documents.remove({FileContainer{filePath, projectPartId}});

    ASSERT_TRUE(waitUntilJobFinished(job));
}

} // anonymous
