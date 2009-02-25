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

#include "CheckExpression.h"
#include "Semantic.h"
#include "TranslationUnit.h"
#include "AST.h"
#include "Scope.h"
#include "Literals.h"
#include "CoreTypes.h"
#include "Symbols.h"
#include "Control.h"

CPLUSPLUS_BEGIN_NAMESPACE

CheckExpression::CheckExpression(Semantic *semantic)
    : SemanticCheck(semantic),
      _expression(0),
      _scope(0),
      _checkOldStyleCasts(false)
{ }

CheckExpression::~CheckExpression()
{ }

FullySpecifiedType CheckExpression::check(ExpressionAST *expression, Scope *scope)
{
    FullySpecifiedType previousType = switchFullySpecifiedType(FullySpecifiedType());
    Scope *previousScope = switchScope(scope);
    ExpressionAST *previousExpression = switchExpression(expression);
    accept(expression);
    (void) switchExpression(previousExpression);
    (void) switchScope(previousScope);
    return switchFullySpecifiedType(previousType);
}

ExpressionAST *CheckExpression::switchExpression(ExpressionAST *expression)
{
    ExpressionAST *previousExpression = _expression;
    _expression = expression;
    return previousExpression;
}

FullySpecifiedType CheckExpression::switchFullySpecifiedType(FullySpecifiedType type)
{
    FullySpecifiedType previousType = _fullySpecifiedType;
    _fullySpecifiedType = type;
    return previousType;
}

Scope *CheckExpression::switchScope(Scope *scope)
{
    Scope *previousScope = _scope;
    _scope = scope;
    return previousScope;
}

bool CheckExpression::visit(ExpressionListAST *ast)
{
    for (ExpressionListAST *it = ast; it; it = it->next) {
        FullySpecifiedType exprTy = semantic()->check(it->expression, _scope);
    }
    return false;
}

bool CheckExpression::visit(BinaryExpressionAST *ast)
{
    FullySpecifiedType leftExprTy = semantic()->check(ast->left_expression, _scope);
    FullySpecifiedType rightExprTy = semantic()->check(ast->right_expression, _scope);
    return false;
}

bool CheckExpression::visit(CastExpressionAST *ast)
{
    FullySpecifiedType castTy = semantic()->check(ast->type_id, _scope);
    FullySpecifiedType exprTy = semantic()->check(ast->expression, _scope);
    if (_checkOldStyleCasts && ! castTy->isVoidType())
        translationUnit()->warning(ast->firstToken(),
                                   "ugly old style cast");
    return false;
}

bool CheckExpression::visit(ConditionAST *ast)
{
    FullySpecifiedType typeSpecTy = semantic()->check(ast->type_specifier, _scope);
    Name *name = 0;
    FullySpecifiedType declTy = semantic()->check(ast->declarator, typeSpecTy.qualifiedType(),
                                                  _scope, &name);
    Declaration *decl = control()->newDeclaration(ast->declarator->firstToken(), name);
    decl->setType(declTy);
    _scope->enterSymbol(decl);
    return false;
}

bool CheckExpression::visit(ConditionalExpressionAST *ast)
{
    FullySpecifiedType condTy = semantic()->check(ast->condition, _scope);
    FullySpecifiedType leftExprTy = semantic()->check(ast->left_expression, _scope);
    FullySpecifiedType rightExprTy = semantic()->check(ast->right_expression, _scope);
    return false;
}

bool CheckExpression::visit(CppCastExpressionAST *ast)
{
    FullySpecifiedType typeIdTy = semantic()->check(ast->type_id, _scope);
    FullySpecifiedType exprTy = semantic()->check(ast->expression, _scope);
    return false;
}

bool CheckExpression::visit(DeleteExpressionAST *ast)
{
    FullySpecifiedType exprTy = semantic()->check(ast->expression, _scope);
    return false;
}

bool CheckExpression::visit(ArrayInitializerAST *ast)
{
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        FullySpecifiedType exprTy = semantic()->check(it->expression, _scope);
    }
    return false;
}

bool CheckExpression::visit(QualifiedNameAST *ast)
{
    Name *name = semantic()->check(ast, _scope);
    _scope->addUse(ast->firstToken(), name);
    return false;
}

bool CheckExpression::visit(OperatorFunctionIdAST *ast)
{
    Name *name = semantic()->check(ast, _scope);
    _scope->addUse(ast->firstToken(), name);
    return false;
}

bool CheckExpression::visit(ConversionFunctionIdAST *ast)
{
    Name *name = semantic()->check(ast, _scope);
    _scope->addUse(ast->firstToken(), name);
    return false;
}

bool CheckExpression::visit(SimpleNameAST *ast)
{
    Name *name = semantic()->check(ast, _scope);
    _scope->addUse(ast->firstToken(), name);
    return false;
}

bool CheckExpression::visit(DestructorNameAST *ast)
{
    Name *name = semantic()->check(ast, _scope);
    _scope->addUse(ast->firstToken(), name);
    return false;
}

bool CheckExpression::visit(TemplateIdAST *ast)
{
    Name *name = semantic()->check(ast, _scope);
    _scope->addUse(ast->firstToken(), name);
    return false;
}

bool CheckExpression::visit(NewExpressionAST *ast)
{
    FullySpecifiedType exprTy = semantic()->check(ast->expression, _scope);
    FullySpecifiedType typeIdTy = semantic()->check(ast->type_id, _scope);
    // ### process new-typeid
    // ### process new-initializer
    return false;
}

bool CheckExpression::visit(TypeidExpressionAST *ast)
{
    FullySpecifiedType exprTy = semantic()->check(ast->expression, _scope);
    return false;
}

bool CheckExpression::visit(TypenameCallExpressionAST *ast)
{
    if (Name *name = semantic()->check(ast->name, _scope)) {
        _scope->addUse(ast->name->firstToken(), name);
    }
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        FullySpecifiedType exprTy = semantic()->check(it->expression, _scope);
    }
    return false;
}

bool CheckExpression::visit(TypeConstructorCallAST *ast)
{
    FullySpecifiedType typeSpecTy = semantic()->check(ast->type_specifier, _scope);
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        FullySpecifiedType exprTy = semantic()->check(it->expression, _scope);
    }
    return false;
}

bool CheckExpression::visit(PostfixExpressionAST *ast)
{
    FullySpecifiedType exprTy = semantic()->check(ast->base_expression, _scope);
    for (PostfixAST *fx = ast->postfix_expressions; fx; fx = fx->next) {
        accept(fx); // ### not exactly.
    }
    return false;
}

bool CheckExpression::visit(SizeofExpressionAST *ast)
{
    FullySpecifiedType exprTy = semantic()->check(ast->expression, _scope);
    return false;
}

bool CheckExpression::visit(NumericLiteralAST *)
{
    _fullySpecifiedType.setType(control()->integerType(IntegerType::Int));
    return false;
}

bool CheckExpression::visit(BoolLiteralAST *)
{
    _fullySpecifiedType.setType(control()->integerType(IntegerType::Bool));
    return false;
}

bool CheckExpression::visit(StringLiteralAST *)
{
    IntegerType *charTy = control()->integerType(IntegerType::Char);
    _fullySpecifiedType.setType(control()->pointerType(charTy));
    return false;
}

bool CheckExpression::visit(ThisExpressionAST *)
{
    return false;
}

bool CheckExpression::visit(NestedExpressionAST *ast)
{
    FullySpecifiedType exprTy = semantic()->check(ast->expression, _scope);
    return false;
}

bool CheckExpression::visit(ThrowExpressionAST *ast)
{
    FullySpecifiedType exprTy = semantic()->check(ast->expression, _scope);
    return false;
}

bool CheckExpression::visit(TypeIdAST *ast)
{
    FullySpecifiedType typeSpecTy = semantic()->check(ast->type_specifier, _scope);
    FullySpecifiedType declTy = semantic()->check(ast->declarator, typeSpecTy.qualifiedType(),
                                                  _scope);
    _fullySpecifiedType = declTy;
    return false;
}

bool CheckExpression::visit(UnaryExpressionAST *ast)
{
    FullySpecifiedType exprTy = semantic()->check(ast->expression, _scope);
    return false;
}

bool CheckExpression::visit(QtMethodAST *ast)
{
    Name *name = 0;
    Scope dummy;
    FullySpecifiedType methTy = semantic()->check(ast->declarator, FullySpecifiedType(),
                                                  &dummy, &name);
    Function *fty = methTy->asFunction();
    if (! fty)
        translationUnit()->warning(ast->firstToken(), "expected a function declarator");
    else {
        for (unsigned i = 0; i < fty->argumentCount(); ++i) {
            Symbol *arg = fty->argumentAt(i);
            if (arg->name())
                translationUnit()->warning(arg->sourceLocation(),
                                           "argument should be anonymous");
        }
    }
    return false;
}

bool CheckExpression::visit(CompoundLiteralAST *ast)
{
    /*FullySpecifiedType exprTy = */ semantic()->check(ast->type_id, _scope);
    /*FullySpecifiedType initTy = */ semantic()->check(ast->initializer, _scope);
    return false;
}

bool CheckExpression::visit(CallAST *ast)
{
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        FullySpecifiedType exprTy = semantic()->check(it->expression, _scope);
    }
    return false;
}

bool CheckExpression::visit(ArrayAccessAST *ast)
{
    FullySpecifiedType exprTy = semantic()->check(ast->expression, _scope);
    return false;
}

bool CheckExpression::visit(PostIncrDecrAST *)
{
    return false;
}

bool CheckExpression::visit(MemberAccessAST *ast)
{
    if (Name *name = semantic()->check(ast->member_name, _scope))
        _scope->addUse(ast->member_name->firstToken(), name);
    return false;
}

CPLUSPLUS_END_NAMESPACE
