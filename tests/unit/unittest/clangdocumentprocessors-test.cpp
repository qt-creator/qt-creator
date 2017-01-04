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
#include <clangdocumentprocessors.h>
#include <clangdocuments.h>
#include <clangexceptions.h>
#include <clangjobrequest.h>
#include <clangjobs.h>
#include <projects.h>
#include <unsavedfiles.h>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

using testing::Eq;

using namespace ClangBackEnd;

namespace {

class DocumentProcessors : public ::testing::Test
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

    ClangBackEnd::JobRequest jobRequest;
    ClangBackEnd::JobContext jobContext;

    ClangBackEnd::DocumentProcessors documentProcessors{documents,
                                                        unsavedFiles,
                                                        projects,
                                                        dummyIpcClient};
};

using DocumentProcessorsSlowTest = DocumentProcessors;

TEST_F(DocumentProcessors, HasNoItemsInitially)
{
    ASSERT_TRUE(documentProcessors.processors().empty());
}

TEST_F(DocumentProcessors, CreateAddsADocumentProcessor)
{
    documentProcessors.create(document);

    ASSERT_THAT(documentProcessors.processors().size(), Eq(1));
}

TEST_F(DocumentProcessors, CreateReturnsDocumentProcessor)
{
    const DocumentProcessor documentProcessor = documentProcessors.create(document);

    ASSERT_THAT(documentProcessor.document(), Eq(document));
}

TEST_F(DocumentProcessors, CreateThrowsForAlreadyExisting)
{
    documentProcessors.create(document);

    ASSERT_THROW(documentProcessors.create(document),
                 ClangBackEnd::DocumentProcessorAlreadyExists);
}

TEST_F(DocumentProcessors, Access)
{
    documentProcessors.create(document);

    const DocumentProcessor documentProcessor = documentProcessors.processor(document);

    ASSERT_THAT(documentProcessor.document(), Eq(document));
}

TEST_F(DocumentProcessors, AccessThrowsForNotExisting)
{
    ASSERT_THROW(documentProcessors.processor(document),
                 ClangBackEnd::DocumentProcessorDoesNotExist);
}

TEST_F(DocumentProcessors, Remove)
{
    documentProcessors.create(document);

    documentProcessors.remove(document);

    ASSERT_TRUE(documentProcessors.processors().empty());
}

TEST_F(DocumentProcessors, RemoveThrowsForNotExisting)
{
    ASSERT_THROW(documentProcessors.remove(document),
                 ClangBackEnd::DocumentProcessorDoesNotExist);
}

TEST_F(DocumentProcessors, ProcessEmpty)
{
    documentProcessors.create(document);

    const JobRequests jobsStarted = documentProcessors.process();

    ASSERT_TRUE(jobsStarted.isEmpty());
}

TEST_F(DocumentProcessorsSlowTest, ProcessSingle)
{
    DocumentProcessor documentProcessor = documentProcessors.create(document);
    const JobRequest jobRequest = createJobRequest(JobRequest::Type::UpdateDocumentAnnotations);
    documentProcessor.addJob(jobRequest);

    const JobRequests jobsStarted = documentProcessors.process();

     ASSERT_THAT(jobsStarted.size(), 1);
}

void DocumentProcessors::SetUp()
{
    projects.createOrUpdate({ProjectPartContainer(projectPartId)});

    const QVector<FileContainer> fileContainer{FileContainer(filePath, projectPartId)};
    document = documents.create(fileContainer).front();
    documents.setVisibleInEditors({filePath});
    documents.setUsedByCurrentEditor(filePath);
}

void DocumentProcessors::TearDown()
{
    ASSERT_TRUE(waitUntilAllJobsFinished()); // QFuture/QFutureWatcher is implemented with events
}

JobRequest DocumentProcessors::createJobRequest(JobRequest::Type type) const
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

bool DocumentProcessors::waitUntilAllJobsFinished(int timeOutInMs) const
{
    const auto noJobsRunningAnymore = [this](){ return documentProcessors.runningJobs().isEmpty(); };

    return ProcessEventUtilities::processEventsUntilTrue(noJobsRunningAnymore, timeOutInMs);
}

} // anonymous
