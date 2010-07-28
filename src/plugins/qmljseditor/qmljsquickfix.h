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

#ifndef QMLJSQUICKFIX_H
#define QMLJSQUICKFIX_H

#include "qmljseditor.h"
#include "qmljsrefactoringchanges.h"

#include <texteditor/quickfix.h>
#include <qmljs/parser/qmljsastfwd_p.h>
#include <qmljs/qmljsdocument.h>

namespace ExtensionSystem {
class IPlugin;
}

namespace QmlJS {
    class ModelManagerInterface;
}

namespace QmlJSEditor {

namespace Internal {
class QmlJSQuickFixCollector;
} // end of namespace Internal

/*!
    Specialized QuickFixState for QML/JavaScript quick-fixes.

    This specialized state for QML/JavaScript quick-fixes also holds the
    QmlJSEditor::Internal::SemanticInfo for the document in the editor.
 */
class QmlJSQuickFixState: public TextEditor::QuickFixState
{
    friend class Internal::QmlJSQuickFixCollector;

public:
    /// Creates a new state for the given editor.
    QmlJSQuickFixState(TextEditor::BaseTextEditor *editor);
    typedef Utils::ChangeSet::Range Range;

    Internal::SemanticInfo semanticInfo() const;

    /// \returns the snapshot holding the document of the editor.
    QmlJS::Snapshot snapshot() const;

    /// \returns the document of the editor
    QmlJS::Document::Ptr document() const;

    /*!
        \returns the offset in the document for the start position of the given
                 source location.
     */
    unsigned startPosition(const QmlJS::AST::SourceLocation &loc) const;

private:
    Internal::SemanticInfo _semanticInfo;
};

/*!
    A quick-fix operation for the QML/JavaScript editor, which works on a
    QmlJSQuickFixState .
 */
class QmlJSQuickFixOperation: public TextEditor::QuickFixOperation
{
    Q_DISABLE_COPY(QmlJSQuickFixOperation)

public:
    /*!
        Creates a new QmlJSQuickFixOperation.

        This operation will copy the complete state, in order to be able to perform
        its changes later on.

        \param state The state for which this operation was created.
        \param priority The priority for this operation.
     */
    QmlJSQuickFixOperation(const QmlJSQuickFixState &state, int priority = -1);
    virtual ~QmlJSQuickFixOperation();

protected:
    /// \returns A const-reference to the state of the operation.
    const QmlJSQuickFixState &state() const;

    /// \returns The name of the file for for which this operation is invoked.
    QString fileName() const;

    /// \returns The refactoring changes associated with this quick-fix operation.
    QmlJSRefactoringChanges *refactoringChanges() const;

protected: // Utility functions forwarding to QmlJSQuickFixState
    /// \see QmlJSQuickFixState#startPosition
    unsigned startPosition(const QmlJS::AST::SourceLocation &loc) const
    { return state().startPosition(loc); }

    /// \see QmlJSQuickFixState#range
    static QmlJSQuickFixState::Range range(int start, int end)
    { return QmlJSQuickFixState::range(start, end); }

private:
    QmlJSQuickFixState _state;
    QScopedPointer<QmlJSRefactoringChanges> _refactoringChanges;
};

class QmlJSQuickFixFactory: public TextEditor::QuickFixFactory
{
    Q_OBJECT

public:
    QmlJSQuickFixFactory();
    virtual ~QmlJSQuickFixFactory();

    virtual QList<TextEditor::QuickFixOperation::Ptr> matchingOperations(TextEditor::QuickFixState *state);

    /*!
        Implement this method to match and create the appropriate
        QmlJSQuickFixOperation objects.
     */
    virtual QList<QmlJSQuickFixOperation::Ptr> match(const QmlJSQuickFixState &state) = 0;
};

namespace Internal {

class QmlJSQuickFixCollector: public TextEditor::QuickFixCollector
{
    Q_OBJECT

public:
    QmlJSQuickFixCollector();
    virtual ~QmlJSQuickFixCollector();

    virtual bool supportsEditor(TextEditor::ITextEditable *editor);
    virtual TextEditor::QuickFixState *initializeCompletion(TextEditor::BaseTextEditor *editor);

    virtual QList<TextEditor::QuickFixFactory *> quickFixFactories() const;

    /// Registers all quick-fixes in this plug-in as auto-released objects.
    static void registerQuickFixes(ExtensionSystem::IPlugin *plugIn);
};

} // end of namespace Internal
} // end of namespace QmlJSEditor

#endif // QMLJSQUICKFIX_H
