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

#include "clangfollowsymboljob.h"

#include <clangsupport/clangsupportdebugutils.h>
#include <clangsupport/followsymbolmessage.h>
#include <clangsupport/clangcodemodelclientinterface.h>

#include <utils/qtcassert.h>

namespace ClangBackEnd {

IAsyncJob::AsyncPrepareResult FollowSymbolJob::prepareAsyncRun()
{
    const JobRequest jobRequest = context().jobRequest;
    QTC_ASSERT(jobRequest.type == JobRequest::Type::FollowSymbol,
               return AsyncPrepareResult());
    QTC_ASSERT(acquireDocument(), return AsyncPrepareResult());

    const TranslationUnit translationUnit = *m_translationUnit;
    const TranslationUnitUpdateInput updateInput = m_pinnedDocument.createUpdateInput();
    const CommandLineArguments currentArgs(updateInput.filePath.constData(),
                                           updateInput.projectArguments,
                                           updateInput.fileArguments,
                                           false);

    const quint32 line = jobRequest.line;
    const quint32 column = jobRequest.column;
    const QVector<Utf8String> &dependentFiles = jobRequest.dependentFiles;
    setRunner([translationUnit, line, column, dependentFiles, currentArgs]() {
        TIME_SCOPE_DURATION("FollowSymbolJobRunner");
        return translationUnit.followSymbol(line, column, dependentFiles, currentArgs);
    });

    return AsyncPrepareResult{translationUnit.id()};
}

void FollowSymbolJob::finalizeAsyncRun()
{
    if (!context().isOutdated()) {
        const AsyncResult result = asyncResult();

        const FollowSymbolMessage message(m_pinnedFileContainer,
                                          result,
                                          context().jobRequest.ticketNumber);
        context().client->followSymbol(message);
    }
}

} // namespace ClangBackEnd
