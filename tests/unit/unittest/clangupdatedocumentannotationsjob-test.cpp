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

#include <clangupdatedocumentannotationsjob.h>

using namespace ClangBackEnd;

using testing::Not;
using testing::_;

namespace {

class UpdateDocumentAnnotationsJob : public ClangAsyncJobTest
{
protected:
    void SetUp() override { BaseSetUp(JobRequest::Type::UpdateDocumentAnnotations, job); }

protected:
    ClangBackEnd::UpdateDocumentAnnotationsJob job;
};

using UpdateDocumentAnnotationsJobSlowTest = UpdateDocumentAnnotationsJob;

TEST_F(UpdateDocumentAnnotationsJob, PrepareAsyncRun)
{
    job.setContext(jobContext);

    ASSERT_TRUE(job.prepareAsyncRun());
}

TEST_F(UpdateDocumentAnnotationsJobSlowTest, RunAsync)
{
    job.setContext(jobContext);
    job.prepareAsyncRun();

    job.runAsync();

    ASSERT_TRUE(waitUntilJobFinished(job));
}

TEST_F(UpdateDocumentAnnotationsJobSlowTest, SendAnnotations)
{
    job.setContext(jobContextWithMockClient);
    job.prepareAsyncRun();
    EXPECT_CALL(mockIpcClient, documentAnnotationsChanged(_)).Times(1);

    job.runAsync();

    ASSERT_TRUE(waitUntilJobFinished(job));
}

TEST_F(UpdateDocumentAnnotationsJobSlowTest, DontSendAnnotationsIfDocumentWasClosed)
{
    job.setContext(jobContextWithMockClient);
    job.prepareAsyncRun();
    EXPECT_CALL(mockIpcClient, documentAnnotationsChanged(_)).Times(0);

    job.runAsync();
    documents.remove({FileContainer{filePath, projectPartId}});

    ASSERT_TRUE(waitUntilJobFinished(job));
}

TEST_F(UpdateDocumentAnnotationsJobSlowTest, DontSendAnnotationsIfDocumentRevisionChanged)
{
    job.setContext(jobContextWithMockClient);
    job.prepareAsyncRun();
    EXPECT_CALL(mockIpcClient, documentAnnotationsChanged(_)).Times(0);

    job.runAsync();
    documents.update({FileContainer(filePath, projectPartId, Utf8String(), true, 99)});

    ASSERT_TRUE(waitUntilJobFinished(job));
}

TEST_F(UpdateDocumentAnnotationsJobSlowTest, UpdatesTranslationUnit)
{
    const TimePoint timePointBefore = document.lastProjectPartChangeTimePoint();
    const QSet<Utf8String> dependendOnFilesBefore = document.dependedFilePaths();
    job.setContext(jobContext);
    job.prepareAsyncRun();

    job.runAsync();
    ASSERT_TRUE(waitUntilJobFinished(job));

    ASSERT_THAT(timePointBefore, Not(document.lastProjectPartChangeTimePoint()));
    ASSERT_THAT(dependendOnFilesBefore, Not(document.dependedFilePaths()));
}

} // anonymous
