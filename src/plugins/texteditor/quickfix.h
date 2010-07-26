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

class TEXTEDITOR_EXPORT QuickFixState
{
public:
    QuickFixState(TextEditor::BaseTextEditor *editor);
    virtual ~QuickFixState();

    TextEditor::BaseTextEditor *editor() const;

    QTextCursor textCursor() const;
    void setCursor(const QTextCursor &cursor);

    int selectionStart() const;
    int selectionEnd() const;

    /**
     * Calculates the offset in the document for the given line and column.
     *
     * \param line The line number, 1-based.
     * \param column The column number, 1-based.
     * \return The offset in the \c QTextDocument of the editor.
     */
    int position(int line, int column) const;

    QChar charAt(int offset) const;
    QString textOf(int start, int end) const;

    static TextEditor::RefactoringChanges::Range range(int start, int end);

private:
    TextEditor::BaseTextEditor *_editor;
    QTextCursor _textCursor;
};

class TEXTEDITOR_EXPORT QuickFixOperation
{
    Q_DISABLE_COPY(QuickFixOperation)

public:
    typedef QSharedPointer<QuickFixOperation> Ptr;

public:
    QuickFixOperation(int priority = -1);
    virtual ~QuickFixOperation();

    virtual int priority() const;
    void setPriority(int priority);
    virtual QString description() const;
    void setDescription(const QString &description);

    virtual void perform();

    virtual void createChanges() = 0;

protected:
    virtual TextEditor::RefactoringChanges *refactoringChanges() const = 0;

private:
    TextEditor::BaseTextEditor *_editor;
    int _priority;
    QString _description;
};

class TEXTEDITOR_EXPORT QuickFixFactory: public QObject
{
    Q_OBJECT

public:
    QuickFixFactory(QObject *parent = 0);
    virtual ~QuickFixFactory() = 0;

    virtual QList<QuickFixOperation::Ptr> matchingOperations(QuickFixState *state) = 0;
};

class TEXTEDITOR_EXPORT QuickFixCollector: public TextEditor::IQuickFixCollector
{
    Q_OBJECT

public:
    QuickFixCollector();
    virtual ~QuickFixCollector();

    QList<TextEditor::QuickFixOperation::Ptr> quickFixes() const { return _quickFixes; }

    virtual TextEditor::ITextEditable *editor() const;
    virtual int startPosition() const;
    virtual bool triggersCompletion(TextEditor::ITextEditable *editor);
    virtual int startCompletion(TextEditor::ITextEditable *editor);
    virtual void completions(QList<TextEditor::CompletionItem> *completions);
    virtual void fix(const TextEditor::CompletionItem &item);
    virtual void cleanup();

    virtual TextEditor::QuickFixState *initializeCompletion(BaseTextEditor *editable) = 0;

    virtual QList<QuickFixFactory *> quickFixFactories() const = 0;

private:
    TextEditor::ITextEditable *_editable;
    QList<QuickFixOperation::Ptr> _quickFixes;
};

} // end of namespace TextEditor

#endif // TEXTEDITORQUICKFIX_H
