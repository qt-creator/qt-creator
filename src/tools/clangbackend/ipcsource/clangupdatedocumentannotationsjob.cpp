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
        const TranslationUnitCore &translationUnitCore,
        const TranslationUnitUpdateInput &translationUnitUpdatInput)
{
    TIME_SCOPE_DURATION("UpdateDocumentAnnotationsJobRunner");

    UpdateDocumentAnnotationsJob::AsyncResult asyncResult;

    try {
        // Update
        asyncResult.updateResult = translationUnitCore.update(translationUnitUpdatInput);

        // Collect
        translationUnitCore.extractDocumentAnnotations(asyncResult.diagnostics,
                                                       asyncResult.highlightingMarks,
                                                       asyncResult.skippedSourceRanges);

    } catch (const std::exception &exception) {
        qWarning() << "Error in UpdateDocumentAnnotationsJobRunner:" << exception.what();
    }

    return asyncResult;
}

bool UpdateDocumentAnnotationsJob::prepareAsyncRun()
{
    const JobRequest jobRequest = context().jobRequest;
    QTC_ASSERT(jobRequest.type == JobRequest::Type::UpdateDocumentAnnotations, return false);

    try {
        m_pinnedDocument = context().documentForJobRequest();
        m_pinnedFileContainer = m_pinnedDocument.fileContainer();

        const TranslationUnitCore translationUnitCore = m_pinnedDocument.translationUnitCore();
        const TranslationUnitUpdateInput updateInput = m_pinnedDocument.createUpdateInput();
        setRunner([translationUnitCore, updateInput]() {
            return runAsyncHelper(translationUnitCore, updateInput);
        });

    } catch (const std::exception &exception) {
        qWarning() << "Error in UpdateDocumentAnnotationsJob::prepareAsyncRun:" << exception.what();
        return false;
    }

    return true;
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
                                                    result.highlightingMarks,
                                                    result.skippedSourceRanges);

    context().client->documentAnnotationsChanged(message);
}

} // namespace ClangBackEnd

