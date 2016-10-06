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

#include <clangdocument.h>
#include <clangdocuments.h>
#include <clangtranslationunit.h>
#include <clangtranslationunits.h>
#include <clangjobs.h>
#include <filecontainer.h>
#include <projectpart.h>
#include <projects.h>
#include <unsavedfiles.h>

#include <clang-c/Index.h>

#include <QTemporaryFile>

using namespace ClangBackEnd;

using testing::IsNull;
using testing::NotNull;
using testing::Eq;
using testing::Gt;
using testing::Contains;
using testing::EndsWith;
using testing::AllOf;

namespace {

class JobQueue : public ::testing::Test
{
protected:
    void SetUp() override;

    void resetVisibilityAndCurrentEditor();

    Utf8String createTranslationUnitForDeletedFile();

    JobRequest createJobRequest(const Utf8String &filePath,
                                JobRequest::Type type,
                                PreferredTranslationUnit preferredTranslationUnit
                                    = PreferredTranslationUnit::RecentlyParsed) const;

    void updateDocumentRevision();
    void updateUnsavedFiles();
    void updateProject();
    void removeProject();
    void removeDocument();

protected:
    ClangBackEnd::ProjectParts projects;
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::Documents documents{projects, unsavedFiles};
    ClangBackEnd::Document document;

    Utf8String filePath1 = Utf8StringLiteral(TESTDATA_DIR"/translationunits.cpp");
    Utf8String filePath2 = Utf8StringLiteral(TESTDATA_DIR"/skippedsourceranges.cpp");
    Utf8String projectPartId{Utf8StringLiteral("/path/to/projectfile")};

    ClangBackEnd::JobQueue jobQueue{documents, projects};
};

TEST_F(JobQueue, AddJob)
{
    const JobRequest jobRequest = createJobRequest(filePath1,
                                                   JobRequest::Type::UpdateDocumentAnnotations);

    jobQueue.add(jobRequest);

    ASSERT_THAT(jobQueue.queue().size(), Eq(1));
}

TEST_F(JobQueue, DoNotAddDuplicate)
{
    const JobRequest request = createJobRequest(filePath1,
                                                JobRequest::Type::UpdateDocumentAnnotations);
    jobQueue.add(request);

    const bool added = jobQueue.add(request);

    ASSERT_FALSE(added);
}

TEST_F(JobQueue, DoNotAddDuplicateForWhichAJobIsAlreadyRunning)
{
    jobQueue.setIsJobRunningForJobRequestHandler([](const JobRequest &) {
       return true;
    });

    const bool added = jobQueue.add(createJobRequest(filePath1,
                                                     JobRequest::Type::UpdateDocumentAnnotations));

    ASSERT_FALSE(added);
}

TEST_F(JobQueue, ProcessEmpty)
{
    jobQueue.processQueue();

    ASSERT_THAT(jobQueue.size(), Eq(0));
}

TEST_F(JobQueue, ProcessSingleJob)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateDocumentAnnotations));

    const JobRequests jobsToRun = jobQueue.processQueue();

    ASSERT_THAT(jobsToRun.size(), Eq(1));
    ASSERT_THAT(jobQueue.size(), Eq(0));
}

TEST_F(JobQueue, ProcessUntilEmpty)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateDocumentAnnotations));
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::CreateInitialDocumentPreamble));

    JobRequests jobsToRun;
    ASSERT_THAT(jobQueue.size(), Eq(2));

    jobsToRun = jobQueue.processQueue();
    ASSERT_THAT(jobQueue.size(), Eq(1));
    ASSERT_THAT(jobsToRun.size(), Eq(1));

    jobsToRun = jobQueue.processQueue();
    ASSERT_THAT(jobQueue.size(), Eq(0));
    ASSERT_THAT(jobsToRun.size(), Eq(1));
}

TEST_F(JobQueue, RemoveRequestsForClosedDocuments)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateDocumentAnnotations));
    removeDocument();

    const JobRequests jobsToRun = jobQueue.processQueue();

    ASSERT_THAT(jobQueue.size(), Eq(0));
    ASSERT_THAT(jobsToRun.size(), Eq(0));
}

TEST_F(JobQueue, RemoveRequestsForClosedProject)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateDocumentAnnotations));
    removeProject();

    const JobRequests jobsToRun = jobQueue.processQueue();

    ASSERT_THAT(jobQueue.size(), Eq(0));
    ASSERT_THAT(jobsToRun.size(), Eq(0));
}

TEST_F(JobQueue, RemoveRequestsForOudatedUnsavedFiles)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateDocumentAnnotations));
    updateUnsavedFiles();

    const JobRequests jobsToRun = jobQueue.processQueue();

    ASSERT_THAT(jobQueue.size(), Eq(0));
    ASSERT_THAT(jobsToRun.size(), Eq(0));
}

TEST_F(JobQueue, RemoveRequestsForChangedDocumentRevision)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateDocumentAnnotations));
    updateDocumentRevision();

    const JobRequests jobsToRun = jobQueue.processQueue();

    ASSERT_THAT(jobQueue.size(), Eq(0));
    ASSERT_THAT(jobsToRun.size(), Eq(0));
}

TEST_F(JobQueue, RemoveRequestsForOudatedProject)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateDocumentAnnotations));
    updateProject();

    const JobRequests jobsToRun = jobQueue.processQueue();

    ASSERT_THAT(jobQueue.size(), Eq(0));
    ASSERT_THAT(jobsToRun.size(), Eq(0));
}

TEST_F(JobQueue, RemoveRequestsForNotIntactDocuments)
{
    const Utf8String filePath = createTranslationUnitForDeletedFile();
    jobQueue.add(createJobRequest(filePath, JobRequest::Type::UpdateDocumentAnnotations));

    const JobRequests jobsToRun = jobQueue.processQueue();

    ASSERT_THAT(jobQueue.size(), Eq(0));
    ASSERT_THAT(jobsToRun.size(), Eq(0));
}

TEST_F(JobQueue, PrioritizeCurrentDocumentOverNotCurrent)
{
    resetVisibilityAndCurrentEditor();
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateDocumentAnnotations));
    jobQueue.add(createJobRequest(filePath2, JobRequest::Type::UpdateDocumentAnnotations));
    documents.setUsedByCurrentEditor(filePath2);

    jobQueue.prioritizeRequests();

    ASSERT_THAT(jobQueue.queue().first().filePath, Eq(filePath2));
}

TEST_F(JobQueue, PrioritizeVisibleDocumentsOverNotVisible)
{
    resetVisibilityAndCurrentEditor();
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateDocumentAnnotations));
    jobQueue.add(createJobRequest(filePath2, JobRequest::Type::UpdateDocumentAnnotations));
    documents.setVisibleInEditors({filePath2});

    jobQueue.prioritizeRequests();

    ASSERT_THAT(jobQueue.queue().first().filePath, Eq(filePath2));
}

TEST_F(JobQueue, PrioritizeCurrentDocumentOverVisible)
{
    resetVisibilityAndCurrentEditor();
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateDocumentAnnotations));
    jobQueue.add(createJobRequest(filePath2, JobRequest::Type::UpdateDocumentAnnotations));
    documents.setVisibleInEditors({filePath1, filePath2});
    documents.setUsedByCurrentEditor(filePath2);

    jobQueue.prioritizeRequests();

    ASSERT_THAT(jobQueue.queue().first().filePath, Eq(filePath2));
}

TEST_F(JobQueue, RunNothingForNotCurrentOrVisibleDocument)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateDocumentAnnotations));
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::CreateInitialDocumentPreamble));
    documents.setVisibleInEditors({});
    documents.setUsedByCurrentEditor(Utf8StringLiteral("aNonExistingFilePath"));

    const JobRequests jobsToRun = jobQueue.processQueue();

    ASSERT_THAT(jobsToRun.size(), Eq(0));
}

TEST_F(JobQueue, RunOnlyOneJobPerTranslationUnitIfMultipleAreInQueue)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateDocumentAnnotations));
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::CreateInitialDocumentPreamble));

    const JobRequests jobsToRun = jobQueue.processQueue();

    ASSERT_THAT(jobsToRun.size(), Eq(1));
    ASSERT_THAT(jobQueue.size(), Eq(1));
}

TEST_F(JobQueue, RunJobsForDistinctTranslationUnits)
{
    const TranslationUnit initialTu = document.translationUnit();
    document.translationUnits().updateParseTimePoint(initialTu.id(), std::chrono::steady_clock::now());
    const TranslationUnit alternativeTu = document.translationUnits().createAndAppend();
    document.translationUnits().updateParseTimePoint(alternativeTu.id(), std::chrono::steady_clock::now());
    jobQueue.add(createJobRequest(filePath1,
                                  JobRequest::Type::UpdateDocumentAnnotations,
                                  PreferredTranslationUnit::RecentlyParsed));
    jobQueue.add(createJobRequest(filePath1,
                                  JobRequest::Type::UpdateDocumentAnnotations,
                                  PreferredTranslationUnit::PreviouslyParsed));

    const JobRequests jobsToRun = jobQueue.processQueue();

    ASSERT_THAT(jobsToRun.size(), Eq(2));
    ASSERT_THAT(jobQueue.size(), Eq(0));
}
TEST_F(JobQueue, DoNotRunJobForTranslationUnittThatIsBeingProcessed)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateDocumentAnnotations));
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::CreateInitialDocumentPreamble));
    JobRequests jobsToRun = jobQueue.processQueue();
    jobQueue.setIsJobRunningForTranslationUnitHandler([](const Utf8String &) {
       return true;
    });

    jobsToRun = jobQueue.processQueue();

    ASSERT_THAT(jobsToRun.size(), Eq(0));
}

TEST_F(JobQueue, RequestUpdateDocumentAnnotationsOutdatableByUnsavedFileChange)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateDocumentAnnotations));
    updateUnsavedFiles();

    const JobRequests jobsToStart = jobQueue.processQueue();

    ASSERT_THAT(jobsToStart.size(), Eq(0));
}

TEST_F(JobQueue, RequestUpdateDocumentAnnotationsOutdatableByProjectRemoval)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateDocumentAnnotations));
    removeProject();

    const JobRequests jobsToStart = jobQueue.processQueue();

    ASSERT_THAT(jobsToStart.size(), Eq(0));
}

TEST_F(JobQueue, RequestUpdateDocumentAnnotationsOutdatableByProjectChange)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateDocumentAnnotations));
    updateProject();

    const JobRequests jobsToStart = jobQueue.processQueue();

    ASSERT_THAT(jobsToStart.size(), Eq(0));
}

TEST_F(JobQueue, RequestUpdateDocumentAnnotationsOutdatableByDocumentClose)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateDocumentAnnotations));
    removeDocument();

    const JobRequests jobsToStart = jobQueue.processQueue();

    ASSERT_THAT(jobsToStart.size(), Eq(0));
}

TEST_F(JobQueue, RequestUpdateDocumentAnnotationsOutdatableByNotIntactDocument)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateDocumentAnnotations));
    document.setHasParseOrReparseFailed(true);

    const JobRequests jobsToStart = jobQueue.processQueue();

    ASSERT_THAT(jobsToStart.size(), Eq(0));
}

TEST_F(JobQueue, RequestCompleteCodeOutdatableByDocumentClose)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::CompleteCode));
    removeDocument();

    const JobRequests jobsToStart = jobQueue.processQueue();

    ASSERT_THAT(jobsToStart.size(), Eq(0));
}

TEST_F(JobQueue, RequestCompleteCodeNotOutdatableByUnsavedFilesChange)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::CompleteCode));
    updateUnsavedFiles();

    const JobRequests jobsToStart = jobQueue.processQueue();

    ASSERT_THAT(jobsToStart.size(), Eq(1));
}

TEST_F(JobQueue, RequestCompleteCodeNotOutdatableByDocumentRevisionChange)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::CompleteCode));
    updateDocumentRevision();

    const JobRequests jobsToStart = jobQueue.processQueue();

    ASSERT_THAT(jobsToStart.size(), Eq(1));
}

TEST_F(JobQueue, RequestCreateInitialDocumentPreambleOutdatableByDocumentClose)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::CreateInitialDocumentPreamble));
    removeDocument();

    const JobRequests jobsToStart = jobQueue.processQueue();

    ASSERT_THAT(jobsToStart.size(), Eq(0));
}

TEST_F(JobQueue, RequestCompleteCodeOutdatableByDocumentRevisionChange)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::RequestDocumentAnnotations));
    updateDocumentRevision();

    const JobRequests jobsToStart = jobQueue.processQueue();

    ASSERT_THAT(jobsToStart.size(), Eq(0));
}

void JobQueue::SetUp()
{
    projects.createOrUpdate({ProjectPartContainer(projectPartId)});

    const QVector<FileContainer> fileContainer{FileContainer(filePath1, projectPartId),
                                               FileContainer(filePath2, projectPartId)};
    document = documents.create(fileContainer).front();
    documents.setVisibleInEditors({filePath1});
    documents.setUsedByCurrentEditor(filePath1);
}

void JobQueue::resetVisibilityAndCurrentEditor()
{
    documents.setVisibleInEditors({});
    documents.setUsedByCurrentEditor(Utf8String());
}

Utf8String JobQueue::createTranslationUnitForDeletedFile()
{
    QTemporaryFile temporaryFile(QLatin1String("XXXXXX.cpp"));
    EXPECT_TRUE(temporaryFile.open());
    const QString temporaryFilePath = Utf8String::fromString(temporaryFile.fileName());

    ClangBackEnd::FileContainer fileContainer(temporaryFilePath,
                                              projectPartId, Utf8String(), true);
    documents.create({fileContainer});
    auto document = documents.document(fileContainer);
    document.setIsUsedByCurrentEditor(true);

    return temporaryFilePath;
}

JobRequest JobQueue::createJobRequest(
        const Utf8String &filePath,
        JobRequest::Type type,
        PreferredTranslationUnit preferredTranslationUnit) const
{
    JobRequest jobRequest;
    jobRequest.type = type;
    jobRequest.requirements = JobRequest::requirementsForType(type);
    jobRequest.filePath = filePath;
    jobRequest.projectPartId = projectPartId;
    jobRequest.unsavedFilesChangeTimePoint = unsavedFiles.lastChangeTimePoint();
    jobRequest.documentRevision = document.documentRevision();
    jobRequest.preferredTranslationUnit = preferredTranslationUnit;
    jobRequest.projectChangeTimePoint = projects.project(projectPartId).lastChangeTimePoint();

    return jobRequest;
}

void JobQueue::updateDocumentRevision()
{
    documents.update({FileContainer(filePath1, projectPartId, Utf8String(), true, 1)});
}

void JobQueue::updateUnsavedFiles()
{
    unsavedFiles.createOrUpdate({FileContainer(filePath1, projectPartId, Utf8String(), true, 1)});
}

void JobQueue::updateProject()
{
    projects.createOrUpdate({projectPartId});
}

void JobQueue::removeProject()
{
    projects.remove({projectPartId});
}

void JobQueue::removeDocument()
{
    documents.remove({FileContainer(filePath1, projectPartId)});
}

} // anonymous
