/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/
// Copyright (c) 2008 Roberto Raggi <roberto.raggi@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "CheckDeclarator.h"
#include "Semantic.h"
#include "AST.h"
#include "Control.h"
#include "TranslationUnit.h"
#include "Literals.h"
#include "CoreTypes.h"
#include "Symbols.h"

CPLUSPLUS_BEGIN_NAMESPACE

CheckDeclarator::CheckDeclarator(Semantic *semantic)
    : SemanticCheck(semantic),
      _declarator(0),
      _scope(0),
      _name(0)
{ }

CheckDeclarator::~CheckDeclarator()
{ }

FullySpecifiedType CheckDeclarator::check(DeclaratorAST *declarator,
                                               FullySpecifiedType type,
                                               Scope *scope,
                                               Name **name)
{
    FullySpecifiedType previousType = switchFullySpecifiedType(type);
    Scope *previousScope = switchScope(scope);
    DeclaratorAST *previousDeclarator = switchDeclarator(declarator);
    Name **previousName = switchName(name);
    accept(declarator);
    (void) switchName(previousName);
    (void) switchDeclarator(previousDeclarator);
    (void) switchScope(previousScope);
    return switchFullySpecifiedType(previousType);
}

FullySpecifiedType CheckDeclarator::check(PtrOperatorAST *ptrOperators,
                                               FullySpecifiedType type,
                                               Scope *scope)
{
    FullySpecifiedType previousType = switchFullySpecifiedType(type);
    Scope *previousScope = switchScope(scope);
    accept(ptrOperators);
    (void) switchScope(previousScope);
    return switchFullySpecifiedType(previousType);
}

DeclaratorAST *CheckDeclarator::switchDeclarator(DeclaratorAST *declarator)
{
    DeclaratorAST *previousDeclarator = _declarator;
    _declarator = declarator;
    return previousDeclarator;
}

FullySpecifiedType CheckDeclarator::switchFullySpecifiedType(FullySpecifiedType type)
{
    FullySpecifiedType previousType = _fullySpecifiedType;
    _fullySpecifiedType = type;
    return previousType;
}

Scope *CheckDeclarator::switchScope(Scope *scope)
{
    Scope *previousScope = _scope;
    _scope = scope;
    return previousScope;
}

Name **CheckDeclarator::switchName(Name **name)
{
    Name **previousName = _name;
    _name = name;
    return previousName;
}

bool CheckDeclarator::visit(DeclaratorAST *ast)
{
    accept(ast->ptr_operators);
    accept(ast->postfix_declarators);
    accept(ast->core_declarator);

    // ### check the initializer
    // FullySpecifiedType exprTy = semantic()->check(ast->initializer, _scope);

    if (ast->initializer && _fullySpecifiedType->isFunction()) {
        _fullySpecifiedType->asFunction()->setPureVirtual(true);
    }
    return false;
}

bool CheckDeclarator::visit(DeclaratorIdAST *ast)
{
    Name *name = semantic()->check(ast->name, _scope);
    if (_name)
        *_name = name;
    return false;
}

bool CheckDeclarator::visit(NestedDeclaratorAST *ast)
{
    accept(ast->declarator);
    return false;
}

bool CheckDeclarator::visit(FunctionDeclaratorAST *ast)
{
    Function *fun = control()->newFunction(ast->firstToken());
    fun->setReturnType(_fullySpecifiedType);

    if (ast->parameters) {
        DeclarationAST *parameter_declarations = ast->parameters->parameter_declarations;
        for (DeclarationAST *decl = parameter_declarations; decl; decl = decl->next) {
            semantic()->check(decl, fun->arguments());
        }

        if (ast->parameters->dot_dot_dot_token)
            fun->setVariadic(true);
    }

    // check the arguments
    bool hasDefaultArguments = false;
    for (unsigned i = 0; i < fun->argumentCount(); ++i) {
        Argument *arg = fun->argumentAt(i)->asArgument();
        if (hasDefaultArguments && ! arg->hasInitializer()) {
            translationUnit()->error(ast->firstToken(),
                "default argument missing for parameter at position %d", i + 1);
        } else if (! hasDefaultArguments) {
            hasDefaultArguments = arg->hasInitializer();
        }
    }

    FullySpecifiedType funTy(fun);
    _fullySpecifiedType = funTy;

    for (SpecifierAST *it = ast->cv_qualifier_seq; it; it = it->next) {
        SimpleSpecifierAST *cv = static_cast<SimpleSpecifierAST *>(it);
        int k = tokenKind(cv->specifier_token);
        if (k == T_CONST)
            fun->setConst(true);
        else if (k == T_VOLATILE)
            fun->setVolatile(true);
    }

    accept(ast->next);
    return false;
}

bool CheckDeclarator::visit(ArrayDeclaratorAST *ast)
{
    ArrayType *ty = control()->arrayType(_fullySpecifiedType); // ### set the dimension
    FullySpecifiedType exprTy = semantic()->check(ast->expression, _scope);
    FullySpecifiedType arrTy(ty);
    _fullySpecifiedType = ty;
    accept(ast->next);
    return false;
}

bool CheckDeclarator::visit(PointerToMemberAST *ast)
{
    Name *memberName = semantic()->check(ast->nested_name_specifier, _scope);
    PointerToMemberType *ptrTy = control()->pointerToMemberType(memberName, _fullySpecifiedType);
    FullySpecifiedType ty(ptrTy);
    _fullySpecifiedType = ty;
    applyCvQualifiers(ast->cv_qualifier_seq);
    accept(ast->next);
    return false;
}

bool CheckDeclarator::visit(PointerAST *ast)
{
    PointerType *ptrTy = control()->pointerType(_fullySpecifiedType);
    FullySpecifiedType ty(ptrTy);
    _fullySpecifiedType = ty;
    applyCvQualifiers(ast->cv_qualifier_seq);
    accept(ast->next);
    return false;
}

bool CheckDeclarator::visit(ReferenceAST *ast)
{
    ReferenceType *refTy = control()->referenceType(_fullySpecifiedType);
    FullySpecifiedType ty(refTy);
    _fullySpecifiedType = ty;
    accept(ast->next);
    return false;
}

void CheckDeclarator::applyCvQualifiers(SpecifierAST *cv)
{
    for (; cv; cv = cv->next) {
        SimpleSpecifierAST *spec = static_cast<SimpleSpecifierAST *>(cv);
        switch (translationUnit()->tokenKind(spec->specifier_token)) {
        case T_VOLATILE:
            _fullySpecifiedType.setVolatile(true);
            break;
        case T_CONST:
            _fullySpecifiedType.setConst(true);
            break;
        default:
            break;
        } // switch
    }
}

CPLUSPLUS_END_NAMESPACE
