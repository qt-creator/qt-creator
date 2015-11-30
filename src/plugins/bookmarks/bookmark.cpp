/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "bookmark.h"
#include "bookmarkmanager.h"
#include "bookmarks_global.h"

#include <QDebug>
#include <QFileInfo>
#include <QTextBlock>

using namespace Bookmarks::Internal;

Bookmark::Bookmark(int lineNumber, BookmarkManager *manager) :
    TextMark(QString(), lineNumber, Constants::BOOKMARKS_TEXT_MARK_CATEGORY),
    m_manager(manager)
{
    setPriority(TextEditor::TextMark::NormalPriority);
    setIcon(m_manager->bookmarkIcon());
}

void Bookmark::removedFromEditor()
{
    m_manager->deleteBookmark(this);
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
    m_note = note;
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
    return m_note;
}
