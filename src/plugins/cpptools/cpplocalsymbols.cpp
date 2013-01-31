/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cpplocalsymbols.h"
#include "cppsemanticinfo.h"

#include <cplusplus/CppDocument.h>
#include <ASTVisitor.h>
#include <AST.h>
#include <Scope.h>
#include <Symbols.h>
#include <CoreTypes.h>
#include <Names.h>
#include <Literals.h>

using namespace CPlusPlus;
using namespace CppTools;

namespace {

class FindLocalSymbols: protected ASTVisitor
{
    Scope *_functionScope;
    Document::Ptr _doc;

public:
    FindLocalSymbols(Document::Ptr doc)
        : ASTVisitor(doc->translationUnit()), _doc(doc), hasD(false), hasQ(false)
    { }

    // local and external uses.
    SemanticInfo::LocalUseMap localUses;
    bool hasD;
    bool hasQ;

    void operator()(DeclarationAST *ast)
    {
        localUses.clear();

        if (!ast)
            return;

        if (FunctionDefinitionAST *def = ast->asFunctionDefinition()) {
            if (def->symbol) {
                _functionScope = def->symbol;
                accept(ast);
            }
        } else if (ObjCMethodDeclarationAST *decl = ast->asObjCMethodDeclaration()) {
            if (decl->method_prototype->symbol) {
                _functionScope = decl->method_prototype->symbol;
                accept(ast);
            }
        }
    }

protected:
    using ASTVisitor::visit;
    using ASTVisitor::endVisit;

    void enterScope(Scope *scope)
    {
        _scopeStack.append(scope);

        for (unsigned i = 0; i < scope->memberCount(); ++i) {
            if (Symbol *member = scope->memberAt(i)) {
                if (member->isTypedef())
                    continue;
                else if (! member->isGenerated() && (member->isDeclaration() || member->isArgument())) {
                    if (member->name() && member->name()->isNameId()) {
                        const Identifier *id = member->identifier();
                        unsigned line, column;
                        getTokenStartPosition(member->sourceLocation(), &line, &column);
                        localUses[member].append(SemanticInfo::Use(line, column, id->size(), SemanticInfo::LocalUse));
                    }
                }
            }
        }
    }

    bool checkLocalUse(NameAST *nameAst, unsigned firstToken)
    {
        if (SimpleNameAST *simpleName = nameAst->asSimpleName()) {
            if (tokenAt(simpleName->identifier_token).generated())
                return false;
            const Identifier *id = identifier(simpleName->identifier_token);
            for (int i = _scopeStack.size() - 1; i != -1; --i) {
                if (Symbol *member = _scopeStack.at(i)->find(id)) {
                    if (member->isTypedef() ||
                            !(member->isDeclaration() || member->isArgument()))
                        continue;
                    else if (!member->isGenerated() && (member->sourceLocation() < firstToken || member->enclosingScope()->isFunction())) {
                        unsigned line, column;
                        getTokenStartPosition(simpleName->identifier_token, &line, &column);
                        localUses[member].append(SemanticInfo::Use(line, column, id->size(), SemanticInfo::LocalUse));
                        return false;
                    }
                }
            }
        }

        return true;
    }

    virtual bool visit(CaptureAST *ast)
    {
        return checkLocalUse(ast->identifier, ast->firstToken());
    }

    virtual bool visit(IdExpressionAST *ast)
    {
        return checkLocalUse(ast->name, ast->firstToken());
    }

    virtual bool visit(SizeofExpressionAST *ast)
    {
        if (ast->expression && ast->expression->asTypeId()) {
            TypeIdAST *typeId = ast->expression->asTypeId();
            if (!typeId->declarator && typeId->type_specifier_list && !typeId->type_specifier_list->next) {
                if (NamedTypeSpecifierAST *namedTypeSpec = typeId->type_specifier_list->value->asNamedTypeSpecifier()) {
                    if (checkLocalUse(namedTypeSpec->name, namedTypeSpec->firstToken()))
                        return false;
                }
            }
        }

        return true;
    }

    virtual bool visit(CastExpressionAST *ast)
    {
        if (ast->expression && ast->expression->asUnaryExpression()) {
            TypeIdAST *typeId = ast->type_id->asTypeId();
            if (typeId && !typeId->declarator && typeId->type_specifier_list && !typeId->type_specifier_list->next) {
                if (NamedTypeSpecifierAST *namedTypeSpec = typeId->type_specifier_list->value->asNamedTypeSpecifier()) {
                    if (checkLocalUse(namedTypeSpec->name, namedTypeSpec->firstToken())) {
                        accept(ast->expression);
                        return false;
                    }
                }
            }
        }

        return true;
    }

    virtual bool visit(QtMemberDeclarationAST *ast)
    {
        if (tokenKind(ast->q_token) == T_Q_D)
            hasD = true;
        else
            hasQ = true;

        return true;
    }

    virtual bool visit(FunctionDefinitionAST *ast)
    {
        if (ast->symbol)
            enterScope(ast->symbol);
        return true;
    }

    virtual void endVisit(FunctionDefinitionAST *ast)
    {
        if (ast->symbol)
            _scopeStack.removeLast();
    }

    virtual bool visit(CompoundStatementAST *ast)
    {
        if (ast->symbol)
            enterScope(ast->symbol);
        return true;
    }

    virtual void endVisit(CompoundStatementAST *ast)
    {
        if (ast->symbol)
            _scopeStack.removeLast();
    }

    virtual bool visit(IfStatementAST *ast)
    {
        if (ast->symbol)
            enterScope(ast->symbol);
        return true;
    }

    virtual void endVisit(IfStatementAST *ast)
    {
        if (ast->symbol)
            _scopeStack.removeLast();
    }

    virtual bool visit(WhileStatementAST *ast)
    {
        if (ast->symbol)
            enterScope(ast->symbol);
        return true;
    }

    virtual void endVisit(WhileStatementAST *ast)
    {
        if (ast->symbol)
            _scopeStack.removeLast();
    }

    virtual bool visit(ForStatementAST *ast)
    {
        if (ast->symbol)
            enterScope(ast->symbol);
        return true;
    }

    virtual void endVisit(ForStatementAST *ast)
    {
        if (ast->symbol)
            _scopeStack.removeLast();
    }

    virtual bool visit(ForeachStatementAST *ast)
    {
        if (ast->symbol)
            enterScope(ast->symbol);
        return true;
    }

    virtual void endVisit(ForeachStatementAST *ast)
    {
        if (ast->symbol)
            _scopeStack.removeLast();
    }

    virtual bool visit(RangeBasedForStatementAST *ast)
    {
        if (ast->symbol)
            enterScope(ast->symbol);
        return true;
    }

    virtual void endVisit(RangeBasedForStatementAST *ast)
    {
        if (ast->symbol)
            _scopeStack.removeLast();
    }

    virtual bool visit(SwitchStatementAST *ast)
    {
        if (ast->symbol)
            enterScope(ast->symbol);
        return true;
    }

    virtual void endVisit(SwitchStatementAST *ast)
    {
        if (ast->symbol)
            _scopeStack.removeLast();
    }

    virtual bool visit(CatchClauseAST *ast)
    {
        if (ast->symbol)
            enterScope(ast->symbol);
        return true;
    }

    virtual void endVisit(CatchClauseAST *ast)
    {
        if (ast->symbol)
            _scopeStack.removeLast();
    }

    virtual bool visit(ExpressionOrDeclarationStatementAST *ast)
    {
        accept(ast->declaration);
        return false;
    }

private:
    QList<Scope *> _scopeStack;
};

} // end of anonymous namespace


LocalSymbols::LocalSymbols(CPlusPlus::Document::Ptr doc, CPlusPlus::DeclarationAST *ast)
{
    FindLocalSymbols findLocalSymbols(doc);
    findLocalSymbols(ast);
    hasD = findLocalSymbols.hasD;
    hasQ = findLocalSymbols.hasQ;
    uses = findLocalSymbols.localUses;
}
