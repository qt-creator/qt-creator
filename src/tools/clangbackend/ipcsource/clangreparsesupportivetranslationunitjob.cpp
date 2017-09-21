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

#include "clangreparsesupportivetranslationunitjob.h"

#include <clangsupport/clangsupportdebugutils.h>

#include <utils/qtcassert.h>

namespace ClangBackEnd {

IAsyncJob::AsyncPrepareResult ReparseSupportiveTranslationUnitJob::prepareAsyncRun()
{
    const JobRequest jobRequest = context().jobRequest;
    QTC_ASSERT(jobRequest.type == JobRequest::Type::ReparseSupportiveTranslationUnit, return AsyncPrepareResult());
    QTC_ASSERT(acquireDocument(), return AsyncPrepareResult());

    const TranslationUnit translationUnit = *m_translationUnit;
    const TranslationUnitUpdateInput updateInput = m_pinnedDocument.createUpdateInput();
    setRunner([translationUnit, updateInput]() {
        TIME_SCOPE_DURATION("ReparseSupportiveTranslationUnitJob");

        TranslationUnitUpdateInput theUpdateInput = updateInput;
        theUpdateInput.reparseNeeded = true;

        ReparseSupportiveTranslationUnitJob::AsyncResult asyncResult;
        asyncResult.updateResult = translationUnit.reparse(theUpdateInput);

        return asyncResult;
    });

    return AsyncPrepareResult{translationUnit.id()};
}

void ReparseSupportiveTranslationUnitJob::finalizeAsyncRun()
{
    if (!context().isOutdated()) {
        const AsyncResult result = asyncResult();
        m_pinnedDocument.incorporateUpdaterResult(result.updateResult);
    }
}

} // namespace ClangBackEnd

