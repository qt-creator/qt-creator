/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QMLJSQUICKFIX_H
#define QMLJSQUICKFIX_H

#include "qmljseditor.h"

#include <texteditor/quickfix.h>
#include <qmljs/parser/qmljsastfwd_p.h>
#include <qmljs/qmljsdocument.h>
#include <qmljstools/qmljsrefactoringchanges.h>

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

    SemanticInfo semanticInfo() const;

    /// \returns the snapshot holding the document of the editor.
    QmlJS::Snapshot snapshot() const;

    /// \returns the document of the editor
    QmlJS::Document::Ptr document() const;

    const QmlJSTools::QmlJSRefactoringFile currentFile() const;

private:
    SemanticInfo _semanticInfo;
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
    explicit QmlJSQuickFixOperation(const QmlJSQuickFixState &state, int priority = -1);
    virtual ~QmlJSQuickFixOperation();

    virtual void perform();

protected:
    typedef Utils::ChangeSet::Range Range;

    virtual void performChanges(QmlJSTools::QmlJSRefactoringFile *currentFile,
                                QmlJSTools::QmlJSRefactoringChanges *refactoring) = 0;

    /// \returns A const-reference to the state of the operation.
    const QmlJSQuickFixState &state() const;

    /// \returns The name of the file for for which this operation is invoked.
    QString fileName() const;

private:
    QmlJSQuickFixState _state;
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

    static QList<QmlJSQuickFixOperation::Ptr> noResult();
    static QList<QmlJSQuickFixOperation::Ptr> singleResult(QmlJSQuickFixOperation *operation);
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
