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

#ifndef TEXTEDITORQUICKFIX_H
#define TEXTEDITORQUICKFIX_H

#include "texteditor_global.h"
#include "icompletioncollector.h"

#include <texteditor/refactoringchanges.h>
#include <utils/changeset.h>

#include <QtCore/QSharedPointer>
#include <QtGui/QTextCursor>

namespace TextEditor {

class BaseTextEditor;

/*!
    State of the editor on which the QuickFixFactory and the QuickFixOperation work.

    This class contains a reference
 */
class TEXTEDITOR_EXPORT QuickFixState
{
public:
    /// Creates a new state object for the given text editor.
    QuickFixState(TextEditor::BaseTextEditor *editor);
    virtual ~QuickFixState();

    TextEditor::BaseTextEditor *editor() const;

    /*!
        \returns A QTextCursor positioned as the editor's visible cursor, including
                 possible selections.
     */
    QTextCursor textCursor() const;

    /// \returns The character offset in the document where the selection starts.
    int selectionStart() const;

    /// \returns The character offset in the document where the selection ends.
    int selectionEnd() const;

    /*!
       Calculates the offset in the document for the given line and column.

       \param line The line number, 1-based.
       \param column The column number, 1-based.
       \return The offset in the \c QTextDocument of the editor.
     */
    int position(int line, int column) const;

    /// \returns The character at the given offset in the editor's text document.
    QChar charAt(int offset) const;

    /*!
        \returns The text between the given start- and end-offset in the editor's
                 text document.
     */
    QString textOf(int start, int end) const;

    /// Utility method to create a range.
    static TextEditor::RefactoringChanges::Range range(int start, int end);

private:
    TextEditor::BaseTextEditor *_editor;
    QTextCursor _textCursor;
};

/*!
    Class to perform a single quick-fix.

    Quick-fix operations cannot be copied, and must be passed around as explicitly
    shared pointers ( QuickFixOperation::Ptr ).

    Subclasses should make sure that they copy parts of, or the whole QuickFixState ,
    which are needed to perform the quick-fix operation.
 */
class TEXTEDITOR_EXPORT QuickFixOperation
{
    Q_DISABLE_COPY(QuickFixOperation)

public:
    typedef QSharedPointer<QuickFixOperation> Ptr;

public:
    QuickFixOperation(int priority = -1);
    virtual ~QuickFixOperation();

    /*!
        \returns The priority for this quick-fix. See the QuickFixCollector for more
                 information.
     */
    virtual int priority() const;

    /// Sets the priority for this quick-fix operation.
    void setPriority(int priority);

    /*!
        \returns The description for this quick-fix. This description is shown to the
                 user.
     */
    virtual QString description() const;

    /// Sets the description for this quick-fix, which will be shown to the user.
    void setDescription(const QString &description);

    /*!
        Perform this quick-fix's operation.

        This implementation will call perform and then RefactoringChanges::apply() .
     */
    virtual void perform();

    /*!
        Subclasses should implement this method to do the actual changes by using the
        RefactoringChanges.
     */
    virtual void createChanges() = 0;

protected:
    virtual TextEditor::RefactoringChanges *refactoringChanges() const = 0;

private:
    TextEditor::BaseTextEditor *_editor;
    int _priority;
    QString _description;
};

/*!
    The QuickFixFactory is responsible for generating QuickFixOperation s which are
    applicable to the given QuickFixState.

    A QuickFixFactory should not have any state -- it can be invoked multiple times
    for different QuickFixState objects to create the matching operations, before any
    of those operations are applied (or released).

    This way, a single factory can be used by multiple editors, and a single editor
    can have multiple QuickFixCollector objects for different parts of the code.
 */
class TEXTEDITOR_EXPORT QuickFixFactory: public QObject
{
    Q_OBJECT

public:
    QuickFixFactory(QObject *parent = 0);
    virtual ~QuickFixFactory() = 0;

    /*!
        \returns A list of operations which can be performed for the given state.
     */
    virtual QList<QuickFixOperation::Ptr> matchingOperations(QuickFixState *state) = 0;
};

/*!
    A completion collector which will use the QuickFixFactory classes to generate
    quickfixes for the given editor.

    All QuickFixFactory instances returned by #quickFixFactories are queried for
    possible quick-fix operations. The operations(s) with the highest priority are
    stored, and can be queried by calling #quickFixes .
 */
class TEXTEDITOR_EXPORT QuickFixCollector: public TextEditor::IQuickFixCollector
{
    Q_OBJECT

public:
    QuickFixCollector();
    virtual ~QuickFixCollector();

    QList<TextEditor::QuickFixOperation::Ptr> quickFixes() const
    { return _quickFixes; }

    virtual TextEditor::ITextEditable *editor() const;
    virtual int startPosition() const;
    virtual bool triggersCompletion(TextEditor::ITextEditable *editor);
    virtual int startCompletion(TextEditor::ITextEditable *editor);
    virtual void completions(QList<TextEditor::CompletionItem> *completions);

    /// See IQuickFixCollector::fix
    virtual void fix(const TextEditor::CompletionItem &item);

    /// See ICompletionCollector::cleanup .
    virtual void cleanup();

    /// Called from #startCompletion to create a QuickFixState .
    virtual TextEditor::QuickFixState *initializeCompletion(BaseTextEditor *editable) = 0;

    virtual QList<QuickFixFactory *> quickFixFactories() const = 0;

private:
    TextEditor::ITextEditable *_editable;
    QList<QuickFixOperation::Ptr> _quickFixes;
};

} // end of namespace TextEditor

#endif // TEXTEDITORQUICKFIX_H
