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

#include "clangdocumentsuspenderresumer.h"

#include "clangsupport_global.h"
#include "clangdocumentprocessors.h"
#include "clangdocuments.h"

#include <utils/algorithm.h>

#include <algorithm>

namespace ClangBackEnd {

constexpr int DefaultHotDocumentsSize = 7;

void categorizeHotColdDocuments(int hotDocumentsSize,
                                const std::vector<Document> &inDocuments,
                                std::vector<Document> &hotDocuments,
                                std::vector<Document> &coldDocuments)
{
    // Sort documents, most recently used/visible at top
    std::vector<Document> documents = inDocuments;
    std::stable_sort(documents.begin(), documents.end(), [](const Document &a, const Document &b) {
        return a.visibleTimePoint() > b.visibleTimePoint();
    });

    // Ensure that visible documents are always hot, otherwise not all visible
    // documents will be resumed.
    const auto isVisible = [](const Document &document) { return document.isVisibleInEditor(); };
    const int visibleDocumentsSize = Utils::count(documents, isVisible);
    hotDocumentsSize = std::max(hotDocumentsSize, visibleDocumentsSize);

    if (documents.size() <= uint(hotDocumentsSize)) {
        hotDocuments = documents;
        coldDocuments.clear();
    } else {
        const auto firstColdIterator = documents.begin() + hotDocumentsSize;
        hotDocuments = std::vector<Document>(documents.begin(), firstColdIterator);
        coldDocuments = std::vector<Document>(firstColdIterator,  documents.end());
    }
}

#ifdef IS_SUSPEND_SUPPORTED
static int hotDocumentsSize()
{
    static int hotDocuments = -1;
    if (hotDocuments == -1) {
        bool ok = false;
        const int fromEnvironment = qEnvironmentVariableIntValue("QTC_CLANG_HOT_DOCUMENTS", &ok);
        hotDocuments = ok && fromEnvironment >= 1 ? fromEnvironment : DefaultHotDocumentsSize;
    }

    return hotDocuments;
}

static SuspendResumeJobs createJobs(const Document &document, JobRequest::Type type)
{
    SuspendResumeJobs jobs;

    jobs.append({document, type, PreferredTranslationUnit::RecentlyParsed});
    if (document.isResponsivenessIncreased())
        jobs.append({document, type, PreferredTranslationUnit::PreviouslyParsed});

    return jobs;
}

static bool isFineDocument(const Document &document)
{
    return !document.isNull() && document.isIntact();
}

static bool isSuspendable(const Document &document)
{
    return isFineDocument(document)
        && !document.isSuspended()
        && !document.isVisibleInEditor()
        && document.isParsed();
}

static bool isResumable(const Document &document)
{
    return isFineDocument(document)
        && document.isSuspended()
        && document.isVisibleInEditor();
}

#endif // IS_SUSPEND_SUPPORTED

SuspendResumeJobs createSuspendResumeJobs(const std::vector<Document> &documents,
                                          int customHotDocumentSize)
{
    Q_UNUSED(documents);
    Q_UNUSED(customHotDocumentSize);

    SuspendResumeJobs jobs;

#ifdef IS_SUSPEND_SUPPORTED
    std::vector<Document> hotDocuments;
    std::vector<Document> coldDocuments;

    const int size = (customHotDocumentSize == -1) ? hotDocumentsSize() : customHotDocumentSize;
    categorizeHotColdDocuments(size, documents, hotDocuments, coldDocuments);

    // Cold documents should be suspended...
    const std::vector<Document> toSuspend = Utils::filtered(coldDocuments, &isSuspendable);
    for (const Document &document : toSuspend)
        jobs += createJobs(document, JobRequest::Type::SuspendDocument);

    // ...and hot documents that were suspended should be resumed
    const std::vector<Document> toResume = Utils::filtered(hotDocuments, &isResumable);
    for (const Document &document : toResume)
        jobs += createJobs(document, JobRequest::Type::ResumeDocument);
#endif // IS_SUSPEND_SUPPORTED

    return jobs;
}

} // namespace ClangBackEnd
