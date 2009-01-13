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

#ifndef BOOKMARK_H
#define BOOKMARK_H

#include <texteditor/itexteditor.h>
#include <texteditor/basetextmark.h>

QT_BEGIN_NAMESPACE
class QTreeWidgetItem;
QT_END_NAMESPACE

#include <QtCore/QFileInfo>

namespace Bookmarks {
namespace Internal {

class BookmarkManager;

class Bookmark : public TextEditor::BaseTextMark
{
    Q_OBJECT
public:
    Bookmark(const QString& fileName, int lineNumber, BookmarkManager *manager);

    QIcon icon() const;

    void updateLineNumber(int lineNumber);
    void updateBlock(const QTextBlock &block);
    void removedFromEditor();

    QString filePath() const;
    QString fileName() const;
    QString path() const;
    QString lineText() const;

    inline int lineNumber() const { return m_lineNumber; }

private:
    static const QIcon m_bookmarkIcon;

    BookmarkManager *m_manager;
    int m_lineNumber;
    QString m_name;
    QString m_fileName;
    QString m_onlyFile;
    QString m_path;
    QString m_lineText;
    QFileInfo m_fileInfo;
};

} // namespace Internal
} // namespace Bookmarks

#endif // BOOKMARK_H
