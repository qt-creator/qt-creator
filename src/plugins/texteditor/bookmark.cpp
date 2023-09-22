// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bookmark.h"

#include "bookmarkmanager.h"
#include "texteditor_global.h"
#include "texteditortr.h"

#include <utils/utilsicons.h>

#include <QTextBlock>

using namespace Utils;

namespace TextEditor::Internal {

const char BOOKMARKS_TEXT_MARK_CATEGORY[] = "Bookmarks.TextMarkCategory";

Bookmark::Bookmark(int lineNumber, BookmarkManager *manager) :
    TextMark(FilePath(), lineNumber, {Tr::tr("Bookmark"), BOOKMARKS_TEXT_MARK_CATEGORY}),
    m_manager(manager)
{
    setColor(Theme::Bookmarks_TextMarkColor);
    setIcon(Icons::BOOKMARK_TEXTEDITOR.icon());
    setDefaultToolTip(Tr::tr("Bookmark"));
    setPriority(TextMark::NormalPriority);
}

void Bookmark::removedFromEditor()
{
    m_manager->deleteBookmark(this);
}

bool Bookmark::isDraggable() const
{
    return true;
}

void Bookmark::dragToLine(int lineNumber)
{
    move(lineNumber);
}

void Bookmark::updateLineNumber(int line)
{
    if (line != lineNumber()) {
        TextMark::updateLineNumber(line);
        m_manager->updateBookmark(this);
    }
}

void Bookmark::move(int line)
{
    if (line != lineNumber()) {
        TextMark::move(line);
        m_manager->updateBookmark(this);
        updateMarker();
    }
}

void Bookmark::updateBlock(const QTextBlock &block)
{
    const QString &lineText = block.text().trimmed();
    if (m_lineText != lineText) {
        m_lineText = lineText;
        m_manager->updateBookmark(this);
    }
}

void Bookmark::updateFilePath(const FilePath &filePath)
{
    const FilePath oldFilePath = this->filePath();
    TextMark::updateFilePath(filePath);
    m_manager->updateBookmarkFileName(this, oldFilePath);
}

void Bookmark::setNote(const QString &note)
{
    setToolTip(note);
    setLineAnnotation(note);
    updateMarker();
}

void Bookmark::updateNote(const QString &note)
{
    setNote(note);
    m_manager->updateBookmark(this);
}

QString Bookmark::lineText() const
{
    return m_lineText;
}

QString Bookmark::note() const
{
    return toolTip();
}

} // Bookmarks::Internal
