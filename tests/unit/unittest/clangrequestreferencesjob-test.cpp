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

#include <clangrequestreferencesjob.h>

using namespace ClangBackEnd;

using testing::_;
using testing::Eq;
using testing::Property;

namespace {

class RequestReferencesJob : public ClangAsyncJobTest
{
protected:
    void SetUp() override { BaseSetUp(JobRequest::Type::RequestReferences, job); }

protected:
    ClangBackEnd::RequestReferencesJob job;
};

TEST_F(RequestReferencesJob, PrepareAsyncRun)
{
    job.setContext(jobContext);

    ASSERT_TRUE(job.prepareAsyncRun());
}

TEST_F(RequestReferencesJob, RunAsync)
{
    job.setContext(jobContext);
    job.prepareAsyncRun();

    job.runAsync();

    ASSERT_TRUE(waitUntilJobFinished(job));
}

TEST_F(RequestReferencesJob, SendReferences)
{
    job.setContext(jobContextWithMockClient);
    job.prepareAsyncRun();
    EXPECT_CALL(mockIpcClient, references(_)).Times(1);

    job.runAsync();

    ASSERT_TRUE(waitUntilJobFinished(job));
}

TEST_F(RequestReferencesJob, ForwardTicketNumber)
{
    jobRequest.ticketNumber = static_cast<quint64>(99);
    jobContextWithMockClient = JobContext(jobRequest, &documents, &unsavedFiles, &mockIpcClient);
    job.setContext(jobContextWithMockClient);
    job.prepareAsyncRun();
    EXPECT_CALL(mockIpcClient,
                references(Property(&ReferencesMessage::ticketNumber, Eq(jobRequest.ticketNumber))))
                    .Times(1);

    job.runAsync();

    ASSERT_TRUE(waitUntilJobFinished(job));
}

TEST_F(RequestReferencesJob, DontSendReferencesIfDocumentWasClosed)
{
    job.setContext(jobContextWithMockClient);
    job.prepareAsyncRun();
    EXPECT_CALL(mockIpcClient, references(_)).Times(0);

    job.runAsync();
    documents.remove({FileContainer{filePath, projectPartId}});

    ASSERT_TRUE(waitUntilJobFinished(job));
}

TEST_F(RequestReferencesJob, DontSendReferencesIfDocumentRevisionChanged)
{
    job.setContext(jobContextWithMockClient);
    job.prepareAsyncRun();
    EXPECT_CALL(mockIpcClient, references(_)).Times(0);

    job.runAsync();
    documents.update({FileContainer(filePath, projectPartId, Utf8String(), true, 99)});

    ASSERT_TRUE(waitUntilJobFinished(job));
}

} // anonymous
