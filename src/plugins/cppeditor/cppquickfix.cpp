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

#include "cppquickfix.h"
#include "cppeditor.h"
#include "cppquickfixcollector.h"

#include <AST.h>
#include <TranslationUnit.h>
#include <Token.h>

#include <cplusplus/ASTPath.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/ResolveExpression.h>
#include <cplusplus/Overview.h>
#include <cplusplus/TypeOfExpression.h>
#include <cplusplus/DependencyTable.h>
#include <cplusplus/CppRewriter.h>

#include <cpptools/cpprefactoringchanges.h>

#include <QtGui/QTextBlock>
#include <QtCore/QFileInfo>

using namespace CppEditor;
using namespace CppEditor::Internal;
using namespace CppTools;
using namespace TextEditor;
using namespace CPlusPlus;
using namespace Utils;

CppQuickFixState::CppQuickFixState(TextEditor::BaseTextEditorWidget *editor)
    : QuickFixState(editor)
{}

const QList<AST *> &CppQuickFixState::path() const
{
    return _path;
}

Snapshot CppQuickFixState::snapshot() const
{
    return _snapshot;
}

Document::Ptr CppQuickFixState::document() const
{
    return _semanticInfo.doc;
}

SemanticInfo CppQuickFixState::semanticInfo() const
{
    return _semanticInfo;
}

const LookupContext &CppQuickFixState::context() const
{
    return _context;
}

const CppRefactoringFile CppQuickFixState::currentFile() const
{
    return CppRefactoringFile(editor(), document());
}

bool CppQuickFixState::isCursorOn(unsigned tokenIndex) const
{
    return currentFile().isCursorOn(tokenIndex);
}

bool CppQuickFixState::isCursorOn(const CPlusPlus::AST *ast) const
{
    return currentFile().isCursorOn(ast);
}

CppQuickFixOperation::CppQuickFixOperation(const CppQuickFixState &state, int priority)
    : QuickFixOperation(priority)
    , _state(state)
{}

CppQuickFixOperation::~CppQuickFixOperation()
{}

void CppQuickFixOperation::perform()
{
    CppRefactoringChanges refactoring(_state.snapshot());
    CppRefactoringFile current = refactoring.file(fileName());

    performChanges(&current, &refactoring);
}

const CppQuickFixState &CppQuickFixOperation::state() const
{
    return _state;
}

QString CppQuickFixOperation::fileName() const
{ return state().document()->fileName(); }

CppQuickFixFactory::CppQuickFixFactory()
{
}

CppQuickFixFactory::~CppQuickFixFactory()
{
}

QList<QuickFixOperation::Ptr> CppQuickFixFactory::matchingOperations(QuickFixState *state)
{
    if (CppQuickFixState *cppState = static_cast<CppQuickFixState *>(state))
        return match(*cppState);
    else
        return QList<TextEditor::QuickFixOperation::Ptr>();
}

QList<CppQuickFixOperation::Ptr> CppQuickFixFactory::singleResult(CppQuickFixOperation *operation)
{
    QList<CppQuickFixOperation::Ptr> result;
    result.append(CppQuickFixOperation::Ptr(operation));
    return result;
}

QList<CppQuickFixOperation::Ptr> CppQuickFixFactory::noResult()
{
    return QList<CppQuickFixOperation::Ptr>();
}
