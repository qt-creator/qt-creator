/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef REFACTORINGCHANGES_H
#define REFACTORINGCHANGES_H

#include <utils/changeset.h>
#include <texteditor/texteditor_global.h>

#include <QtCore/QList>
#include <QtCore/QString>

QT_FORWARD_DECLARE_CLASS(QTextDocument)

namespace TextEditor {
class BaseTextEditorWidget;
class RefactoringChanges;

class TEXTEDITOR_EXPORT RefactoringFile
{
public:
    typedef Utils::ChangeSet::Range Range;

public:
    RefactoringFile();
    RefactoringFile(const QString &fileName, RefactoringChanges *refactoringChanges);
    RefactoringFile(const RefactoringFile &other);
    virtual ~RefactoringFile();

    bool isValid() const;

    const QTextDocument *document() const;
    const QTextCursor cursor() const;
    QString fileName() const;

    // converts 1-based line and column into 0-based offset
    int position(unsigned line, unsigned column) const;

    QChar charAt(int pos) const;
    QString textOf(int start, int end) const;
    QString textOf(const Range &range) const;

    bool change(const Utils::ChangeSet &changeSet, bool openEditor = true);
    bool indent(const Range &range, bool openEditor = true);

protected:
    // not assignable
    //const RefactoringFile &operator=(const RefactoringFile &other);

    QTextDocument *mutableDocument() const;

protected:
    QString m_fileName;
    RefactoringChanges *m_refactoringChanges;
    mutable QTextDocument *m_document;
    BaseTextEditorWidget *m_editor;
    Utils::ChangeSet m_changes;
    QList<Range> m_indentRanges;
    bool m_openEditor;
};

 /*!
    This class batches changes to multiple file, which are applied as a single big
    change.
 */
class TEXTEDITOR_EXPORT RefactoringChanges
{
public:
    typedef Utils::ChangeSet::Range Range;

public:
    RefactoringChanges();
    virtual ~RefactoringChanges();

    bool createFile(const QString &fileName, const QString &contents, bool reindent = true, bool openEditor = true);
    bool removeFile(const QString &fileName);

    RefactoringFile file(const QString &fileName);

    BaseTextEditorWidget *openEditor(const QString &fileName, int pos = -1);

    /*!
       \param fileName the file to activate the editor for
       \param line the line to put the cursor on (1-based)
       \param column the column to put the cursor on (1-based)
     */
    void activateEditor(const QString &fileName, int line, int column);


private:
    static BaseTextEditorWidget *editorForFile(const QString &fileName,
                                         bool openIfClosed = false);

    static QList<QTextCursor> rangesToSelections(QTextDocument *document, const QList<Range> &ranges);
    virtual void indentSelection(const QTextCursor &selection,
                                 const QString &fileName,
                                 const BaseTextEditorWidget *textEditor) const = 0;
    virtual void fileChanged(const QString &fileName) = 0;

    friend class RefactoringFile;

private:
    QString m_fileToOpen;
    int m_lineToOpen;
    int m_columnToOpen;
};

} // namespace TextEditor

#endif // REFACTORINGCHANGES_H
