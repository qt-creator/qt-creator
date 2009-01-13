/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

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
