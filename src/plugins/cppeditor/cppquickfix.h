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

#ifndef CPPQUICKFIX_H
#define CPPQUICKFIX_H

#include "cpprefactoringchanges.h"
#include "cppsemanticinfo.h"

#include <ASTfwd.h>
#include <cplusplus/CppDocument.h>
#include <texteditor/icompletioncollector.h>
#include <texteditor/quickfix.h>
#include <utils/changeset.h>

#include <QtCore/QSharedPointer>
#include <QtGui/QTextCursor>

namespace CppTools {
    class CppModelManagerInterface;
} // end of namespace CppTools

namespace ExtensionSystem {
class IPlugin;
}

namespace CppEditor {

namespace Internal {
class CppQuickFixCollector;
} // end of namespace Internal

class CPPEDITOR_EXPORT CppQuickFixState: public TextEditor::QuickFixState
{
    friend class Internal::CppQuickFixCollector;

public:
    CppQuickFixState(TextEditor::BaseTextEditor *editor);

    const QList<CPlusPlus::AST *> &path() const;
    CPlusPlus::Snapshot snapshot() const;
    CPlusPlus::Document::Ptr document() const;
    CppEditor::Internal::SemanticInfo semanticInfo() const;
    const CPlusPlus::LookupContext &context() const;

    const CppRefactoringFile currentFile() const;

    bool isCursorOn(unsigned tokenIndex) const
    { return currentFile().isCursorOn(tokenIndex); }
    bool isCursorOn(const CPlusPlus::AST *ast) const
    { return currentFile().isCursorOn(ast); }

private:
    QList<CPlusPlus::AST *> _path;
    CPlusPlus::Snapshot _snapshot;
    CppEditor::Internal::SemanticInfo _semanticInfo;
    CPlusPlus::LookupContext _context;
};

class CPPEDITOR_EXPORT CppQuickFixOperation: public TextEditor::QuickFixOperation
{
    Q_DISABLE_COPY(CppQuickFixOperation)

public:
    CppQuickFixOperation(const CppQuickFixState &state, int priority = -1);
    virtual ~CppQuickFixOperation();

    virtual void perform();

protected:
    virtual void performChanges(CppRefactoringFile *currentFile, CppRefactoringChanges *refactoring) = 0;

    QString fileName() const;

    const CppQuickFixState &state() const;

protected: // Utility functions forwarding to CppQuickFixState
    typedef Utils::ChangeSet::Range Range;

private:
    CppQuickFixState _state;
};

class CPPEDITOR_EXPORT CppQuickFixFactory: public TextEditor::QuickFixFactory
{
    Q_OBJECT

public:
    CppQuickFixFactory();
    virtual ~CppQuickFixFactory();

    virtual QList<TextEditor::QuickFixOperation::Ptr> matchingOperations(TextEditor::QuickFixState *state);
    /*!
        Implement this method to match and create the appropriate
        CppQuickFixOperation objects.
     */
    virtual QList<CppQuickFixOperation::Ptr> match(const CppQuickFixState &state) = 0;

protected:
    /*!
        Creates a list of 1 single element: the shared-pointer to the given
        operation. This shared-pointer takes over the ownership (meaning the
        responsibility to delete the operation).
     */
    static QList<CppQuickFixOperation::Ptr> singleResult(CppQuickFixOperation *operation);

    /// Utility method which creates an empty list.
    static QList<CppQuickFixOperation::Ptr> noResult();
};

namespace Internal {

class CppQuickFixCollector: public TextEditor::QuickFixCollector
{
    Q_OBJECT

public:
    CppQuickFixCollector();
    virtual ~CppQuickFixCollector();

    virtual bool supportsEditor(TextEditor::ITextEditable *editor);
    virtual TextEditor::QuickFixState *initializeCompletion(TextEditor::BaseTextEditor *editor);

    virtual QList<TextEditor::QuickFixFactory *> quickFixFactories() const;

    /// Registers all quick-fixes in this plug-in as auto-released objects.
    static void registerQuickFixes(ExtensionSystem::IPlugin *plugIn);
};

} // end of namespace Internal
} // end of namespace CppEditor

#endif // CPPQUICKFIX_H
