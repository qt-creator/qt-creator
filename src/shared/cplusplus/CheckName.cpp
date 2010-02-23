/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
#include "Scope.h"
#include <cassert>

using namespace CPlusPlus;

CheckName::CheckName(Semantic *semantic)
    : SemanticCheck(semantic),
      _name(0),
      _scope(0)
{ }

CheckName::~CheckName()
{ }

const Name *CheckName::check(NameAST *name, Scope *scope)
{
    const Name *previousName = switchName(0);
    Scope *previousScope = switchScope(scope);
    accept(name);

    if (_name && name)
        name->name = _name;

    (void) switchScope(previousScope);
    return switchName(previousName);
}

const Name *CheckName::check(NestedNameSpecifierListAST *nested_name_specifier_list, Scope *scope)
{
    const Name *previousName = switchName(0);
    Scope *previousScope = switchScope(scope);

    std::vector<const Name *> names;
    for (NestedNameSpecifierListAST *it = nested_name_specifier_list; it; it = it->next) {
        NestedNameSpecifierAST *nested_name_specifier = it->value;
        names.push_back(semantic()->check(nested_name_specifier->class_or_namespace_name, _scope));
    }

    if (! names.empty())
        _name = control()->qualifiedNameId(&names[0], names.size());

    (void) switchScope(previousScope);
    return switchName(previousName);
}

void CheckName::check(ObjCMessageArgumentDeclarationAST *arg, Scope *scope)
{
    const Name *previousName = switchName(0);
    Scope *previousScope = switchScope(scope);

    accept(arg);

    (void) switchScope(previousScope);
    (void) switchName(previousName);
}

const Name *CheckName::switchName(const Name *name)
{
    const Name *previousName = _name;
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
    std::vector<const Name *> names;
    for (NestedNameSpecifierListAST *it = ast->nested_name_specifier_list; it; it = it->next) {
        NestedNameSpecifierAST *nested_name_specifier = it->value;
        names.push_back(semantic()->check(nested_name_specifier->class_or_namespace_name, _scope));
    }
    names.push_back(semantic()->check(ast->unqualified_name, _scope));
    _name = control()->qualifiedNameId(&names[0], names.size(), ast->global_scope_token != 0);

    ast->name = _name;
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
    ast->name = _name;
    return false;
}

bool CheckName::visit(ConversionFunctionIdAST *ast)
{
    FullySpecifiedType ty = semantic()->check(ast->type_specifier_list, _scope);
    ty = semantic()->check(ast->ptr_operator_list, ty, _scope);
    _name = control()->conversionNameId(ty);
    return false;
}

bool CheckName::visit(SimpleNameAST *ast)
{
    const Identifier *id = identifier(ast->identifier_token);
    _name = control()->nameId(id);
    ast->name = _name;
    return false;
}

bool CheckName::visit(DestructorNameAST *ast)
{
    const Identifier *id = identifier(ast->identifier_token);
    _name = control()->destructorNameId(id);
    ast->name = _name;
    return false;
}

bool CheckName::visit(TemplateIdAST *ast)
{
    const Identifier *id = identifier(ast->identifier_token);
    std::vector<FullySpecifiedType> templateArguments;
    for (TemplateArgumentListAST *it = ast->template_argument_list; it;
            it = it->next) {
        ExpressionAST *arg = it->value;
        FullySpecifiedType exprTy = semantic()->check(arg, _scope);
        templateArguments.push_back(exprTy);
    }
    if (templateArguments.empty())
        _name = control()->templateNameId(id);
    else
        _name = control()->templateNameId(id, &templateArguments[0],
                                          templateArguments.size());
    ast->name = _name;
    return false;
}

bool CheckName::visit(ObjCSelectorAST *ast)
{
    std::vector<const Name *> names;
    for (ObjCSelectorArgumentListAST *it = ast->selector_argument_list; it; it = it->next) {
        if (it->value->name_token) {
            const Identifier *id = control()->findOrInsertIdentifier(spell(it->value->name_token));
            const NameId *nameId = control()->nameId(id);
            names.push_back(nameId);
        } else {
            // we have an incomplete name due, probably due to error recovery. So, back out completely
            return false;
        }
    }

    if (!names.empty()) {
        _name = control()->selectorNameId(&names[0], names.size(), true);
        ast->name = _name;
    }

    return false;
}

bool CheckName::visit(ObjCMessageArgumentDeclarationAST *ast)
{
    FullySpecifiedType type;

    if (ast->type_name)
        type = semantic()->check(ast->type_name, _scope);

    if (ast->param_name) {
        accept(ast->param_name);

        Argument *arg = control()->newArgument(ast->param_name->firstToken(),
                                               ast->param_name->name);
        ast->argument = arg;
        arg->setType(type);
        arg->setInitializer(0);
        _scope->enterSymbol(arg);
    }

    return false;
}
