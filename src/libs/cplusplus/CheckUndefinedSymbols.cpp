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

#include "CheckUndefinedSymbols.h"
#include "Overview.h"

#include <Names.h>
#include <Literals.h>
#include <Symbols.h>
#include <TranslationUnit.h>
#include <Scope.h>
#include <AST.h>

#include <QCoreApplication>
#include <QDebug>

using namespace CPlusPlus;

CheckUndefinedSymbols::CheckUndefinedSymbols(TranslationUnit *unit, const LookupContext &context)
    : ASTVisitor(unit), _context(context)
{
    _fileName = context.thisDocument()->fileName();
}

CheckUndefinedSymbols::~CheckUndefinedSymbols()
{ }

QList<Document::DiagnosticMessage> CheckUndefinedSymbols::operator()(AST *ast)
{
    _diagnosticMessages.clear();
    accept(ast);
    return _diagnosticMessages;
}

bool CheckUndefinedSymbols::warning(unsigned line, unsigned column, const QString &text, unsigned length)
{
    Document::DiagnosticMessage m(Document::DiagnosticMessage::Warning, _fileName, line, column, text, length);
    _diagnosticMessages.append(m);
    return false;
}

bool CheckUndefinedSymbols::warning(AST *ast, const QString &text)
{
    const Token &firstToken = tokenAt(ast->firstToken());
    const Token &lastToken = tokenAt(ast->lastToken() - 1);

    const unsigned length = lastToken.end() - firstToken.begin();
    unsigned line = 1, column = 1;
    getTokenStartPosition(ast->firstToken(), &line, &column);

    warning(line, column, text, length);
    return false;
}

bool CheckUndefinedSymbols::visit(UsingDirectiveAST *ast)
{
    checkNamespace(ast->name);
    return false;
}

bool CheckUndefinedSymbols::visit(SimpleDeclarationAST *ast)
{
    return true;
}

bool CheckUndefinedSymbols::visit(NamedTypeSpecifierAST *ast)
{
    if (ast->name) {
        unsigned line, column;
        getTokenStartPosition(ast->firstToken(), &line, &column);

        Scope *enclosingScope = _context.thisDocument()->scopeAt(line, column);
        const QList<Symbol *> candidates = _context.lookup(ast->name->name, enclosingScope);

        Symbol *ty = 0;
        foreach (Symbol *c, candidates) {
            if (c->isTypedef() || c->isClass() || c->isEnum()
                    || c->isForwardClassDeclaration() || c->isTypenameArgument())
                ty = c;
        }

        if (! ty)
            warning(ast->name, QCoreApplication::translate("CheckUndefinedSymbols", "Expected a type-name"));
    }

    return false;
}

void CheckUndefinedSymbols::checkNamespace(NameAST *name)
{
    if (! name)
        return;

    unsigned line, column;
    getTokenStartPosition(name->firstToken(), &line, &column);

    Scope *enclosingScope = _context.thisDocument()->scopeAt(line, column);
    if (ClassOrNamespace *b = _context.lookupType(name->name, enclosingScope)) {
        foreach (Symbol *s, b->symbols()) {
            if (s->isNamespace())
                return;
        }
    }

    const unsigned length = tokenAt(name->lastToken() - 1).end() - tokenAt(name->firstToken()).begin();
    warning(line, column, QCoreApplication::translate("CheckUndefinedSymbols", "Expected a namespace-name"), length);
}
