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

#include "cppquickfix.h"
#include "cppeditor.h"
#include "cppdeclfromdef.h"

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

#include <cppeditor/cppeditor.h>
#include <cppeditor/cpprefactoringchanges.h>
#include <cpptools/cpptoolsconstants.h>
#include <cpptools/cppmodelmanagerinterface.h>
#include <extensionsystem/pluginmanager.h>

#include <QtGui/QTextBlock>

using namespace CppEditor;
using namespace CppEditor::Internal;
using namespace TextEditor;
using namespace CPlusPlus;
using namespace Utils;

CppQuickFixState::CppQuickFixState(TextEditor::BaseTextEditor *editor)
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

Scope *CppQuickFixState::scopeAt(unsigned index) const
{
    unsigned line, column;
    document()->translationUnit()->getTokenStartPosition(index, &line, &column);
    return document()->scopeAt(line, column);
}

bool CppQuickFixState::isCursorOn(unsigned tokenIndex) const
{
    QTextCursor tc = textCursor();
    int cursorBegin = tc.selectionStart();

    int start = startOf(tokenIndex);
    int end = endOf(tokenIndex);

    if (cursorBegin >= start && cursorBegin <= end)
        return true;

    return false;
}

bool CppQuickFixState::isCursorOn(const AST *ast) const
{
    QTextCursor tc = textCursor();
    int cursorBegin = tc.selectionStart();

    int start = startOf(ast);
    int end = endOf(ast);

    if (cursorBegin >= start && cursorBegin <= end)
        return true;

    return false;
}

ChangeSet::Range CppQuickFixState::range(unsigned tokenIndex) const
{
    const Token &token = tokenAt(tokenIndex);
    unsigned line, column;
    document()->translationUnit()->getPosition(token.begin(), &line, &column);
    const int start = editor()->document()->findBlockByNumber(line - 1).position() + column - 1;
    return ChangeSet::Range(start, start + token.length());
}

ChangeSet::Range CppQuickFixState::range(AST *ast) const
{
    return ChangeSet::Range(startOf(ast), endOf(ast));
}

int CppQuickFixState::startOf(unsigned index) const
{
    unsigned line, column;
    document()->translationUnit()->getPosition(tokenAt(index).begin(), &line, &column);
    return editor()->document()->findBlockByNumber(line - 1).position() + column - 1;
}

int CppQuickFixState::startOf(const AST *ast) const
{
    return startOf(ast->firstToken());
}

int CppQuickFixState::endOf(unsigned index) const
{
    unsigned line, column;
    document()->translationUnit()->getPosition(tokenAt(index).end(), &line, &column);
    return editor()->document()->findBlockByNumber(line - 1).position() + column - 1;
}

int CppQuickFixState::endOf(const AST *ast) const
{
    if (unsigned end = ast->lastToken())
        return endOf(end - 1);
    else
        return 0;
}

void CppQuickFixState::startAndEndOf(unsigned index, int *start, int *end) const
{
    unsigned line, column;
    Token token(tokenAt(index));
    document()->translationUnit()->getPosition(token.begin(), &line, &column);
    *start = editor()->document()->findBlockByNumber(line - 1).position() + column - 1;
    *end = *start + token.length();
}

QString CppQuickFixState::textOf(const AST *ast) const
{
    return textOf(startOf(ast), endOf(ast));
}

const Token &CppQuickFixState::tokenAt(unsigned index) const
{
    return document()->translationUnit()->tokenAt(index);
}

CppQuickFixOperation::CppQuickFixOperation(const CppQuickFixState &state, int priority)
    : QuickFixOperation(priority)
    , _state(state)
    , _refactoringChanges(new CppRefactoringChanges(state.document(), state.snapshot()))
{}

CppQuickFixOperation::~CppQuickFixOperation()
{}

const CppQuickFixState &CppQuickFixOperation::state() const
{
    return _state;
}

QString CppQuickFixOperation::fileName() const
{ return state().document()->fileName(); }

CppRefactoringChanges *CppQuickFixOperation::refactoringChanges() const
{ return _refactoringChanges.data(); }

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

CppQuickFixCollector::CppQuickFixCollector()
{
}

CppQuickFixCollector::~CppQuickFixCollector()
{
}

bool CppQuickFixCollector::supportsEditor(TextEditor::ITextEditable *editor)
{
    return CppTools::CppModelManagerInterface::instance()->isCppEditor(editor);
}

TextEditor::QuickFixState *CppQuickFixCollector::initializeCompletion(TextEditor::BaseTextEditor *editor)
{
    if (CPPEditor *cppEditor = qobject_cast<CPPEditor *>(editor)) {
        const SemanticInfo info = cppEditor->semanticInfo();

        if (info.revision != cppEditor->editorRevision()) {
            // outdated
            qWarning() << "TODO: outdated semantic info, force a reparse.";
            return 0;
        }

        if (info.doc) {
            ASTPath astPath(info.doc);

            const QList<AST *> path = astPath(cppEditor->textCursor());
            if (! path.isEmpty()) {
                CppQuickFixState *state = new CppQuickFixState(editor);
                state->_path = path;
                state->_semanticInfo = info;
                state->_snapshot = CppTools::CppModelManagerInterface::instance()->snapshot();
                state->_context = LookupContext(info.doc, state->snapshot());
                return state;
            }
        }
    }

    return 0;
}

QList<TextEditor::QuickFixFactory *> CppQuickFixCollector::quickFixFactories() const
{
    QList<TextEditor::QuickFixFactory *> results;
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    foreach (CppQuickFixFactory *f, pm->getObjects<CppEditor::CppQuickFixFactory>())
        results.append(f);
    return results;
}
