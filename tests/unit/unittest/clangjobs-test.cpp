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

#include "googletest.h"
#include "processevents-utilities.h"
#include "dummyclangipcclient.h"

#include <clangdocument.h>
#include <clangjobs.h>
#include <filecontainer.h>
#include <projectpart.h>
#include <projects.h>
#include <clangdocuments.h>
#include <unsavedfiles.h>

#include <clang-c/Index.h>

using namespace ClangBackEnd;

using testing::IsNull;
using testing::NotNull;
using testing::Eq;
using testing::Gt;
using testing::Contains;
using testing::EndsWith;
using testing::AllOf;

namespace {

class Jobs : public ::testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    bool waitUntilAllJobsFinished(int timeOutInMs = 10000) const;
    bool waitUntilJobChainFinished(int timeOutInMs = 10000);

protected:
    ClangBackEnd::ProjectParts projects;
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::Documents documents{projects, unsavedFiles};
    ClangBackEnd::Document document;
    DummyIpcClient dummyClientInterface;

    Utf8String filePath1 = Utf8StringLiteral(TESTDATA_DIR"/translationunits.cpp");
    Utf8String projectPartId{Utf8StringLiteral("/path/to/projectfile")};

    ClangBackEnd::Jobs jobs{documents, unsavedFiles, projects, dummyClientInterface};
};

using JobsSlowTest = Jobs;

TEST_F(Jobs, ProcessEmptyQueue)
{
    const JobRequests jobsStarted = jobs.process();

    ASSERT_THAT(jobsStarted.size(), Eq(0));
    ASSERT_TRUE(jobs.runningJobs().isEmpty());
}

TEST_F(JobsSlowTest, ProcessQueueWithSingleJob)
{
    jobs.add(document, JobRequest::Type::UpdateDocumentAnnotations);

    const JobRequests jobsStarted = jobs.process();

    ASSERT_THAT(jobsStarted.size(), Eq(1));
    ASSERT_THAT(jobs.runningJobs().size(), Eq(1));
}

TEST_F(JobsSlowTest, ProcessQueueUntilEmpty)
{
    jobs.add(document, JobRequest::Type::UpdateDocumentAnnotations);
    jobs.add(document, JobRequest::Type::UpdateDocumentAnnotations);
    jobs.add(document, JobRequest::Type::UpdateDocumentAnnotations);

    jobs.process();

    waitUntilJobChainFinished();
}

TEST_F(JobsSlowTest, IsJobRunning)
{
    jobs.add(document, JobRequest::Type::UpdateDocumentAnnotations);
    jobs.process();

    const bool isJobRunning = jobs.isJobRunningForTranslationUnit(document.translationUnit().id());

    ASSERT_TRUE(isJobRunning);
}

void Jobs::SetUp()
{
    projects.createOrUpdate({ProjectPartContainer(projectPartId)});

    const QVector<FileContainer> fileContainer{FileContainer(filePath1, projectPartId)};
    document = documents.create(fileContainer).front();
    documents.setVisibleInEditors({filePath1});
    documents.setUsedByCurrentEditor(filePath1);
}

void Jobs::TearDown()
{
    ASSERT_TRUE(waitUntilAllJobsFinished()); // QFuture/QFutureWatcher is implemented with events
}

bool Jobs::waitUntilAllJobsFinished(int timeOutInMs) const
{
    const auto noJobsRunningAnymore = [this](){ return jobs.runningJobs().isEmpty(); };

    return ProcessEventUtilities::processEventsUntilTrue(noJobsRunningAnymore, timeOutInMs);
}

bool Jobs::waitUntilJobChainFinished(int timeOutInMs)
{
    const auto noJobsRunningAnymore = [this]() {
        return jobs.runningJobs().isEmpty() && jobs.queue().isEmpty();
    };

    return ProcessEventUtilities::processEventsUntilTrue(noJobsRunningAnymore, timeOutInMs);
}

} // anonymous
