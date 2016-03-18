/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <utils/changeset.h>
#include <utils/textfileformat.h>
#include <texteditor/texteditor_global.h>

#include <QList>
#include <QString>
#include <QSharedPointer>
#include <QPair>

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

namespace TextEditor {
class TextDocument;
class TextEditorWidget;
class RefactoringChanges;
class RefactoringFile;
class RefactoringChangesData;
typedef QSharedPointer<RefactoringFile> RefactoringFilePtr;
typedef QVector<QPair<QTextCursor, QTextCursor> > RefactoringSelections;

// ### listen to the m_editor::destroyed signal?
class TEXTEDITOR_EXPORT RefactoringFile
{
    Q_DISABLE_COPY(RefactoringFile)
public:
    typedef Utils::ChangeSet::Range Range;

public:
    virtual ~RefactoringFile();

    bool isValid() const;

    const QTextDocument *document() const;
    // mustn't use the cursor to change the document
    const QTextCursor cursor() const;
    QString fileName() const;
    TextEditorWidget *editor() const;

    // converts 1-based line and column into 0-based source offset
    int position(unsigned line, unsigned column) const;
    // converts 0-based source offset into 1-based line and column
    void lineAndColumn(int offset, unsigned *line, unsigned *column) const;

    QChar charAt(int pos) const;
    QString textOf(int start, int end) const;
    QString textOf(const Range &range) const;

    void setChangeSet(const Utils::ChangeSet &changeSet);
    void appendIndentRange(const Range &range);
    void appendReindentRange(const Range &range);
    void setOpenEditor(bool activate = false, int pos = -1);
    void apply();

protected:
    // users may only get const access to RefactoringFiles created through
    // this constructor, because it can't be used to apply changes
    RefactoringFile(QTextDocument *document, const QString &fileName);

    RefactoringFile(TextEditorWidget *editor);
    RefactoringFile(const QString &fileName, const QSharedPointer<RefactoringChangesData> &data);

    QTextDocument *mutableDocument() const;
    // derived classes may want to clear language specific extra data
    virtual void fileChanged();

    void indentOrReindent(void (RefactoringChangesData::*mf)(const QTextCursor &,
                                                             const QString &,
                                                             const TextDocument *) const,
                          const RefactoringSelections &ranges);

protected:
    QString m_fileName;
    QSharedPointer<RefactoringChangesData> m_data;
    mutable Utils::TextFileFormat m_textFileFormat;
    mutable QTextDocument *m_document;
    TextEditorWidget *m_editor;
    Utils::ChangeSet m_changes;
    QList<Range> m_indentRanges;
    QList<Range> m_reindentRanges;
    bool m_openEditor;
    bool m_activateEditor;
    int m_editorCursorPosition;
    bool m_appliedOnce;

    friend class RefactoringChanges; // access to constructor
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

    static RefactoringFilePtr file(TextEditorWidget *editor);
    RefactoringFilePtr file(const QString &fileName) const;
    bool createFile(const QString &fileName, const QString &contents, bool reindent = true, bool openEditor = true) const;
    bool removeFile(const QString &fileName) const;

protected:
    explicit RefactoringChanges(RefactoringChangesData *data);

    static TextEditorWidget *openEditor(const QString &fileName, bool activate, int line, int column);
    static RefactoringSelections rangesToSelections(QTextDocument *document, const QList<Range> &ranges);

protected:
    QSharedPointer<RefactoringChangesData> m_data;

    friend class RefactoringFile;
};

class TEXTEDITOR_EXPORT RefactoringChangesData
{
    Q_DISABLE_COPY(RefactoringChangesData)

public:
    RefactoringChangesData() {}
    virtual ~RefactoringChangesData();

    virtual void indentSelection(const QTextCursor &selection,
                                 const QString &fileName,
                                 const TextDocument *textEditor) const;
    virtual void reindentSelection(const QTextCursor &selection,
                                   const QString &fileName,
                                   const TextDocument *textEditor) const;
    virtual void fileChanged(const QString &fileName);
};

} // namespace TextEditor
