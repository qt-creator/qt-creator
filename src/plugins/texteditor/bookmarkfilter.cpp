// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bookmarkfilter.h"

#include "bookmark.h"
#include "bookmarkmanager.h"
#include "texteditortr.h"

#include <coreplugin/locator/ilocatorfilter.h>

#include <utils/algorithm.h>

using namespace Core;
using namespace Tasking;
using namespace Utils;

namespace TextEditor::Internal {

class BookmarkFilter final : public ILocatorFilter
{
public:
    BookmarkFilter()
    {
        setId("Bookmarks");
        setDisplayName(Tr::tr("Bookmarks"));
        setDescription(Tr::tr("Locates bookmarks. Filter by file name, by the text on the line of the "
                              "bookmark, or by the bookmark's note text."));
        setPriority(Medium);
        setDefaultShortcutString("b");
    }

private:
    LocatorMatcherTasks matchers() final;
    LocatorFilterEntries match(const QString &input) const;
};

LocatorMatcherTasks BookmarkFilter::matchers()
{
    const auto onSetup = [this] {
        const LocatorStorage &storage = *LocatorStorage::storage();
        storage.reportOutput(match(storage.input()));
    };
    return {Sync(onSetup)};
}

LocatorFilterEntries BookmarkFilter::match(const QString &input) const
{
    if (bookmarkManager().rowCount() == 0)
        return {};

    const auto match = [](const QString &name, BookmarkManager::Roles role) {
        const BookmarkManager &manager = bookmarkManager();
        return manager.match(manager.index(0, 0), role, name, -1, Qt::MatchContains | Qt::MatchWrap);
    };

    const int colonIndex = input.lastIndexOf(':');
    QModelIndexList fileNameLineNumberMatches;
    if (colonIndex >= 0) {
        // Filter by "fileName:lineNumber" pattern
        const QString fileName = input.left(colonIndex);
        const QString lineNumber = input.mid(colonIndex + 1);
        fileNameLineNumberMatches = match(fileName, BookmarkManager::Filename);
        fileNameLineNumberMatches = Utils::filtered(
            fileNameLineNumberMatches, [lineNumber](const QModelIndex &index) {
                return index.data(BookmarkManager::LineNumber).toString().contains(lineNumber);
            });
    }
    const QModelIndexList matches = filteredUnique(fileNameLineNumberMatches
                                                   + match(input, BookmarkManager::Filename)
                                                   + match(input, BookmarkManager::LineNumber)
                                                   + match(input, BookmarkManager::Note)
                                                   + match(input, BookmarkManager::LineText));
    LocatorFilterEntries entries;
    BookmarkManager &manager = bookmarkManager();
    for (const QModelIndex &idx : matches) {
        const Bookmark *bookmark = manager.bookmarkForIndex(idx);
        const QString filename = bookmark->filePath().fileName();
        LocatorFilterEntry entry;
        entry.displayName = QString("%1:%2").arg(filename).arg(bookmark->lineNumber());
        // TODO: according to QModelIndex docs, we shouldn't store model indexes:
        // Model indexes should be used immediately and then discarded.
        // You should not rely on indexes to remain valid after calling model functions
        // that change the structure of the model or delete items.
        entry.acceptor = [idx] {
            const BookmarkManager &manager = bookmarkManager();
            if (Bookmark *bookmark = manager.bookmarkForIndex(idx))
                manager.gotoBookmark(bookmark);
            return AcceptResult();
        };
        if (!bookmark->note().isEmpty())
            entry.extraInfo = bookmark->note();
        else if (!bookmark->lineText().isEmpty())
            entry.extraInfo = bookmark->lineText();
        else
            entry.extraInfo = bookmark->filePath().toString();
        int highlightIndex = entry.displayName.indexOf(input, 0, Qt::CaseInsensitive);
        if (highlightIndex >= 0) {
            entry.highlightInfo = {highlightIndex, int(input.length())};
        } else  {
            highlightIndex = entry.extraInfo.indexOf(input, 0, Qt::CaseInsensitive);
            if (highlightIndex >= 0) {
                entry.highlightInfo = {highlightIndex, int(input.length()),
                                       LocatorFilterEntry::HighlightInfo::ExtraInfo};
            } else if (colonIndex >= 0) {
                const QString fileName = input.left(colonIndex);
                const QString lineNumber = input.mid(colonIndex + 1);
                highlightIndex = entry.displayName.indexOf(fileName, 0,
                                                           Qt::CaseInsensitive);
                if (highlightIndex >= 0) {
                    entry.highlightInfo = {highlightIndex, int(fileName.length())};
                    highlightIndex = entry.displayName.indexOf(
                        lineNumber, highlightIndex, Qt::CaseInsensitive);
                    if (highlightIndex >= 0) {
                        entry.highlightInfo.startsDisplay += highlightIndex;
                        entry.highlightInfo.lengthsDisplay += lineNumber.length();
                    }
                }
            }
        }
        entry.displayIcon = bookmark->icon();
        entries.append(entry);
    }
    return entries;
}

void setupBookmarkFilter()
{
    static BookmarkFilter theBookmarkFilter;
}

} // TextEditor::Internal
