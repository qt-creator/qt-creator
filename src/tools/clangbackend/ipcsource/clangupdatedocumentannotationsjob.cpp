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

#include <clangbackendipc/clangbackendipcdebugutils.h>
#include <clangbackendipc/clangcodemodelclientinterface.h>
#include <clangbackendipc/documentannotationschangedmessage.h>

#include <utils/qtcassert.h>

namespace ClangBackEnd {

static UpdateDocumentAnnotationsJob::AsyncResult runAsyncHelper(
        const TranslationUnit &translationUnit,
        const TranslationUnitUpdateInput &translationUnitUpdatInput)
{
    TIME_SCOPE_DURATION("UpdateDocumentAnnotationsJobRunner");

    UpdateDocumentAnnotationsJob::AsyncResult asyncResult;

    // Update
    asyncResult.updateResult = translationUnit.update(translationUnitUpdatInput);

    // Collect
    translationUnit.extractDocumentAnnotations(asyncResult.firstHeaderErrorDiagnostic,
                                               asyncResult.diagnostics,
                                               asyncResult.highlightingMarks,
                                               asyncResult.skippedSourceRanges);

    return asyncResult;
}

IAsyncJob::AsyncPrepareResult UpdateDocumentAnnotationsJob::prepareAsyncRun()
{
    const JobRequest jobRequest = context().jobRequest;
    QTC_ASSERT(jobRequest.type == JobRequest::Type::UpdateDocumentAnnotations,
               return AsyncPrepareResult());

    try {
        m_pinnedDocument = context().documentForJobRequest();
        m_pinnedFileContainer = m_pinnedDocument.fileContainer();

        const TranslationUnit translationUnit
                = m_pinnedDocument.translationUnit(jobRequest.preferredTranslationUnit);
        const TranslationUnitUpdateInput updateInput = m_pinnedDocument.createUpdateInput();
        setRunner([translationUnit, updateInput]() {
            return runAsyncHelper(translationUnit, updateInput);
        });
        return AsyncPrepareResult{translationUnit.id()};

    } catch (const std::exception &exception) {
        qWarning() << "Error in UpdateDocumentAnnotationsJob::prepareAsyncRun:" << exception.what();
        return AsyncPrepareResult();
    }
}

void UpdateDocumentAnnotationsJob::finalizeAsyncRun()
{
    if (!context().isOutdated()) {
        const AsyncResult result = asyncResult();

        incorporateUpdaterResult(result);
        sendAnnotations(result);
    }
}

void UpdateDocumentAnnotationsJob::incorporateUpdaterResult(const AsyncResult &result)
{
    m_pinnedDocument.incorporateUpdaterResult(result.updateResult);
}

void UpdateDocumentAnnotationsJob::sendAnnotations(const AsyncResult &result)
{
    const DocumentAnnotationsChangedMessage message(m_pinnedFileContainer,
                                                    result.diagnostics,
                                                    result.firstHeaderErrorDiagnostic,
                                                    result.highlightingMarks,
                                                    result.skippedSourceRanges);

    context().client->documentAnnotationsChanged(message);
}

} // namespace ClangBackEnd

