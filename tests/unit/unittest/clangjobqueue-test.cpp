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

    void pretendParsedTranslationUnit();

    void updateDocumentRevision();
    void updateUnsavedFiles();
    void removeDocument();

protected:
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::Documents documents{unsavedFiles};
    ClangBackEnd::Document document;

    Utf8String filePath1 = Utf8StringLiteral(TESTDATA_DIR"/translationunits.cpp");
    Utf8String filePath2 = Utf8StringLiteral(TESTDATA_DIR"/skippedsourceranges.cpp");

    ClangBackEnd::JobQueue jobQueue{documents};
};

TEST_F(JobQueue, AddJob)
{
    const JobRequest jobRequest = createJobRequest(filePath1,
                                                   JobRequest::Type::UpdateAnnotations);

    jobQueue.add(jobRequest);

    ASSERT_THAT(jobQueue.queue().size(), Eq(1));
}

TEST_F(JobQueue, DoNotAddDuplicate)
{
    const JobRequest request = createJobRequest(filePath1,
                                                JobRequest::Type::UpdateAnnotations);
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
                                                     JobRequest::Type::UpdateAnnotations));

    ASSERT_FALSE(added);
}

TEST_F(JobQueue, DoNotAddForNotExistingDocument)
{
    jobQueue.setCancelJobRequest([](const JobRequest &) {
       return true;
    });

    const bool added = jobQueue.add(createJobRequest(Utf8StringLiteral("notExistingDocument.cpp"),
                                                     JobRequest::Type::UpdateAnnotations));

    ASSERT_FALSE(added);
}

TEST_F(JobQueue, DoNotAddForNotIntactDocument)
{
    document.setHasParseOrReparseFailed(true);
    const bool added = jobQueue.add(createJobRequest(filePath1,
                                                     JobRequest::Type::UpdateAnnotations));

    ASSERT_FALSE(added);
}

TEST_F(JobQueue, CancelDuringAddForNotIntactDocument)
{
    document.setHasParseOrReparseFailed(true);
    bool canceled = false;
    jobQueue.setCancelJobRequest([&canceled](const JobRequest &) {
        canceled = true;
    });


    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateAnnotations));

    ASSERT_TRUE(canceled);
}

TEST_F(JobQueue, ProcessEmpty)
{
    jobQueue.processQueue();

    ASSERT_THAT(jobQueue.size(), Eq(0));
}

TEST_F(JobQueue, ProcessSingleJob)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateAnnotations));

    const JobRequests jobsToRun = jobQueue.processQueue();

    ASSERT_THAT(jobsToRun.size(), Eq(1));
    ASSERT_THAT(jobQueue.size(), Eq(0));
}

TEST_F(JobQueue, ProcessUntilEmpty)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateAnnotations));
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::ParseSupportiveTranslationUnit));

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
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateAnnotations));
    removeDocument();

    const JobRequests jobsToRun = jobQueue.processQueue();

    ASSERT_THAT(jobQueue.size(), Eq(0));
    ASSERT_THAT(jobsToRun.size(), Eq(0));
}

TEST_F(JobQueue, RemoveRequestsForOudatedUnsavedFiles)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateAnnotations));
    updateUnsavedFiles();

    const JobRequests jobsToRun = jobQueue.processQueue();

    ASSERT_THAT(jobQueue.size(), Eq(0));
    ASSERT_THAT(jobsToRun.size(), Eq(0));
}

TEST_F(JobQueue, RemoveRequestsForChangedDocumentRevision)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateAnnotations));
    updateDocumentRevision();

    const JobRequests jobsToRun = jobQueue.processQueue();

    ASSERT_THAT(jobQueue.size(), Eq(0));
    ASSERT_THAT(jobsToRun.size(), Eq(0));
}

TEST_F(JobQueue, RemoveRequestsForNotIntactDocuments)
{
    const Utf8String filePath = createTranslationUnitForDeletedFile();
    jobQueue.add(createJobRequest(filePath, JobRequest::Type::UpdateAnnotations));

    const JobRequests jobsToRun = jobQueue.processQueue();

    ASSERT_THAT(jobQueue.size(), Eq(0));
    ASSERT_THAT(jobsToRun.size(), Eq(0));
}

TEST_F(JobQueue, CancelRequestsForNotIntactDocuments)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateAnnotations));
    document.setHasParseOrReparseFailed(true);
    bool canceled = false;
    jobQueue.setCancelJobRequest([&canceled](const JobRequest &) {
        canceled = true;
    });

    jobQueue.processQueue();

    ASSERT_TRUE(canceled);
}

TEST_F(JobQueue, PrioritizeCurrentDocumentOverNotCurrent)
{
    resetVisibilityAndCurrentEditor();
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateAnnotations));
    jobQueue.add(createJobRequest(filePath2, JobRequest::Type::UpdateAnnotations));
    documents.setUsedByCurrentEditor(filePath2);

    jobQueue.prioritizeRequests();

    ASSERT_THAT(jobQueue.queue().first().filePath, Eq(filePath2));
}

TEST_F(JobQueue, PrioritizeVisibleDocumentsOverNotVisible)
{
    resetVisibilityAndCurrentEditor();
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateAnnotations));
    jobQueue.add(createJobRequest(filePath2, JobRequest::Type::UpdateAnnotations));
    documents.setVisibleInEditors({filePath2});

    jobQueue.prioritizeRequests();

    ASSERT_THAT(jobQueue.queue().first().filePath, Eq(filePath2));
}

TEST_F(JobQueue, PrioritizeCurrentDocumentOverVisible)
{
    resetVisibilityAndCurrentEditor();
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateAnnotations));
    jobQueue.add(createJobRequest(filePath2, JobRequest::Type::UpdateAnnotations));
    documents.setVisibleInEditors({filePath1, filePath2});
    documents.setUsedByCurrentEditor(filePath2);

    jobQueue.prioritizeRequests();

    ASSERT_THAT(jobQueue.queue().first().filePath, Eq(filePath2));
}

TEST_F(JobQueue, RunNothingForNotCurrentOrVisibleDocument)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateAnnotations));
    documents.setVisibleInEditors({});
    documents.setUsedByCurrentEditor(Utf8StringLiteral("aNonExistingFilePath"));

    const JobRequests jobsToRun = jobQueue.processQueue();

    ASSERT_THAT(jobsToRun.size(), Eq(0));
}

TEST_F(JobQueue, RunOnlyOneJobPerTranslationUnitIfMultipleAreInQueue)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateAnnotations));
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::RequestAnnotations));

    const JobRequests jobsToRun = jobQueue.processQueue();

    ASSERT_THAT(jobsToRun.size(), Eq(1));
    ASSERT_THAT(jobQueue.size(), Eq(1));
}

TEST_F(JobQueue, RunJobsForDistinctTranslationUnits)
{
    const TranslationUnit initialTu = document.translationUnit();
    document.translationUnits().updateParseTimePoint(initialTu.id(), Clock::now());
    const TranslationUnit alternativeTu = document.translationUnits().createAndAppend();
    document.translationUnits().updateParseTimePoint(alternativeTu.id(), Clock::now());
    jobQueue.add(createJobRequest(filePath1,
                                  JobRequest::Type::UpdateAnnotations,
                                  PreferredTranslationUnit::RecentlyParsed));
    jobQueue.add(createJobRequest(filePath1,
                                  JobRequest::Type::UpdateAnnotations,
                                  PreferredTranslationUnit::PreviouslyParsed));

    const JobRequests jobsToRun = jobQueue.processQueue();

    ASSERT_THAT(jobsToRun.size(), Eq(2));
    ASSERT_THAT(jobQueue.size(), Eq(0));
}
TEST_F(JobQueue, DoNotRunJobForTranslationUnittThatIsBeingProcessed)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateAnnotations));
    JobRequests jobsToRun = jobQueue.processQueue();
    jobQueue.setIsJobRunningForTranslationUnitHandler([](const Utf8String &) {
       return true;
    });

    jobsToRun = jobQueue.processQueue();

    ASSERT_THAT(jobsToRun.size(), Eq(0));
}

TEST_F(JobQueue, RequestUpdateAnnotationsOutdatableByUnsavedFileChange)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateAnnotations));
    updateUnsavedFiles();

    const JobRequests jobsToStart = jobQueue.processQueue();

    ASSERT_THAT(jobsToStart.size(), Eq(0));
}

TEST_F(JobQueue, RequestUpdateAnnotationsOutdatableByDocumentClose)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateAnnotations));
    removeDocument();

    const JobRequests jobsToStart = jobQueue.processQueue();

    ASSERT_THAT(jobsToStart.size(), Eq(0));
}

TEST_F(JobQueue, RequestUpdateAnnotationsOutdatableByNotIntactDocument)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::UpdateAnnotations));
    document.setHasParseOrReparseFailed(true);

    const JobRequests jobsToStart = jobQueue.processQueue();

    ASSERT_THAT(jobsToStart.size(), Eq(0));
}

TEST_F(JobQueue, RequestCompleteCodeOutdatableByDocumentClose)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::RequestCompletions));
    removeDocument();

    const JobRequests jobsToStart = jobQueue.processQueue();

    ASSERT_THAT(jobsToStart.size(), Eq(0));
}

TEST_F(JobQueue, RequestCompleteCodeNotOutdatableByUnsavedFilesChange)
{
    pretendParsedTranslationUnit();
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::RequestCompletions));
    updateUnsavedFiles();

    const JobRequests jobsToStart = jobQueue.processQueue();

    ASSERT_THAT(jobsToStart.size(), Eq(1));
}

TEST_F(JobQueue, RequestCompleteCodeNotOutdatableByDocumentRevisionChange)
{
    pretendParsedTranslationUnit();
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::RequestCompletions));
    updateDocumentRevision();

    const JobRequests jobsToStart = jobQueue.processQueue();

    ASSERT_THAT(jobsToStart.size(), Eq(1));
}

TEST_F(JobQueue, RequestCompleteCodeOutdatableByDocumentRevisionChange)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::RequestAnnotations));
    updateDocumentRevision();

    const JobRequests jobsToStart = jobQueue.processQueue();

    ASSERT_THAT(jobsToStart.size(), Eq(0));
}

TEST_F(JobQueue, RequestReferencesRunsForCurrentDocumentRevision)
{
    pretendParsedTranslationUnit();
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::RequestReferences));

    const JobRequests jobsToStart = jobQueue.processQueue();

    ASSERT_THAT(jobsToStart.size(), Eq(1));
}

TEST_F(JobQueue, RequestReferencesOutdatableByDocumentClose)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::RequestReferences));
    removeDocument();

    const JobRequests jobsToStart = jobQueue.processQueue();

    ASSERT_THAT(jobsToStart.size(), Eq(0));
    ASSERT_THAT(jobQueue.size(), Eq(0));
}

TEST_F(JobQueue, RequestReferencesDoesNotRunOnSuspendedDocument)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::RequestReferences));
    document.setIsSuspended(true);

    const JobRequests jobsToStart = jobQueue.processQueue();

    ASSERT_THAT(jobsToStart.size(), Eq(0));
    ASSERT_THAT(jobQueue.size(), Eq(1));
}

TEST_F(JobQueue, ResumeDocumentDoesNotRunOnUnsuspended)
{
    jobQueue.add(createJobRequest(filePath1, JobRequest::Type::ResumeDocument));
    document.setIsSuspended(false);

    const JobRequests jobsToStart = jobQueue.processQueue();

    ASSERT_THAT(jobsToStart.size(), Eq(0));
    ASSERT_THAT(jobQueue.size(), Eq(1));
}

void JobQueue::SetUp()
{
    const QVector<FileContainer> fileContainer{FileContainer(filePath1),
                                               FileContainer(filePath2)};
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

    ClangBackEnd::FileContainer fileContainer(temporaryFilePath, Utf8String(), true);
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
    JobRequest jobRequest(type);
    jobRequest.filePath = filePath;
    jobRequest.unsavedFilesChangeTimePoint = unsavedFiles.lastChangeTimePoint();
    jobRequest.documentRevision = document.documentRevision();
    jobRequest.preferredTranslationUnit = preferredTranslationUnit;

    return jobRequest;
}

void JobQueue::pretendParsedTranslationUnit()
{
    document.translationUnits().updateParseTimePoint(document.translationUnit().id(), Clock::now());
}

void JobQueue::updateDocumentRevision()
{
    documents.update({FileContainer(filePath1, Utf8String(), true, 1)});
}

void JobQueue::updateUnsavedFiles()
{
    unsavedFiles.createOrUpdate({FileContainer(filePath1, Utf8String(), true, 1)});
}

void JobQueue::removeDocument()
{
    documents.remove({FileContainer(filePath1)});
}

} // anonymous
