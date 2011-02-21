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

#ifndef TEXTEDITORQUICKFIX_H
#define TEXTEDITORQUICKFIX_H

#include "texteditor_global.h"
#include "icompletioncollector.h"

#include <QtCore/QSharedPointer>

namespace TextEditor {

class BaseTextEditorWidget;

/*!
    State of the editor on which the QuickFixFactory and the QuickFixOperation work.

    This class contains a reference
 */
class TEXTEDITOR_EXPORT QuickFixState
{
public:
    /// Creates a new state object for the given text editor.
    QuickFixState(TextEditor::BaseTextEditorWidget *editor);
    virtual ~QuickFixState();

    TextEditor::BaseTextEditorWidget *editor() const;

private:
    TextEditor::BaseTextEditorWidget *_editor;
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

        Subclasses should implement this method to do the actual changes.
     */
    virtual void perform() = 0;

private:
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
    { return m_quickFixes; }

    virtual TextEditor::ITextEditor *editor() const;
    virtual int startPosition() const;
    virtual bool triggersCompletion(TextEditor::ITextEditor *editor);
    virtual int startCompletion(TextEditor::ITextEditor *editor);
    virtual void completions(QList<TextEditor::CompletionItem> *completions);

    virtual bool supportsPolicy(TextEditor::CompletionPolicy policy) const
    { return policy == TextEditor::QuickFixCompletion; }

    /// See IQuickFixCollector::fix
    virtual void fix(const TextEditor::CompletionItem &item);

    /// See ICompletionCollector::cleanup .
    virtual void cleanup();

    /// Called from #startCompletion to create a QuickFixState .
    virtual TextEditor::QuickFixState *initializeCompletion(BaseTextEditorWidget *editable) = 0;

    virtual QList<QuickFixFactory *> quickFixFactories() const = 0;

private:
    TextEditor::ITextEditor *m_editor;
    QList<QuickFixOperation::Ptr> m_quickFixes;
};

} // namespace TextEditor

#endif // TEXTEDITORQUICKFIX_H
