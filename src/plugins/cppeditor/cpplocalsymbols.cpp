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
using namespace CppEditor::Internal;

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
                _functionScope = def->symbol->members();
                accept(ast);
            }
        } else if (ObjCMethodDeclarationAST *decl = ast->asObjCMethodDeclaration()) {
            if (decl->method_prototype->symbol) {
                _functionScope = decl->method_prototype->symbol->members();
                accept(ast);
            }
        }
    }

protected:
    using ASTVisitor::visit;

    bool findMember(Scope *scope, NameAST *ast, unsigned line, unsigned column)
    {
        if (! (ast && ast->name))
            return false;

        const Identifier *id = ast->name->identifier();

        if (scope) {
            for (Symbol *member = scope->lookat(id); member; member = member->next()) {
                if (member->identifier() != id)
                    continue;
                else if (member->line() < line || (member->line() == line && member->column() <= column)) {
                    localUses[member].append(SemanticInfo::Use(line, column, id->size(), SemanticInfo::Use::Local));
                    return true;
                }
            }
        }

        return false;
    }

    void searchUsesInTemplateArguments(NameAST *name)
    {
        if (! name)
            return;

        else if (TemplateIdAST *template_id = name->asTemplateId()) {
            for (TemplateArgumentListAST *it = template_id->template_argument_list; it; it = it->next) {
                accept(it->value);
            }
        }
    }

    virtual bool visit(SimpleNameAST *ast)
    { return findMemberForToken(ast->firstToken(), ast); }

    bool findMemberForToken(unsigned tokenIdx, NameAST *ast)
    {
        const Token &tok = tokenAt(tokenIdx);
        if (tok.generated())
            return false;

        unsigned line, column;
        getTokenStartPosition(tokenIdx, &line, &column);

        Scope *scope = _doc->scopeAt(line, column);

        while (scope) {
            if (scope->isFunctionScope()) {
                Function *fun = scope->owner()->asFunction();
                if (findMember(fun->members(), ast, line, column))
                    return false;
                else if (findMember(fun->arguments(), ast, line, column))
                    return false;
            } else if (scope->isObjCMethodScope()) {
                ObjCMethod *method = scope->owner()->asObjCMethod();
                if (findMember(method->members(), ast, line, column))
                    return false;
                else if (findMember(method->arguments(), ast, line, column))
                    return false;
            } else if (scope->isBlockScope()) {
                if (findMember(scope, ast, line, column))
                    return false;
            } else {
                break;
            }

            scope = scope->enclosingScope();
        }

        return false;
    }

    virtual bool visit(MemInitializerAST *ast)
    {
        accept(ast->expression_list);
        return false;
    }

    virtual bool visit(TemplateIdAST *ast)
    {
        for (TemplateArgumentListAST *arg = ast->template_argument_list; arg; arg = arg->next)
            accept(arg->value);

        const Token &tok = tokenAt(ast->identifier_token);
        if (tok.generated())
            return false;

        unsigned line, column;
        getTokenStartPosition(ast->firstToken(), &line, &column);

        Scope *scope = _doc->scopeAt(line, column);

        while (scope) {
            if (scope->isFunctionScope()) {
                Function *fun = scope->owner()->asFunction();
                if (findMember(fun->members(), ast, line, column))
                    return false;
                else if (findMember(fun->arguments(), ast, line, column))
                    return false;
            } else if (scope->isBlockScope()) {
                if (findMember(scope, ast, line, column))
                    return false;
            } else {
                break;
            }

            scope = scope->enclosingScope();
        }

        return false;
    }

    virtual bool visit(QualifiedNameAST *ast)
    {
        for (NestedNameSpecifierListAST *it = ast->nested_name_specifier_list; it; it = it->next)
            searchUsesInTemplateArguments(it->value->class_or_namespace_name);

        searchUsesInTemplateArguments(ast->unqualified_name);
        return false;
    }

    virtual bool visit(MemberAccessAST *ast)
    {
        // accept only the base expression
        accept(ast->base_expression);
        // and ignore the member name.
        return false;
    }

    virtual bool visit(ElaboratedTypeSpecifierAST *)
    {
        // ### template args
        return false;
    }

    virtual bool visit(ClassSpecifierAST *)
    {
        // ### template args
        return false;
    }

    virtual bool visit(EnumSpecifierAST *)
    {
        // ### template args
        return false;
    }

    virtual bool visit(UsingDirectiveAST *)
    {
        return false;
    }

    virtual bool visit(UsingAST *ast)
    {
        accept(ast->name);
        return false;
    }

    virtual bool visit(QtMemberDeclarationAST *ast)
    {
        if (tokenKind(ast->q_token) == T_Q_D)
            hasD = true;
        else
            hasQ = true;

        return true;
    }

    virtual bool visit(ExpressionOrDeclarationStatementAST *ast)
    {
        accept(ast->declaration);
        return false;
    }

    virtual bool visit(FunctionDeclaratorAST *ast)
    {
        accept(ast->parameters);

        for (SpecifierListAST *it = ast->cv_qualifier_list; it; it = it->next)
            accept(it->value);

        accept(ast->exception_specification);

        return false;
    }

    virtual bool visit(ObjCMethodPrototypeAST *ast)
    {
        accept(ast->argument_list);
        return false;
    }

    virtual bool visit(ObjCMessageArgumentDeclarationAST *ast)
    {
        accept(ast->param_name);
        return false;
    }
};

} // end of anonymous namespace


LocalSymbols::LocalSymbols(CPlusPlus::Document::Ptr doc, CPlusPlus::DeclarationAST *ast)
{
    FindLocalSymbols FindLocalSymbols(doc);
    FindLocalSymbols(ast);
    hasD = FindLocalSymbols.hasD;
    hasQ = FindLocalSymbols.hasQ;
    uses = FindLocalSymbols.localUses;
}
