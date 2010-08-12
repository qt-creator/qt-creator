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

class RefactoringChanges;

class TEXTEDITOR_EXPORT RefactoringFile
{
public:
    typedef Utils::ChangeSet::Range Range;

public:
    RefactoringFile();
    RefactoringFile(const QString &fileName, RefactoringChanges *refactoringChanges);
    RefactoringFile(const RefactoringFile &other);
    ~RefactoringFile();

    bool isValid() const;

    const QTextDocument *document() const;
    const QTextCursor cursor() const;

    // converts 1-based line and column into 0-based offset
    int position(unsigned line, unsigned column) const;

    QChar charAt(int pos) const;
    QString textAt(int start, int end) const;
    QString textAt(const Range &range) const;

    bool change(const Utils::ChangeSet &changeSet, bool openEditor = true);
    bool indent(const Range &range, bool openEditor = true);

private:
    // not assignable
    //const RefactoringFile &operator=(const RefactoringFile &other);

    QTextDocument *mutableDocument() const;

private:
    QString m_fileName;
    RefactoringChanges *m_refactoringChanges;
    mutable QTextDocument *m_document;
    BaseTextEditor *m_editor;
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

    BaseTextEditor *openEditor(const QString &fileName, int pos = -1);

    /**
     * \param fileName the file to activate the editor for
     * \param pos, 0-based offset to put the cursor on, -1 means don't move
     */
    BaseTextEditor *activateEditor(const QString &fileName, int pos = -1);


private:
    static BaseTextEditor *editorForFile(const QString &fileName,
                                         bool openIfClosed = false);

    static QList<QTextCursor> rangesToSelections(QTextDocument *document, const QList<Range> &ranges);
    virtual void indentSelection(const QTextCursor &selection) const = 0;
    virtual void fileChanged(const QString &fileName) = 0;

    friend class RefactoringFile;
};

} // namespace TextEditor

#endif // REFACTORINGCHANGES_H
