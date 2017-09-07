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

#include "googletest.h"

#include "dummyclangipcclient.h"

#include <clangclock.h>
#include <clangdocument.h>
#include <clangdocumentprocessors.h>
#include <clangdocuments.h>
#include <clangdocumentsuspenderresumer.h>
#include <clangtranslationunits.h>
#include <projects.h>
#include <unsavedfiles.h>
#include <utf8string.h>

#include <utils/algorithm.h>

#include <clang-c/Index.h>

using ClangBackEnd::Clock;
using ClangBackEnd::Document;
using ClangBackEnd::JobRequest;
using ClangBackEnd::PreferredTranslationUnit;
using ClangBackEnd::SuspendResumeJobs;
using ClangBackEnd::SuspendResumeJobsEntry;
using ClangBackEnd::TimePoint;

using testing::ContainerEq;
using testing::ElementsAre;
using testing::IsEmpty;

namespace ClangBackEnd {

bool operator==(const SuspendResumeJobsEntry &a, const SuspendResumeJobsEntry &b)
{
    return a.document == b.document
        && a.jobRequestType == b.jobRequestType
        && a.preferredTranslationUnit == b.preferredTranslationUnit;
}

std::ostream &operator<<(std::ostream &os, const SuspendResumeJobsEntry &entry)
{
    os << "SuspendResumeJobsEntry("
       << entry.document.filePath() << ", "
       << entry.jobRequestType << ", "
       << entry.preferredTranslationUnit
       << ")";

    return os;
}

} // ClangBackEnd

namespace {

class DocumentSuspenderResumer : public ::testing::Test
{
protected:
    void SetUp() override;
    Document getDocument(const Utf8String &filePath);
    void categorizeDocuments(int hotDocumentsSize);
    SuspendResumeJobs createSuspendResumeJobs(int hotDocumentsSize = -1);
    static void setParsed(Document &document);

protected:
    ClangBackEnd::ProjectParts projects;
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::Documents documents{projects, unsavedFiles};
    DummyIpcClient dummyIpcClient;
    ClangBackEnd::DocumentProcessors documentProcessors{documents, unsavedFiles, projects,
                                                        dummyIpcClient};

    const Utf8String projectPartId = Utf8StringLiteral("projectPartId");

    const Utf8String filePath1 = Utf8StringLiteral(TESTDATA_DIR"/empty1.cpp");
    const ClangBackEnd::FileContainer fileContainer1{filePath1, projectPartId, Utf8String(), true};

    const Utf8String filePath2 = Utf8StringLiteral(TESTDATA_DIR"/empty2.cpp");
    const ClangBackEnd::FileContainer fileContainer2{filePath2, projectPartId, Utf8String(), true};

    const Utf8String filePath3 = Utf8StringLiteral(TESTDATA_DIR"/empty3.cpp");
    const ClangBackEnd::FileContainer fileContainer3{filePath3, projectPartId, Utf8String(), true};

    std::vector<Document> hotDocuments;
    std::vector<Document> coldDocuments;
};

TEST_F(DocumentSuspenderResumer, CategorizeNoDocuments)
{
    categorizeDocuments(99);

    ASSERT_THAT(hotDocuments, IsEmpty());
    ASSERT_THAT(coldDocuments, IsEmpty());
}

TEST_F(DocumentSuspenderResumer, CategorizeSingleDocument)
{
    documents.create({fileContainer1});

    categorizeDocuments(99);

    ASSERT_THAT(hotDocuments, ElementsAre(getDocument(filePath1)));
    ASSERT_THAT(coldDocuments, IsEmpty());
}

TEST_F(DocumentSuspenderResumer, CategorizeKeepsStableOrder)
{
    documents.create({fileContainer1, fileContainer2});

    categorizeDocuments(99);

    ASSERT_THAT(hotDocuments, ElementsAre(getDocument(filePath1),
                                          getDocument(filePath2)));
}

TEST_F(DocumentSuspenderResumer, CategorizePutsLastVisibleToTopOfHotDocuments)
{
    documents.create({fileContainer1, fileContainer2});
    documents.setVisibleInEditors({filePath1});
    documents.setVisibleInEditors({filePath2});

    categorizeDocuments(99);

    ASSERT_THAT(hotDocuments, ElementsAre(getDocument(filePath2),
                                          getDocument(filePath1)));
}

TEST_F(DocumentSuspenderResumer, CategorizeWithLessDocumentsThanWeCareFor)
{
    documents.create({fileContainer1});

    categorizeDocuments(2);

    ASSERT_THAT(hotDocuments, ElementsAre(getDocument(filePath1)));
    ASSERT_THAT(coldDocuments, IsEmpty());
}

TEST_F(DocumentSuspenderResumer, CategorizeWithZeroHotDocuments)
{
    documents.create({fileContainer1});

    categorizeDocuments(0);

    ASSERT_THAT(hotDocuments, IsEmpty());
    ASSERT_THAT(coldDocuments, ElementsAre(getDocument(filePath1)));
}

TEST_F(DocumentSuspenderResumer, CategorizeWithMoreVisibleDocumentsThanHotDocuments)
{
    const TimePoint timePoint = Clock::now();
    Document document1 = documents.create({fileContainer1})[0];
    document1.setIsVisibleInEditor(true, timePoint);
    Document document2 = documents.create({fileContainer2})[0];
    document2.setIsVisibleInEditor(true, timePoint);

    categorizeDocuments(1);

    ASSERT_THAT(hotDocuments, ElementsAre(getDocument(filePath1), getDocument(filePath2)));
    ASSERT_THAT(coldDocuments, IsEmpty());
}

TEST_F(DocumentSuspenderResumer, DISABLED_WITHOUT_SUSPEND_PATCH(CreateSuspendJobForInvisible))
{
    Document document = documents.create({fileContainer1})[0];
    document.setIsSuspended(false);
    document.setIsVisibleInEditor(false, Clock::now());
    setParsed(document);

    const SuspendResumeJobs expectedJobs = {
        {document, JobRequest::Type::SuspendDocument, PreferredTranslationUnit::RecentlyParsed}
    };

    const SuspendResumeJobs jobs = createSuspendResumeJobs(/*hotDocumentsSize=*/ 0);

    ASSERT_THAT(jobs, ContainerEq(expectedJobs));
}

TEST_F(DocumentSuspenderResumer, DISABLED_WITHOUT_SUSPEND_PATCH(DoNotCreateSuspendJobForVisible))
{
    Document document = documents.create({fileContainer1})[0];
    document.setIsSuspended(false);
    document.setIsVisibleInEditor(true, Clock::now());

    const SuspendResumeJobs jobs = createSuspendResumeJobs(/*hotDocumentsSize=*/ 0);

    ASSERT_THAT(jobs, IsEmpty());
}

TEST_F(DocumentSuspenderResumer, DISABLED_WITHOUT_SUSPEND_PATCH(DoNotCreateSuspendJobForUnparsed))
{
    Document document = documents.create({fileContainer1})[0];
    document.setIsSuspended(false);
    document.setIsVisibleInEditor(true, Clock::now());

    const SuspendResumeJobs jobs = createSuspendResumeJobs(/*hotDocumentsSize=*/ 0);

    ASSERT_THAT(jobs, IsEmpty());
}

TEST_F(DocumentSuspenderResumer, DISABLED_WITHOUT_SUSPEND_PATCH(CreateSuspendJobsForDocumentWithSupportiveTranslationUnit))
{
    Document document = documents.create({fileContainer1})[0];
    document.setIsSuspended(false);
    document.setIsVisibleInEditor(false, Clock::now());
    document.translationUnits().createAndAppend(); // Add supportive translation unit
    setParsed(document);
    const SuspendResumeJobs expectedJobs = {
        {document, JobRequest::Type::SuspendDocument, PreferredTranslationUnit::RecentlyParsed},
        {document, JobRequest::Type::SuspendDocument, PreferredTranslationUnit::PreviouslyParsed},
    };

    const SuspendResumeJobs jobs = createSuspendResumeJobs(/*hotDocumentsSize=*/ 0);

    ASSERT_THAT(jobs, ContainerEq(expectedJobs));
}

TEST_F(DocumentSuspenderResumer, DISABLED_WITHOUT_SUSPEND_PATCH(CreateResumeJob))
{
    Document document = documents.create({fileContainer1})[0];
    document.setIsSuspended(true);
    document.setIsVisibleInEditor(true, Clock::now());
    const SuspendResumeJobs expectedJobs = {
        {document, JobRequest::Type::ResumeDocument, PreferredTranslationUnit::RecentlyParsed}
    };

    const SuspendResumeJobs jobs = createSuspendResumeJobs();

    ASSERT_THAT(jobs, ContainerEq(expectedJobs));
}

TEST_F(DocumentSuspenderResumer, DISABLED_WITHOUT_SUSPEND_PATCH(DoNotCreateResumeJobForInvisible))
{
    Document document = documents.create({fileContainer1})[0];
    document.setIsSuspended(true);
    document.setIsVisibleInEditor(false, Clock::now());

    const SuspendResumeJobs jobs = createSuspendResumeJobs(/*hotDocumentsSize=*/ 0);

    ASSERT_THAT(jobs, IsEmpty());
}

TEST_F(DocumentSuspenderResumer, DISABLED_WITHOUT_SUSPEND_PATCH(CreateResumeJobsForDocumentWithSupportiveTranslationUnit))
{
    Document document = documents.create({fileContainer1})[0];
    document.setIsSuspended(true);
    document.setIsVisibleInEditor(true, Clock::now());
    document.translationUnits().createAndAppend(); // Add supportive translation unit
    const SuspendResumeJobs expectedJobs = {
        {document, JobRequest::Type::ResumeDocument, PreferredTranslationUnit::RecentlyParsed},
        {document, JobRequest::Type::ResumeDocument, PreferredTranslationUnit::PreviouslyParsed},
    };

    const SuspendResumeJobs jobs = createSuspendResumeJobs();

    ASSERT_THAT(jobs, ContainerEq(expectedJobs));
}

TEST_F(DocumentSuspenderResumer, DISABLED_WITHOUT_SUSPEND_PATCH(CreateSuspendAndResumeJobs))
{
    Document hotDocument = documents.create({fileContainer1})[0];
    hotDocument.setIsSuspended(true);
    Document coldDocument = documents.create({fileContainer2})[0];
    setParsed(coldDocument);
    coldDocument.setIsSuspended(false);
    documents.setVisibleInEditors({filePath1});
    const SuspendResumeJobs expectedJobs = {
        {coldDocument, JobRequest::Type::SuspendDocument, PreferredTranslationUnit::RecentlyParsed},
        {hotDocument, JobRequest::Type::ResumeDocument, PreferredTranslationUnit::RecentlyParsed},
    };

    const SuspendResumeJobs jobs = createSuspendResumeJobs(/*hotDocumentsSize=*/ 1);

    ASSERT_THAT(jobs, ContainerEq(expectedJobs));
}

void DocumentSuspenderResumer::SetUp()
{
    projects.createOrUpdate({ClangBackEnd::ProjectPartContainer(projectPartId)});
}

ClangBackEnd::Document DocumentSuspenderResumer::getDocument(const Utf8String &filePath)
{
    return documents.document(filePath, projectPartId);
}

void DocumentSuspenderResumer::categorizeDocuments(int hotDocumentsSize)
{
    categorizeHotColdDocuments(hotDocumentsSize, documents.documents(), hotDocuments,
                               coldDocuments);
}

ClangBackEnd::SuspendResumeJobs
DocumentSuspenderResumer::createSuspendResumeJobs(int hotDocumentsSize)
{
    return ClangBackEnd::createSuspendResumeJobs(documents.documents(), hotDocumentsSize);
}

void DocumentSuspenderResumer::setParsed(ClangBackEnd::Document &document)
{
    const Utf8String first = document.translationUnit().id();
    document.translationUnits().updateParseTimePoint(first, Clock::now());

    const Utf8String second
            = document.translationUnit(PreferredTranslationUnit::LastUninitialized).id();
    if (second != first)
        document.translationUnits().updateParseTimePoint(second, Clock::now());
}

} // anonymous
