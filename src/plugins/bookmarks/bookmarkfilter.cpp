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

#include "bookmarkfilter.h"

#include "bookmark.h"
#include "bookmarkmanager.h"

#include <utils/algorithm.h>

using namespace Bookmarks::Internal;
using namespace Core;
using namespace Utils;

BookmarkFilter::BookmarkFilter(BookmarkManager *manager)
    : m_manager(manager)
{
    setId("Bookmarks");
    setDisplayName(tr("Bookmarks"));
    setPriority(Medium);
    setShortcutString("b");
}

QList<LocatorFilterEntry> BookmarkFilter::matchesFor(QFutureInterface<LocatorFilterEntry> &future,
                                                     const QString &entry)
{
    Q_UNUSED(future);
    if (m_manager->rowCount() == 0)
        return QList<LocatorFilterEntry>();

    auto match = [this](const QString &name, BookmarkManager::Roles role) {
        return m_manager->match(m_manager->index(0, 0), role, name, -1,
                                Qt::MatchContains | Qt::MatchWrap);
    };

    int colonIndex = entry.lastIndexOf(':');
    QModelIndexList fileNameLineNumberMatches;
    if (colonIndex >= 0) {
        // Filter by "fileName:lineNumber" pattern
        const QString fileName = entry.left(colonIndex);
        const QString lineNumber = entry.mid(colonIndex + 1);
        fileNameLineNumberMatches = match(fileName, BookmarkManager::Filename);
        fileNameLineNumberMatches =
                Utils::filtered(fileNameLineNumberMatches, [lineNumber](const QModelIndex &index) {
                    return index.data(BookmarkManager::LineNumber).toString().contains(lineNumber);
                });
    }

    const QModelIndexList matches = filteredUnique(fileNameLineNumberMatches
                                                   + match(entry, BookmarkManager::Filename)
                                                   + match(entry, BookmarkManager::LineNumber)
                                                   + match(entry, BookmarkManager::Note)
                                                   + match(entry, BookmarkManager::LineText));

    QList<LocatorFilterEntry> entries;
    for (const QModelIndex &idx : matches) {
        const Bookmark *bookmark = m_manager->bookmarkForIndex(idx);
        const QString filename = bookmark->fileName().fileName();
        LocatorFilterEntry filterEntry(this,
                                       QString("%1:%2").arg(filename).arg(bookmark->lineNumber()),
                                       QVariant::fromValue(idx));
        if (!bookmark->note().isEmpty())
            filterEntry.extraInfo = bookmark->note();
        else if (!bookmark->lineText().isEmpty())
            filterEntry.extraInfo = bookmark->lineText();
        else
            filterEntry.extraInfo = bookmark->fileName().toString();
        int highlightIndex = filterEntry.displayName.indexOf(entry, 0, Qt::CaseInsensitive);
        if (highlightIndex >= 0) {
            filterEntry.highlightInfo = {highlightIndex, entry.length()};
        } else  {
            highlightIndex = filterEntry.extraInfo.indexOf(entry, 0, Qt::CaseInsensitive);
            if (highlightIndex >= 0) {
                filterEntry.highlightInfo = {highlightIndex, entry.length(),
                                             LocatorFilterEntry::HighlightInfo::ExtraInfo};
            } else if (colonIndex >= 0) {
                const QString fileName = entry.left(colonIndex);
                const QString lineNumber = entry.mid(colonIndex + 1);
                highlightIndex = filterEntry.displayName.indexOf(fileName, 0, Qt::CaseInsensitive);
                if (highlightIndex >= 0) {
                    filterEntry.highlightInfo = {highlightIndex, fileName.length()};
                    highlightIndex = filterEntry.displayName.indexOf(
                        lineNumber, highlightIndex, Qt::CaseInsensitive);
                    if (highlightIndex >= 0) {
                        filterEntry.highlightInfo.starts += highlightIndex;
                        filterEntry.highlightInfo.lengths += lineNumber.length();
                    }
                }
            }
        }

        filterEntry.displayIcon = bookmark->icon();
        entries.append(filterEntry);
    }
    return entries;
}

void BookmarkFilter::accept(LocatorFilterEntry selection, QString *newText,
                            int *selectionStart, int *selectionLength) const
{
    Q_UNUSED(newText);
    Q_UNUSED(selectionStart);
    Q_UNUSED(selectionLength);
    if (const Bookmark *bookmark = m_manager->bookmarkForIndex(
                selection.internalData.toModelIndex())) {
        m_manager->gotoBookmark(bookmark);
    }
}

void BookmarkFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future);
}
