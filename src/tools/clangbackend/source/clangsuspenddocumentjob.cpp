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

#include "clangsuspenddocumentjob.h"

#include <clangsupport/clangsupportdebugutils.h>

#include <utils/qtcassert.h>

namespace ClangBackEnd {

IAsyncJob::AsyncPrepareResult SuspendDocumentJob::prepareAsyncRun()
{
    const JobRequest jobRequest = context().jobRequest;
    QTC_ASSERT(jobRequest.type == JobRequest::Type::SuspendDocument, return AsyncPrepareResult());
    QTC_ASSERT(acquireDocument(), return AsyncPrepareResult());

    TranslationUnit translationUnit = *m_translationUnit;
    setRunner([translationUnit]() {
        TIME_SCOPE_DURATION("SuspendDocumentJobRunner");
        return translationUnit.suspend();
    });

    return AsyncPrepareResult{translationUnit.id()};
}

void SuspendDocumentJob::finalizeAsyncRun()
{
    if (context().isDocumentOpen()) {
        const bool suspendSucceeded = asyncResult();
        if (QTC_GUARD(suspendSucceeded)) {
            m_pinnedDocument.setIsSuspended(true);
        }
    }
}

} // namespace ClangBackEnd
