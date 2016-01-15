/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "cpplocalsymbols.h"
#include "semantichighlighter.h"

#include "cppsemanticinfo.h"

using namespace CPlusPlus;
using namespace CppTools;

namespace {

class FindLocalSymbols: protected ASTVisitor
{
public:
    FindLocalSymbols(Document::Ptr doc)
        : ASTVisitor(doc->translationUnit())
    { }

    // local and external uses.
    SemanticInfo::LocalUseMap localUses;

    void operator()(DeclarationAST *ast)
    {
        localUses.clear();

        if (!ast)
            return;

        if (FunctionDefinitionAST *def = ast->asFunctionDefinition()) {
            if (def->symbol) {
                accept(ast);
            }
        } else if (ObjCMethodDeclarationAST *decl = ast->asObjCMethodDeclaration()) {
            if (decl->method_prototype->symbol) {
                accept(ast);
            }
        }
    }

protected:
    using ASTVisitor::visit;
    using ASTVisitor::endVisit;

    typedef TextEditor::HighlightingResult HighlightingResult;

    void enterScope(Scope *scope)
    {
        _scopeStack.append(scope);

        for (unsigned i = 0; i < scope->memberCount(); ++i) {
            if (Symbol *member = scope->memberAt(i)) {
                if (member->isTypedef())
                    continue;
                if (!member->isGenerated() && (member->isDeclaration() || member->isArgument())) {
                    if (member->name() && member->name()->isNameId()) {
                        const Token token = tokenAt(member->sourceLocation());
                        unsigned line, column;
                        getPosition(token.utf16charsBegin(), &line, &column);
                        localUses[member].append(
                                    HighlightingResult(line, column, token.utf16chars(),
                                                       SemanticHighlighter::LocalUse));
                    }
                }
            }
        }
    }

    bool checkLocalUse(NameAST *nameAst, unsigned firstToken)
    {
        if (SimpleNameAST *simpleName = nameAst->asSimpleName()) {
            const Token token = tokenAt(simpleName->identifier_token);
            if (token.generated())
                return false;
            const Identifier *id = identifier(simpleName->identifier_token);
            for (int i = _scopeStack.size() - 1; i != -1; --i) {
                if (Symbol *member = _scopeStack.at(i)->find(id)) {
                    if (member->isTypedef() || !(member->isDeclaration() || member->isArgument()))
                        continue;
                    if (!member->isGenerated() && (member->sourceLocation() < firstToken
                                                   || member->enclosingScope()->isFunction())) {
                        unsigned line, column;
                        getTokenStartPosition(simpleName->identifier_token, &line, &column);
                        localUses[member].append(
                                    HighlightingResult(line, column, token.utf16chars(),
                                                       SemanticHighlighter::LocalUse));
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

    virtual bool visit(LambdaExpressionAST *ast)
    {
        if (ast->lambda_declarator && ast->lambda_declarator->symbol)
            enterScope(ast->lambda_declarator->symbol);
        return true;
    }

    virtual void endVisit(LambdaExpressionAST *ast)
    {
        if (ast->lambda_declarator && ast->lambda_declarator->symbol)
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


LocalSymbols::LocalSymbols(Document::Ptr doc, DeclarationAST *ast)
{
    FindLocalSymbols findLocalSymbols(doc);
    findLocalSymbols(ast);
    uses = findLocalSymbols.localUses;
}
