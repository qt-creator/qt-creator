/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef REFACTORINGCHANGES_H
#define REFACTORINGCHANGES_H

#include <utils/changeset.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/texteditor_global.h>

#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>

namespace TextEditor {

class TEXTEDITOR_EXPORT RefactoringChanges
{
public:
    typedef Utils::ChangeSet::Range Range;

public:
    RefactoringChanges();
    virtual ~RefactoringChanges();

    void createFile(const QString &fileName, const QString &contents);
    void changeFile(const QString &fileName, const Utils::ChangeSet &changeSet);
// TODO:
//    void deleteFile(const QString &fileName);

    void reindent(const QString &fileName, const Range &range);

    virtual QStringList apply();

    Q_DECL_DEPRECATED int positionInFile(const QString &fileName, int line, int column = 0) const;

    static BaseTextEditor *editorForFile(const QString &fileName,
                                         bool openIfClosed = false);
    static BaseTextEditor *editorForNewFile(const QString &fileName);

    /** line and column are zero-based */
    void openEditor(const QString &fileName, int line, int column);

private:
    QMap<QString, QString> m_contentsByCreatedFile;
    QMap<QString, Utils::ChangeSet> m_changesByFile;
    QMap<QString, QList<Range> > m_indentRangesByFile;
    QString m_fileNameToShow;
    int m_lineToShow;
    int m_columnToShow;
};

} // namespace TextEditor

#endif // REFACTORINGCHANGES_H
