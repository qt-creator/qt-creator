// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cpplocalsymbols.h"

#include "cppsemanticinfo.h"
#include "semantichighlighter.h"

using namespace CPlusPlus;

namespace CppEditor::Internal {

namespace {

class FindLocalSymbols: protected ASTVisitor
{
public:
    explicit FindLocalSymbols(Document::Ptr doc)
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

    using HighlightingResult = TextEditor::HighlightingResult;

    void enterScope(Scope *scope)
    {
        _scopeStack.append(scope);

        for (int i = 0; i < scope->memberCount(); ++i) {
            if (Symbol *member = scope->memberAt(i)) {
                if (member->isTypedef())
                    continue;
                if (!member->isGenerated() && (member->asDeclaration() || member->asArgument())) {
                    if (member->name() && member->name()->asNameId()) {
                        const Token token = tokenAt(member->sourceLocation());
                        int line, column;
                        getPosition(token.utf16charsBegin(), &line, &column);
                        localUses[member].append(
                                    HighlightingResult(line, column, token.utf16chars(),
                                                       SemanticHighlighter::LocalUse));
                    }
                }
            }
        }
    }

    bool checkLocalUse(NameAST *nameAst, int firstToken)
    {
        if (SimpleNameAST *simpleName = nameAst->asSimpleName()) {
            const Token token = tokenAt(simpleName->identifier_token);
            if (token.generated())
                return false;
            const Identifier *id = identifier(simpleName->identifier_token);
            for (int i = _scopeStack.size() - 1; i != -1; --i) {
                if (Symbol *member = _scopeStack.at(i)->find(id)) {
                    if (member->isTypedef() || !(member->asDeclaration() || member->asArgument()))
                        continue;
                    if (!member->isGenerated() && (member->sourceLocation() < firstToken
                                                   || member->enclosingScope()->asFunction())) {
                        int line, column;
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

    bool visit(CaptureAST *ast) override
    {
        return checkLocalUse(ast->identifier, ast->firstToken());
    }

    bool visit(IdExpressionAST *ast) override
    {
        return checkLocalUse(ast->name, ast->firstToken());
    }

    bool visit(SizeofExpressionAST *ast) override
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

    bool visit(CastExpressionAST *ast) override
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

    bool visit(FunctionDefinitionAST *ast) override
    {
        if (ast->symbol)
            enterScope(ast->symbol);
        return true;
    }

    void endVisit(FunctionDefinitionAST *ast) override
    {
        if (ast->symbol)
            _scopeStack.removeLast();
    }

    bool visit(LambdaExpressionAST *ast) override
    {
        if (ast->lambda_declarator && ast->lambda_declarator->symbol)
            enterScope(ast->lambda_declarator->symbol);
        return true;
    }

    void endVisit(LambdaExpressionAST *ast) override
    {
        if (ast->lambda_declarator && ast->lambda_declarator->symbol)
            _scopeStack.removeLast();
    }

    bool visit(CompoundStatementAST *ast) override
    {
        if (ast->symbol)
            enterScope(ast->symbol);
        return true;
    }

    void endVisit(CompoundStatementAST *ast) override
    {
        if (ast->symbol)
            _scopeStack.removeLast();
    }

    bool visit(IfStatementAST *ast) override
    {
        if (ast->symbol)
            enterScope(ast->symbol);
        return true;
    }

    void endVisit(IfStatementAST *ast) override
    {
        if (ast->symbol)
            _scopeStack.removeLast();
    }

    bool visit(WhileStatementAST *ast) override
    {
        if (ast->symbol)
            enterScope(ast->symbol);
        return true;
    }

    void endVisit(WhileStatementAST *ast) override
    {
        if (ast->symbol)
            _scopeStack.removeLast();
    }

    bool visit(ForStatementAST *ast) override
    {
        if (ast->symbol)
            enterScope(ast->symbol);
        return true;
    }

    void endVisit(ForStatementAST *ast) override
    {
        if (ast->symbol)
            _scopeStack.removeLast();
    }

    bool visit(ForeachStatementAST *ast) override
    {
        if (ast->symbol)
            enterScope(ast->symbol);
        return true;
    }

    void endVisit(ForeachStatementAST *ast) override
    {
        if (ast->symbol)
            _scopeStack.removeLast();
    }

    bool visit(RangeBasedForStatementAST *ast) override
    {
        if (ast->symbol)
            enterScope(ast->symbol);
        return true;
    }

    void endVisit(RangeBasedForStatementAST *ast) override
    {
        if (ast->symbol)
            _scopeStack.removeLast();
    }

    bool visit(SwitchStatementAST *ast) override
    {
        if (ast->symbol)
            enterScope(ast->symbol);
        return true;
    }

    void endVisit(SwitchStatementAST *ast) override
    {
        if (ast->symbol)
            _scopeStack.removeLast();
    }

    bool visit(CatchClauseAST *ast) override
    {
        if (ast->symbol)
            enterScope(ast->symbol);
        return true;
    }

    void endVisit(CatchClauseAST *ast) override
    {
        if (ast->symbol)
            _scopeStack.removeLast();
    }

    bool visit(ExpressionOrDeclarationStatementAST *ast) override
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

} // namespace CppEditor::Internal
