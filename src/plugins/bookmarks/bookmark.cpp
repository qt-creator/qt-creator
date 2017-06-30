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

#include "bookmark.h"
#include "bookmarkmanager.h"
#include "bookmarks_global.h"

#include <utils/utilsicons.h>

#include <QApplication>
#include <QFileInfo>
#include <QTextBlock>

using namespace Bookmarks::Internal;

Bookmark::Bookmark(int lineNumber, BookmarkManager *manager) :
    TextMark(QString(), lineNumber, Constants::BOOKMARKS_TEXT_MARK_CATEGORY),
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
    if (m_lineText != block.text()) {
        m_lineText = block.text();
        m_manager->updateBookmark(this);
    }
}

void Bookmark::updateFileName(const QString &fileName)
{
    const QString &oldFileName = this->fileName();
    TextMark::updateFileName(fileName);
    m_manager->updateBookmarkFileName(this, oldFileName);
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
