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

#include "clangiasyncjob.h"
#include "dummyclangipcclient.h"
#include "processevents-utilities.h"

#include <clangdocument.h>
#include <clangdocumentprocessor.h>
#include <clangdocuments.h>
#include <clangjobrequest.h>
#include <clangjobs.h>
#include <projects.h>
#include <unsavedfiles.h>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

using namespace ClangBackEnd;

namespace {

class DocumentProcessor : public ::testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    ClangBackEnd::JobRequest createJobRequest(ClangBackEnd::JobRequest::Type type) const;

    bool waitUntilAllJobsFinished(int timeOutInMs = 10000) const;

protected:
    ClangBackEnd::ProjectParts projects;
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::Documents documents{projects, unsavedFiles};
    ClangBackEnd::Document document;

    DummyIpcClient dummyIpcClient;

    Utf8String filePath{Utf8StringLiteral(TESTDATA_DIR"/translationunits.cpp")};
    Utf8String projectPartId{Utf8StringLiteral("/path/to/projectfile")};

    ClangBackEnd::DocumentProcessor documentProcessor{document,
                                                      documents,
                                                      unsavedFiles,
                                                      projects,
                                                      dummyIpcClient};
};

using DocumentProcessorSlowTest = DocumentProcessor;

TEST_F(DocumentProcessor, ProcessEmpty)
{
    const JobRequests jobsStarted = documentProcessor.process();

    ASSERT_THAT(jobsStarted.size(), 0);
}

TEST_F(DocumentProcessorSlowTest, ProcessSingleJob)
{
    const JobRequest jobRequest = createJobRequest(JobRequest::Type::UpdateDocumentAnnotations);
    documentProcessor.addJob(jobRequest);

    const JobRequests jobsStarted = documentProcessor.process();

    ASSERT_THAT(jobsStarted.size(), 1);
}

void DocumentProcessor::SetUp()
{
    projects.createOrUpdate({ProjectPartContainer(projectPartId)});

    const QVector<FileContainer> fileContainer{FileContainer(filePath, projectPartId)};
    document = documents.create(fileContainer).front();
    documents.setVisibleInEditors({filePath});
    documents.setUsedByCurrentEditor(filePath);
}

void DocumentProcessor::TearDown()
{
    ASSERT_TRUE(waitUntilAllJobsFinished()); // QFuture/QFutureWatcher is implemented with events
}

JobRequest DocumentProcessor::createJobRequest(JobRequest::Type type) const
{
    JobRequest jobRequest;
    jobRequest.type = type;
    jobRequest.requirements = JobRequest::requirementsForType(type);
    jobRequest.filePath = filePath;
    jobRequest.projectPartId = projectPartId;
    jobRequest.unsavedFilesChangeTimePoint = unsavedFiles.lastChangeTimePoint();
    jobRequest.documentRevision = document.documentRevision();
    jobRequest.projectChangeTimePoint = projects.project(projectPartId).lastChangeTimePoint();

    return jobRequest;
}

bool DocumentProcessor::waitUntilAllJobsFinished(int timeOutInMs) const
{
    const auto noJobsRunningAnymore = [this](){ return documentProcessor.runningJobs().isEmpty(); };

    return ProcessEventUtilities::processEventsUntilTrue(noJobsRunningAnymore, timeOutInMs);
}

} // anonymous
