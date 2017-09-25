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

#include "qtcreatorsearch.h"

#include "qtcreatorsearchhandle.h"

#include <coreplugin/editormanager/editormanager.h>

#include <QDir>

namespace ClangRefactoring {

QtCreatorSearch::QtCreatorSearch(Core::SearchResultWindow &searchResultWindow)
    : searchResultWindow(searchResultWindow)
{
}

std::unique_ptr<SearchHandle> QtCreatorSearch::startNewSearch(const QString &searchLabel,
                                                                       const QString &searchTerm)
{
    Core::SearchResult *searchResult = searchResultWindow.startNewSearch(
                searchLabel,
                {},
                searchTerm,
                Core::SearchResultWindow::SearchOnly,
                Core::SearchResultWindow::PreserveCaseEnabled);

    QObject::connect(searchResult,
                     &Core::SearchResult::activated,
                     [](const Core::SearchResultItem& item) {
                         Core::EditorManager::openEditorAtSearchResult(item);
                     });

    auto searchHandle = std::unique_ptr<SearchHandle>(new QtCreatorSearchHandle(searchResult));

    QObject::connect(searchResult,
                     &Core::SearchResult::cancelled,
                     [handle=searchHandle.get()] () { handle->cancel(); });

    return searchHandle;
}

} // namespace ClangRefactoring
