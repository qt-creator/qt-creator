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

#include "testutils.h"
#include "dummyclangipcclient.h"

#include <clangjobs.h>
#include <clangtranslationunit.h>
#include <filecontainer.h>
#include <projectpart.h>
#include <projects.h>
#include <translationunits.h>
#include <unsavedfiles.h>

#include <clang-c/Index.h>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

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
    bool waitUntilJobChainFinished(int timeOutInMs = 10000) const;

    JobRequest createJobRequest(const Utf8String &filePath,
                                JobRequest::Type type) const;

protected:
    ClangBackEnd::ProjectParts projects;
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::TranslationUnits translationUnits{projects, unsavedFiles};
    ClangBackEnd::TranslationUnit translationUnit;
    DummyIpcClient dummyClientInterface;

    Utf8String filePath1 = Utf8StringLiteral(TESTDATA_DIR"/translationunits.cpp");
    Utf8String projectPartId{Utf8StringLiteral("/path/to/projectfile")};

    ClangBackEnd::Jobs jobs{translationUnits, unsavedFiles, projects, dummyClientInterface};
};

TEST_F(Jobs, ProcessEmptyQueue)
{
    const JobRequests jobsStarted = jobs.process();

    ASSERT_THAT(jobsStarted.size(), Eq(0));
    ASSERT_THAT(jobs.runningJobs(), Eq(0));
}

TEST_F(Jobs, ProcessQueueWithSingleJob)
{
    jobs.add(createJobRequest(filePath1, JobRequest::Type::UpdateDocumentAnnotations));

    const JobRequests jobsStarted = jobs.process();

    ASSERT_THAT(jobsStarted.size(), Eq(1));
    ASSERT_THAT(jobs.runningJobs(), Eq(1));
}

TEST_F(Jobs, ProcessQueueUntilEmpty)
{
    jobs.add(createJobRequest(filePath1, JobRequest::Type::UpdateDocumentAnnotations));
    jobs.add(createJobRequest(filePath1, JobRequest::Type::UpdateDocumentAnnotations));
    jobs.add(createJobRequest(filePath1, JobRequest::Type::UpdateDocumentAnnotations));

    jobs.process();

    waitUntilJobChainFinished();
}

TEST_F(Jobs, IsJobRunning)
{
    jobs.add(createJobRequest(filePath1, JobRequest::Type::UpdateDocumentAnnotations));
    jobs.process();

    const bool isJobRunning = jobs.isJobRunning(filePath1, projectPartId);

    ASSERT_TRUE(isJobRunning);
}

void Jobs::SetUp()
{
    projects.createOrUpdate({ProjectPartContainer(projectPartId)});

    const QVector<FileContainer> fileContainer{FileContainer(filePath1, projectPartId)};
    translationUnit = translationUnits.create(fileContainer).front();
    translationUnits.setVisibleInEditors({filePath1});
    translationUnits.setUsedByCurrentEditor(filePath1);
}

void Jobs::TearDown()
{
    ASSERT_TRUE(waitUntilAllJobsFinished()); // QFuture/QFutureWatcher is implemented with events
}

bool Jobs::waitUntilAllJobsFinished(int timeOutInMs) const
{
    const auto noJobsRunningAnymore = [this](){ return jobs.runningJobs() == 0; };

    return TestUtils::processEventsUntilTrue(noJobsRunningAnymore, timeOutInMs);
}

bool Jobs::waitUntilJobChainFinished(int timeOutInMs) const
{
    const auto noJobsRunningAnymore = [this]() {
        return jobs.runningJobs() == 0 && jobs.queue().isEmpty();
    };

    return TestUtils::processEventsUntilTrue(noJobsRunningAnymore, timeOutInMs);
}

JobRequest Jobs::createJobRequest(const Utf8String &filePath,
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

} // anonymous
