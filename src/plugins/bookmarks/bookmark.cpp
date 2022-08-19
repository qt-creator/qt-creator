// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "bookmark.h"
#include "bookmarkmanager.h"
#include "bookmarks_global.h"

#include <utils/utilsicons.h>

#include <QApplication>
#include <QFileInfo>
#include <QTextBlock>

using namespace Bookmarks::Internal;
using namespace Utils;

Bookmark::Bookmark(int lineNumber, BookmarkManager *manager) :
    TextMark(FilePath(), lineNumber, Constants::BOOKMARKS_TEXT_MARK_CATEGORY),
    m_manager(manager)
{
    setColor(Utils::Theme::Bookmarks_TextMarkColor);
    setIcon(Utils::Icons::BOOKMARK_TEXTEDITOR.icon());
    setDefaultToolTip(QApplication::translate("BookmarkManager", "Bookmark"));
    setPriority(TextEditor::TextMark::NormalPriority);
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

void Bookmark::updateFileName(const FilePath &fileName)
{
    const FilePath &oldFileName = this->fileName();
    TextMark::updateFileName(fileName);
    m_manager->updateBookmarkFileName(this, oldFileName.toString());
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
