/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "bookmark.h"
#include "bookmarkmanager.h"

#include <QDebug>
#include <QTextBlock>

using namespace Bookmarks::Internal;

Bookmark::Bookmark(const QString& fileName, int lineNumber, BookmarkManager *manager) :
    BaseTextMark(fileName, lineNumber),
    m_manager(manager),
    m_fileName(fileName)
{
    QFileInfo fi(fileName);
    m_onlyFile = fi.fileName();
    m_path = fi.path();
    setPriority(TextEditor::ITextMark::NormalPriority);
    setIcon(m_manager->bookmarkIcon());
}

void Bookmark::removedFromEditor()
{
    m_manager->removeBookmark(this);
}

void Bookmark::updateLineNumber(int line)
{
    if (line != lineNumber())
        m_manager->updateBookmark(this);
    BaseTextMark::updateLineNumber(line);
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
    m_fileName = fileName;
    QFileInfo fi(fileName);
    m_onlyFile = fi.fileName();
    m_path = fi.path();
    m_manager->updateBookmark(this);
    BaseTextMark::updateFileName(fileName);
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

QString Bookmark::filePath() const
{
    return m_fileName;
}

QString Bookmark::fileName() const
{
    return m_onlyFile;
}

QString Bookmark::path() const
{
    return m_path;
}
