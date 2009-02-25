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
