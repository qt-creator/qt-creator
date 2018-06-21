/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "clangrequesttooltipjob.h"

#include <clangsupport/clangsupportdebugutils.h>
#include <clangsupport/clangcodemodelclientinterface.h>
#include <clangsupport/tooltipmessage.h>

#include <utils/qtcassert.h>

namespace ClangBackEnd {

IAsyncJob::AsyncPrepareResult RequestToolTipJob::prepareAsyncRun()
{
    const JobRequest jobRequest = context().jobRequest;
    QTC_ASSERT(jobRequest.type == JobRequest::Type::RequestToolTip, return AsyncPrepareResult());
    QTC_ASSERT(acquireDocument(), return AsyncPrepareResult());

    const TranslationUnit translationUnit = *m_translationUnit;
    const UnsavedFiles unsavedFiles = *context().unsavedFiles;
    const quint32 line = jobRequest.line;
    const quint32 column = jobRequest.column;
    const Utf8String textCodecName = jobRequest.textCodecName;
    setRunner([translationUnit, unsavedFiles, line, column, textCodecName]() {
        TIME_SCOPE_DURATION("RequestToolTipJobRunner");

        UnsavedFiles theUnsavedFiles = unsavedFiles;
        return translationUnit.tooltip(theUnsavedFiles, textCodecName, line, column);
    });
    return AsyncPrepareResult{translationUnit.id()};
}

void RequestToolTipJob::finalizeAsyncRun()
{
    if (!context().isOutdated()) {
        const AsyncResult result = asyncResult();

        context().client->tooltip(ToolTipMessage(m_pinnedFileContainer,
                                                 result,
                                                 context().jobRequest.ticketNumber));
    }
}

} // namespace ClangBackEnd
