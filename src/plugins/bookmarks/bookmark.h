/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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
    Bookmark(const QString &fileName, int lineNumber, BookmarkManager *manager);

    QIcon icon() const;

    void updateLineNumber(int lineNumber);
    void updateBlock(const QTextBlock &block);
    void removedFromEditor();
    TextEditor::ITextMark::Priority priority() const { return TextEditor::ITextMark::LowPriority; }

    QString filePath() const;
    QString fileName() const;
    QString path() const;
    QString lineText() const;

    inline int lineNumber() const { return m_lineNumber; }

private:
    BookmarkManager *m_manager;
    const QFileInfo m_fileInfo;
    const QString m_fileName;
    const QString m_onlyFile;
    const QString m_path;
    QString m_lineText;
    int m_lineNumber;
};

} // namespace Internal
} // namespace Bookmarks

#endif // BOOKMARK_H
