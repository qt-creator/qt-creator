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

#include "clangcompletecodejob.h"

#include <clangbackendipc/clangbackendipcdebugutils.h>
#include <clangbackendipc/clangcodemodelclientinterface.h>
#include <clangbackendipc/cmbcodecompletedmessage.h>

#include <utils/qtcassert.h>

namespace ClangBackEnd {

static CompleteCodeJob::AsyncResult runAsyncHelper(const TranslationUnitCore &translationUnitCore,
                                                   UnsavedFiles unsavedFiles,
                                                   quint32 line,
                                                   quint32 column)
{
    TIME_SCOPE_DURATION("CompleteCodeJobRunner");

    CompleteCodeJob::AsyncResult asyncResult;

    try {
        const TranslationUnitCore::CodeCompletionResult results
                = translationUnitCore.complete(unsavedFiles, line, column);

        asyncResult.completions = results.completions;
        asyncResult.correction = results.correction;
    } catch (const std::exception &exception) {
        qWarning() << "Error in CompleteCodeJobRunner:" << exception.what();
    }

    return asyncResult;
}

bool CompleteCodeJob::prepareAsyncRun()
{
    const JobRequest jobRequest = context().jobRequest;
    QTC_ASSERT(jobRequest.type == JobRequest::Type::CompleteCode, return false);

    try {
        m_pinnedTranslationUnit = context().translationUnitForJobRequest();

        const TranslationUnitCore translationUnitCore = m_pinnedTranslationUnit.translationUnitCore();
        const UnsavedFiles unsavedFiles = *context().unsavedFiles;
        const quint32 line = jobRequest.line;
        const quint32 column = jobRequest.column;
        setRunner([translationUnitCore, unsavedFiles, line, column]() {
            return runAsyncHelper(translationUnitCore, unsavedFiles, line, column);
        });


    } catch (const std::exception &exception) {
        qWarning() << "Error in CompleteCodeJob::prepareAsyncRun:" << exception.what();
        return false;
    }

    return true;
}

void CompleteCodeJob::finalizeAsyncRun()
{
    if (context().isDocumentOpen()) {
        const AsyncResult result = asyncResult();

        const CodeCompletedMessage message(result.completions,
                                           result.correction,
                                           context().jobRequest.ticketNumber);
        context().client->codeCompleted(message);
    }
}

} // namespace ClangBackEnd
