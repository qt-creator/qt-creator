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

#include "clangdocuments.h"

namespace ClangBackEnd {

JobContext::JobContext(const JobRequest &jobRequest,
                       Documents *documents,
                       UnsavedFiles *unsavedFiles,
                       ClangCodeModelClientInterface *clientInterface)
    : jobRequest(jobRequest)
    , documents(documents)
    , unsavedFiles(unsavedFiles)
    , client(clientInterface)
{
}

Document JobContext::documentForJobRequest() const
{
    return documents->document(jobRequest.filePath, jobRequest.projectPartId);
}

bool JobContext::isOutdated() const
{
    return !isDocumentOpen() || documentRevisionChanged();
}

bool JobContext::isDocumentOpen() const
{
    const bool hasDocument = documents->hasDocument(jobRequest.filePath, jobRequest.projectPartId);
    if (!hasDocument)
        qCDebug(jobsLog) << "Document already closed for results of" << jobRequest;

    return hasDocument;
}

bool JobContext::documentRevisionChanged() const
{
    const Document &document = documents->document(jobRequest.filePath, jobRequest.projectPartId);
    const bool revisionChanged = document.documentRevision() != jobRequest.documentRevision;

    if (revisionChanged)
        qCDebug(jobsLog) << "Document revision changed for results of" << jobRequest;

    return revisionChanged;
}

} // ClangBackEnd
