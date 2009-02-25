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

#include "CheckName.h"
#include "Semantic.h"
#include "AST.h"
#include "Control.h"
#include "TranslationUnit.h"
#include "Literals.h"
#include "Names.h"
#include "CoreTypes.h"
#include "Symbols.h"
#include <cassert>

CPLUSPLUS_BEGIN_NAMESPACE

CheckName::CheckName(Semantic *semantic)
    : SemanticCheck(semantic),
      _name(0),
      _scope(0)
{ }

CheckName::~CheckName()
{ }

Name *CheckName::check(NameAST *name, Scope *scope)
{
    Name *previousName = switchName(0);
    Scope *previousScope = switchScope(scope);
    accept(name);
    (void) switchScope(previousScope);
    return switchName(previousName);
}

Name *CheckName::check(NestedNameSpecifierAST *nested_name_specifier, Scope *scope)
{
    Name *previousName = switchName(0);
    Scope *previousScope = switchScope(scope);

    std::vector<Name *> names;
    for (NestedNameSpecifierAST *it = nested_name_specifier;
            it; it = it->next) {
        names.push_back(semantic()->check(it->class_or_namespace_name, _scope));
    }
    _name = control()->qualifiedNameId(&names[0], names.size());

    (void) switchScope(previousScope);
    return switchName(previousName);
}

Name *CheckName::switchName(Name *name)
{
    Name *previousName = _name;
    _name = name;
    return previousName;
}

Scope *CheckName::switchScope(Scope *scope)
{
    Scope *previousScope = _scope;
    _scope = scope;
    return previousScope;
}

bool CheckName::visit(QualifiedNameAST *ast)
{
    std::vector<Name *> names;
    for (NestedNameSpecifierAST *it = ast->nested_name_specifier;
            it; it = it->next) {
        names.push_back(semantic()->check(it->class_or_namespace_name, _scope));
    }
    names.push_back(semantic()->check(ast->unqualified_name, _scope));
    _name = control()->qualifiedNameId(&names[0], names.size(),
                                          ast->global_scope_token != 0);
    return false;
}

bool CheckName::visit(OperatorFunctionIdAST *ast)
{
    assert(ast->op != 0);

    OperatorNameId::Kind kind = OperatorNameId::InvalidOp;

    switch (tokenKind(ast->op->op_token)) {
    case T_NEW:
        if (ast->op->open_token)
            kind = OperatorNameId::NewArrayOp;
        else
            kind = OperatorNameId::NewOp;
        break;

    case T_DELETE:
        if (ast->op->open_token)
            kind = OperatorNameId::DeleteArrayOp;
        else
            kind = OperatorNameId::DeleteOp;
        break;

    case T_PLUS:
        kind = OperatorNameId::PlusOp;
        break;

    case T_MINUS:
        kind = OperatorNameId::MinusOp;
        break;

    case T_STAR:
        kind = OperatorNameId::StarOp;
        break;

    case T_SLASH:
        kind = OperatorNameId::SlashOp;
        break;

    case T_PERCENT:
        kind = OperatorNameId::PercentOp;
        break;

    case T_CARET:
        kind = OperatorNameId::CaretOp;
        break;

    case T_AMPER:
        kind = OperatorNameId::AmpOp;
        break;

    case T_PIPE:
        kind = OperatorNameId::PipeOp;
        break;

    case T_TILDE:
        kind = OperatorNameId::TildeOp;
        break;

    case T_EXCLAIM:
        kind = OperatorNameId::ExclaimOp;
        break;

    case T_EQUAL:
        kind = OperatorNameId::EqualOp;
        break;

    case T_LESS:
        kind = OperatorNameId::LessOp;
        break;

    case T_GREATER:
        kind = OperatorNameId::GreaterOp;
        break;

    case T_PLUS_EQUAL:
        kind = OperatorNameId::PlusEqualOp;
        break;

    case T_MINUS_EQUAL:
        kind = OperatorNameId::MinusEqualOp;
        break;

    case T_STAR_EQUAL:
        kind = OperatorNameId::StarEqualOp;
        break;

    case T_SLASH_EQUAL:
        kind = OperatorNameId::SlashEqualOp;
        break;

    case T_PERCENT_EQUAL:
        kind = OperatorNameId::PercentEqualOp;
        break;

    case T_CARET_EQUAL:
        kind = OperatorNameId::CaretEqualOp;
        break;

    case T_AMPER_EQUAL:
        kind = OperatorNameId::AmpEqualOp;
        break;

    case T_PIPE_EQUAL:
        kind = OperatorNameId::PipeEqualOp;
        break;

    case T_LESS_LESS:
        kind = OperatorNameId::LessLessOp;
        break;

    case T_GREATER_GREATER:
        kind = OperatorNameId::GreaterGreaterOp;
        break;

    case T_LESS_LESS_EQUAL:
        kind = OperatorNameId::LessLessEqualOp;
        break;

    case T_GREATER_GREATER_EQUAL:
        kind = OperatorNameId::GreaterGreaterEqualOp;
        break;

    case T_EQUAL_EQUAL:
        kind = OperatorNameId::EqualEqualOp;
        break;

    case T_EXCLAIM_EQUAL:
        kind = OperatorNameId::ExclaimEqualOp;
        break;

    case T_LESS_EQUAL:
        kind = OperatorNameId::LessEqualOp;
        break;

    case T_GREATER_EQUAL:
        kind = OperatorNameId::GreaterEqualOp;
        break;

    case T_AMPER_AMPER:
        kind = OperatorNameId::AmpAmpOp;
        break;

    case T_PIPE_PIPE:
        kind = OperatorNameId::PipePipeOp;
        break;

    case T_PLUS_PLUS:
        kind = OperatorNameId::PlusPlusOp;
        break;

    case T_MINUS_MINUS:
        kind = OperatorNameId::MinusMinusOp;
        break;

    case T_COMMA:
        kind = OperatorNameId::CommaOp;
        break;

    case T_ARROW_STAR:
        kind = OperatorNameId::ArrowStarOp;
        break;

    case T_ARROW:
        kind = OperatorNameId::ArrowOp;
        break;

    case T_LPAREN:
        kind = OperatorNameId::FunctionCallOp;
        break;

    case T_LBRACKET:
        kind = OperatorNameId::ArrayAccessOp;
        break;

    default:
        kind = OperatorNameId::InvalidOp;
    } // switch

    _name = control()->operatorNameId(kind);
    return false;
}

bool CheckName::visit(ConversionFunctionIdAST *ast)
{
    FullySpecifiedType ty = semantic()->check(ast->type_specifier, _scope);
    ty = semantic()->check(ast->ptr_operators, ty, _scope);
    _name = control()->conversionNameId(ty);
    return false;
}

bool CheckName::visit(SimpleNameAST *ast)
{
    Identifier *id = identifier(ast->identifier_token);
    _name = control()->nameId(id);
    return false;
}

bool CheckName::visit(DestructorNameAST *ast)
{
    Identifier *id = identifier(ast->identifier_token);
    _name = control()->destructorNameId(id);
    return false;
}

bool CheckName::visit(TemplateIdAST *ast)
{
    Identifier *id = identifier(ast->identifier_token);
    std::vector<FullySpecifiedType> templateArguments;
    for (TemplateArgumentListAST *it = ast->template_arguments; it;
            it = it->next) {
        ExpressionAST *arg = it->template_argument;
        FullySpecifiedType exprTy = semantic()->check(arg, _scope);
        templateArguments.push_back(exprTy);
    }
    if (templateArguments.empty())
        _name = control()->templateNameId(id);
    else
        _name = control()->templateNameId(id, &templateArguments[0],
                                          templateArguments.size());
    return false;
}

CPLUSPLUS_END_NAMESPACE
