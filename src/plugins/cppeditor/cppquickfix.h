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

#ifndef CPPQUICKFIX_H
#define CPPQUICKFIX_H

#include "cppeditor_global.h"
#include "cppsemanticinfo.h"

#include <ASTfwd.h>
#include <cplusplus/CppDocument.h>
#include <texteditor/quickfix.h>

namespace CppTools {
    class CppModelManagerInterface;
    class CppRefactoringFile;
    class CppRefactoringChanges;
} // namespace CppTools

namespace ExtensionSystem {
class IPlugin;
}

namespace CppEditor {

namespace Internal {
class CppQuickFixCollector;
} // namespace Internal

class CPPEDITOR_EXPORT CppQuickFixState: public TextEditor::QuickFixState
{
    friend class Internal::CppQuickFixCollector;

public:
    CppQuickFixState(TextEditor::BaseTextEditorWidget *editor);

    const QList<CPlusPlus::AST *> &path() const;
    CPlusPlus::Snapshot snapshot() const;
    CPlusPlus::Document::Ptr document() const;
    CppEditor::Internal::SemanticInfo semanticInfo() const;
    const CPlusPlus::LookupContext &context() const;

    const CppTools::CppRefactoringFile currentFile() const;

    bool isCursorOn(unsigned tokenIndex) const;
    bool isCursorOn(const CPlusPlus::AST *ast) const;

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
    explicit CppQuickFixOperation(const CppQuickFixState &state, int priority = -1);
    virtual ~CppQuickFixOperation();

    virtual void perform();

protected:
    virtual void performChanges(CppTools::CppRefactoringFile *currentFile,
                                CppTools::CppRefactoringChanges *refactoring) = 0;

    QString fileName() const;

    const CppQuickFixState &state() const;

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

} // namespace CppEditor

#endif // CPPQUICKFIX_H
