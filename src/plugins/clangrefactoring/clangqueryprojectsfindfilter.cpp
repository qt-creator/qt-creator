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

#include "clangqueryprojectsfindfilter.h"

#include "refactoringclient.h"
#include "searchinterface.h"

#include <refactoringserverinterface.h>

namespace ClangRefactoring {

ClangQueryProjectsFindFilter::ClangQueryProjectsFindFilter(
        ClangBackEnd::RefactoringServerInterface &server,
        SearchInterface &searchInterface,
        RefactoringClient &refactoringClient)
    : server(server),
      searchInterface(searchInterface),
      refactoringClient(refactoringClient)
{
}

QString ClangQueryProjectsFindFilter::id() const
{
    return QStringLiteral("Clang Query Project");
}

QString ClangQueryProjectsFindFilter::displayName() const
{
    return tr("Clang Query Project");
}

bool ClangQueryProjectsFindFilter::isEnabled() const
{
    return true;
}

void ClangQueryProjectsFindFilter::findAll(const QString &queryText, Core::FindFlags)
{
    searchHandle = searchInterface.startNewSearch(tr("Clang Query"), queryText);

    refactoringClient.setSearchHandle(searchHandle.get());

    //server.requestSourceRangesAndDiagnosticsForQueryMessage(createMessage(queryText));
}

Core::FindFlags ClangQueryProjectsFindFilter::supportedFindFlags() const
{
    return 0;
}

bool ClangQueryProjectsFindFilter::isUsable() const
{
    return server.isUsable();
}

void ClangQueryProjectsFindFilter::setUsable(bool isUsable)
{
    server.setUsable(isUsable);
}

} // namespace ClangRefactoring
