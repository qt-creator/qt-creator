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

#include "clangrequestreferencesjob.h"

#include <clangsupport/clangsupportdebugutils.h>
#include <clangsupport/referencesmessage.h>
#include <clangsupport/clangcodemodelclientinterface.h>

#include <utils/qtcassert.h>

namespace ClangBackEnd {

IAsyncJob::AsyncPrepareResult RequestReferencesJob::prepareAsyncRun()
{
    const JobRequest jobRequest = context().jobRequest;
    QTC_ASSERT(jobRequest.type == JobRequest::Type::RequestReferences,
               return AsyncPrepareResult());
    QTC_ASSERT(acquireDocument(), return AsyncPrepareResult());

    const TranslationUnit translationUnit = *m_translationUnit;
    const quint32 line = jobRequest.line;
    const quint32 column = jobRequest.column;
    setRunner([translationUnit, line, column]() {
        TIME_SCOPE_DURATION("RequestReferencesJobRunner");
        return translationUnit.references(line, column);
    });

    return AsyncPrepareResult{translationUnit.id()};
}

void RequestReferencesJob::finalizeAsyncRun()
{
    if (!context().isOutdated()) {
        const AsyncResult result = asyncResult();

        const ReferencesMessage message(m_pinnedFileContainer,
                                        result.references,
                                        result.isLocalVariable,
                                        context().jobRequest.ticketNumber);
        context().client->references(message);
    }
}

} // namespace ClangBackEnd
