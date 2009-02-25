/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "bookmark.h"
#include "bookmarkmanager.h"

#include <QtCore/QDebug>
#include <QtGui/QTextBlock>

using namespace Bookmarks::Internal;

const QIcon Bookmark::m_bookmarkIcon = QIcon(":/bookmarks/images/bookmark.png");

Bookmark::Bookmark(const QString& fileName, int lineNumber, BookmarkManager *manager)
    : BaseTextMark(fileName, lineNumber), m_manager(manager)
{
    m_fileName = fileName;
    m_fileInfo.setFile(fileName);
    m_onlyFile = m_fileInfo.fileName();
    m_path = m_fileInfo.path();
    m_lineNumber= lineNumber;
}

QIcon Bookmark::icon() const
{
    return m_bookmarkIcon;
}

void Bookmark::removedFromEditor()
{
    m_manager->removeBookmark(this);
}

void Bookmark::updateLineNumber(int lineNumber)
{
    if (lineNumber != m_lineNumber) {
        m_lineNumber = lineNumber;
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

QString Bookmark::lineText() const
{
    return m_lineText;
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
