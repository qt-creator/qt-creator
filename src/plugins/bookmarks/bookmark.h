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

#ifndef BOOKMARK_H
#define BOOKMARK_H

#include <texteditor/itexteditor.h>
#include <texteditor/basetextmark.h>

QT_BEGIN_NAMESPACE
class QTreeWidgetItem;
QT_END_NAMESPACE

#include <QFileInfo>

namespace Bookmarks {
namespace Internal {

class BookmarkManager;

class Bookmark : public TextEditor::BaseTextMark
{
public:
    Bookmark(const QString &fileName, int lineNumber, BookmarkManager *manager);

    void updateLineNumber(int lineNumber);
    void updateBlock(const QTextBlock &block);
    void updateFileName(const QString &fileName);
    void setNote(const QString &note);
    void updateNote(const QString &note);
    void removedFromEditor();

    QString filePath() const;
    QString fileName() const;
    QString path() const;
    QString lineText() const;
    QString note() const;

private:
    BookmarkManager *m_manager;
    QString m_fileName;
    QString m_onlyFile;
    QString m_path;
    QString m_lineText;
    QString m_note;
};

} // namespace Internal
} // namespace Bookmarks

#endif // BOOKMARK_H
