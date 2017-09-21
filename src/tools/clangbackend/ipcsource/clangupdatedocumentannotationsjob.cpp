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

#include "clangupdatedocumentannotationsjob.h"

#include <clangsupport/clangsupportdebugutils.h>
#include <clangsupport/clangcodemodelclientinterface.h>
#include <clangsupport/documentannotationschangedmessage.h>

#include <utils/qtcassert.h>

namespace ClangBackEnd {

IAsyncJob::AsyncPrepareResult UpdateDocumentAnnotationsJob::prepareAsyncRun()
{
    const JobRequest jobRequest = context().jobRequest;
    QTC_ASSERT(isExpectedJobRequestType(jobRequest), return AsyncPrepareResult());
    QTC_ASSERT(acquireDocument(), return AsyncPrepareResult());

    const TranslationUnit translationUnit = *m_translationUnit;
    const TranslationUnitUpdateInput updateInput = createUpdateInput(m_pinnedDocument);
    setRunner([translationUnit, updateInput]() {
        TIME_SCOPE_DURATION("UpdateDocumentAnnotationsJobRunner");

        // Update
        UpdateDocumentAnnotationsJob::AsyncResult asyncResult;
        asyncResult.updateResult = translationUnit.update(updateInput);

        // Collect
        translationUnit.extractDocumentAnnotations(asyncResult.firstHeaderErrorDiagnostic,
                                                   asyncResult.diagnostics,
                                                   asyncResult.highlightingMarks,
                                                   asyncResult.skippedSourceRanges);

        return asyncResult;
    });

    return AsyncPrepareResult{translationUnit.id()};
}

void UpdateDocumentAnnotationsJob::finalizeAsyncRun()
{
    if (!context().isOutdated()) {
        const AsyncResult result = asyncResult();

        m_pinnedDocument.incorporateUpdaterResult(result.updateResult);
        context().client->documentAnnotationsChanged(
            DocumentAnnotationsChangedMessage(m_pinnedFileContainer,
                                              result.diagnostics,
                                              result.firstHeaderErrorDiagnostic,
                                              result.highlightingMarks,
                                              result.skippedSourceRanges));
    }
}

bool UpdateDocumentAnnotationsJob::isExpectedJobRequestType(const JobRequest &jobRequest) const
{
    return jobRequest.type == JobRequest::Type::UpdateDocumentAnnotations;
}

TranslationUnitUpdateInput
UpdateDocumentAnnotationsJob::createUpdateInput(const Document &document) const
{
    return document.createUpdateInput();
}

} // namespace ClangBackEnd

