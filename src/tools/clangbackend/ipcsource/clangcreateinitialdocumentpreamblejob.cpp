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

#include "clangcreateinitialdocumentpreamblejob.h"

#include <clangbackendipc/clangbackendipcdebugutils.h>

#include <utils/qtcassert.h>

namespace ClangBackEnd {

static void runAsyncHelper(const TranslationUnitCore &translationUnitCore,
                           const TranslationUnitUpdateInput &translationUnitUpdateInput)
{
    TIME_SCOPE_DURATION("CreateInitialDocumentPreambleJobRunner");

    try {
        translationUnitCore.reparse(translationUnitUpdateInput);
    } catch (const std::exception &exception) {
        qWarning() << "Error in CreateInitialDocumentPreambleJobRunner:" << exception.what();
    }
}

bool CreateInitialDocumentPreambleJob::prepareAsyncRun()
{
    const JobRequest jobRequest = context().jobRequest;
    QTC_ASSERT(jobRequest.type == JobRequest::Type::CreateInitialDocumentPreamble, return false);

    try {
        m_pinnedTranslationUnit = context().translationUnitForJobRequest();
        m_pinnedFileContainer = m_pinnedTranslationUnit.fileContainer();

        const TranslationUnitCore translationUnitCore = m_pinnedTranslationUnit.translationUnitCore();
        const TranslationUnitUpdateInput updateInput = m_pinnedTranslationUnit.createUpdateInput();
        setRunner([translationUnitCore, updateInput]() {
            return runAsyncHelper(translationUnitCore, updateInput);
        });

    } catch (const std::exception &exception) {
        qWarning() << "Error in CreateInitialDocumentPreambleJob::prepareAsyncRun:"
                   << exception.what();
        return false;
    }

    return true;
}

void CreateInitialDocumentPreambleJob::finalizeAsyncRun()
{
}

} // namespace ClangBackEnd

