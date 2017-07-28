/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <clangresumedocumentjob.h>

using namespace ClangBackEnd;

using testing::_;

namespace {

class ResumeDocumentJob : public ClangAsyncJobTest
{
protected:
    void SetUp() override { BaseSetUp(JobRequest::Type::ResumeDocument, job); }
    void suspendDocument()
    {
        document.parse();
        document.translationUnit().suspend();
        document.setIsSuspended(true);
    }

protected:
    ClangBackEnd::ResumeDocumentJob job;
};

TEST_F(ResumeDocumentJob, PrepareAsyncRun)
{
    job.setContext(jobContext);

    ASSERT_TRUE(job.prepareAsyncRun());
}

TEST_F(ResumeDocumentJob, RunAsync)
{
    suspendDocument();
    job.setContext(jobContext);
    job.prepareAsyncRun();

    job.runAsync();

    ASSERT_TRUE(waitUntilJobFinished(job));
}

TEST_F(ResumeDocumentJob, DocumentIsResumedAfterRun)
{
    suspendDocument();
    job.setContext(jobContext);
    job.prepareAsyncRun();

    job.runAsync();
    ASSERT_TRUE(waitUntilJobFinished(job));

    ASSERT_FALSE(document.isSuspended());
}

TEST_F(ResumeDocumentJob, SendsAnnotationsAfterResume)
{
    suspendDocument();
    job.setContext(jobContextWithMockClient);
    job.prepareAsyncRun();
    EXPECT_CALL(mockIpcClient, documentAnnotationsChanged(_)).Times(1);

    job.runAsync();
    ASSERT_TRUE(waitUntilJobFinished(job));
}

} // anonymous
