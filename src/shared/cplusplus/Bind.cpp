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

#include "Bind.h"
#include "AST.h"
#include "TranslationUnit.h"
#include "Control.h"
#include "Names.h"
#include "Symbols.h"
#include "CoreTypes.h"
#include "Literals.h"
#include "Scope.h"
#include <memory>
#include <cassert>

using namespace CPlusPlus;

namespace { bool debug_todo = false; }

Bind::Bind(TranslationUnit *unit)
    : ASTVisitor(unit),
      _currentScope(0),
      _currentExpression(0),
      _currentName(0)
{
    if (unit->ast())
        translationUnit(unit->ast()->asTranslationUnit());
}

Scope *Bind::currentScope() const
{
    return _currentScope;
}

Scope *Bind::switchScope(Scope *scope)
{
    Scope *previousScope = _currentScope;
    _currentScope = scope;
    return previousScope;
}

void Bind::statement(StatementAST *ast)
{
    accept(ast);
}

Bind::ExpressionTy Bind::expression(ExpressionAST *ast)
{
    ExpressionTy value = ExpressionTy();
    std::swap(_currentExpression, value);
    accept(ast);
    std::swap(_currentExpression, value);
    return value;
}

void Bind::declaration(DeclarationAST *ast)
{
    accept(ast);
}

const Name *Bind::name(NameAST *ast)
{
    const Name *value = 0;
    std::swap(_currentName, value);
    accept(ast);
    std::swap(_currentName, value);
    return value;
}

FullySpecifiedType Bind::specifier(SpecifierAST *ast, const FullySpecifiedType &init)
{
    FullySpecifiedType value = init;
    std::swap(_currentType, value);
    accept(ast);
    std::swap(_currentType, value);
    return value;
}

FullySpecifiedType Bind::ptrOperator(PtrOperatorAST *ast, const FullySpecifiedType &init)
{
    FullySpecifiedType value = init;
    std::swap(_currentType, value);
    accept(ast);
    std::swap(_currentType, value);
    return value;
}

FullySpecifiedType Bind::coreDeclarator(CoreDeclaratorAST *ast, const FullySpecifiedType &init)
{
    FullySpecifiedType value = init;
    std::swap(_currentType, value);
    accept(ast);
    std::swap(_currentType, value);
    return value;
}

FullySpecifiedType Bind::postfixDeclarator(PostfixDeclaratorAST *ast, const FullySpecifiedType &init)
{
    FullySpecifiedType value = init;
    std::swap(_currentType, value);
    accept(ast);
    std::swap(_currentType, value);
    return value;
}

// AST
bool Bind::visit(ObjCSelectorArgumentAST *ast)
{
    (void) ast;
    assert(!"unreachable");
    return false;
}

void Bind::objCSelectorArgument(ObjCSelectorArgumentAST *ast)
{
    if (! ast)
        return;

    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned name_token = ast->name_token;
    // unsigned colon_token = ast->colon_token;
}

bool Bind::visit(AttributeAST *ast)
{
    (void) ast;
    assert(!"unreachable");
    return false;
}

void Bind::attribute(AttributeAST *ast)
{
    if (! ast)
        return;

    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned identifier_token = ast->identifier_token;
    // unsigned lparen_token = ast->lparen_token;
    // unsigned tag_token = ast->tag_token;
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        ExpressionTy value = this->expression(it->value);
    }
    // unsigned rparen_token = ast->rparen_token;
}

bool Bind::visit(DeclaratorAST *ast)
{
    (void) ast;
    assert(!"unreachable");
    return false;
}

FullySpecifiedType Bind::declarator(DeclaratorAST *ast, const FullySpecifiedType &init)
{
    FullySpecifiedType type = init;

    if (! ast)
        return type;

    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);

    for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    for (PtrOperatorListAST *it = ast->ptr_operator_list; it; it = it->next) {
        type = this->ptrOperator(it->value, type);
    }
    type = this->coreDeclarator(ast->core_declarator, type);
    for (PostfixDeclaratorListAST *it = ast->postfix_declarator_list; it; it = it->next) {
        type = this->postfixDeclarator(it->value, type);
    }
    for (SpecifierListAST *it = ast->post_attribute_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    // unsigned equals_token = ast->equals_token;
    ExpressionTy initializer = this->expression(ast->initializer);
    return init;
}

bool Bind::visit(QtPropertyDeclarationItemAST *ast)
{
    (void) ast;
    assert(!"unreachable");
    return false;
}

void Bind::qtPropertyDeclarationItem(QtPropertyDeclarationItemAST *ast)
{
    if (! ast)
        return;

    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned item_name_token = ast->item_name_token;
    ExpressionTy expression = this->expression(ast->expression);
}

bool Bind::visit(QtInterfaceNameAST *ast)
{
    (void) ast;
    assert(!"unreachable");
    return false;
}

void Bind::qtInterfaceName(QtInterfaceNameAST *ast)
{
    if (! ast)
        return;

    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    /*const Name *interface_name =*/ this->name(ast->interface_name);
    for (NameListAST *it = ast->constraint_list; it; it = it->next) {
        /*const Name *value =*/ this->name(it->value);
    }
}

bool Bind::visit(BaseSpecifierAST *ast)
{
    (void) ast;
    assert(!"unreachable");
    return false;
}

void Bind::baseSpecifier(BaseSpecifierAST *ast)
{
    if (! ast)
        return;

    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned virtual_token = ast->virtual_token;
    // unsigned access_specifier_token = ast->access_specifier_token;
    /*const Name *name =*/ this->name(ast->name);
    // BaseClass *symbol = ast->symbol;
}

bool Bind::visit(CtorInitializerAST *ast)
{
    (void) ast;
    assert(!"unreachable");
    return false;
}

void Bind::ctorInitializer(CtorInitializerAST *ast)
{
    if (! ast)
        return;

    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned colon_token = ast->colon_token;
    for (MemInitializerListAST *it = ast->member_initializer_list; it; it = it->next) {
        this->memInitializer(it->value);
    }
    // unsigned dot_dot_dot_token = ast->dot_dot_dot_token;
}

bool Bind::visit(EnumeratorAST *ast)
{
    (void) ast;
    assert(!"unreachable");
    return false;
}

void Bind::enumerator(EnumeratorAST *ast)
{
    if (! ast)
        return;

    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned identifier_token = ast->identifier_token;
    // unsigned equal_token = ast->equal_token;
    ExpressionTy expression = this->expression(ast->expression);
}

bool Bind::visit(ExceptionSpecificationAST *ast)
{
    (void) ast;
    assert(!"unreachable");
    return false;
}

FullySpecifiedType Bind::exceptionSpecification(ExceptionSpecificationAST *ast, const FullySpecifiedType &init)
{
    FullySpecifiedType type = init;

    if (! ast)
        return type;

    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned throw_token = ast->throw_token;
    // unsigned lparen_token = ast->lparen_token;
    // unsigned dot_dot_dot_token = ast->dot_dot_dot_token;
    for (ExpressionListAST *it = ast->type_id_list; it; it = it->next) {
        ExpressionTy value = this->expression(it->value);
    }
    // unsigned rparen_token = ast->rparen_token;
    return type;
}

bool Bind::visit(MemInitializerAST *ast)
{
    (void) ast;
    assert(!"unreachable");
    return false;
}

void Bind::memInitializer(MemInitializerAST *ast)
{
    if (! ast)
        return;

    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    /*const Name *name =*/ this->name(ast->name);
    // unsigned lparen_token = ast->lparen_token;
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        /*ExpressionTy value =*/ this->expression(it->value);
    }
    // unsigned rparen_token = ast->rparen_token;
}

bool Bind::visit(NestedNameSpecifierAST *ast)
{
    (void) ast;
    assert(!"unreachable");
    return false;
}

void Bind::nestedNameSpecifier(NestedNameSpecifierAST *ast)
{
    if (! ast)
        return;

    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    /*const Name *class_or_namespace_name =*/ this->name(ast->class_or_namespace_name);
    // unsigned scope_token = ast->scope_token;
}

bool Bind::visit(NewPlacementAST *ast)
{
    (void) ast;
    assert(!"unreachable");
    return false;
}

void Bind::newPlacement(NewPlacementAST *ast)
{
    if (! ast)
        return;

    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned lparen_token = ast->lparen_token;
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        ExpressionTy value = this->expression(it->value);
    }
    // unsigned rparen_token = ast->rparen_token;
}

bool Bind::visit(NewArrayDeclaratorAST *ast)
{
    (void) ast;
    assert(!"unreachable");
    return false;
}

FullySpecifiedType Bind::newArrayDeclarator(NewArrayDeclaratorAST *ast, const FullySpecifiedType &init)
{
    FullySpecifiedType type = init;

    if (! ast)
        return type;

    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned lbracket_token = ast->lbracket_token;
    ExpressionTy expression = this->expression(ast->expression);
    // unsigned rbracket_token = ast->rbracket_token;
    return type;
}

bool Bind::visit(NewInitializerAST *ast)
{
    (void) ast;
    assert(!"unreachable");
    return false;
}

void Bind::newInitializer(NewInitializerAST *ast)
{
    if (! ast)
        return;

    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned lparen_token = ast->lparen_token;
    ExpressionTy expression = this->expression(ast->expression);
    // unsigned rparen_token = ast->rparen_token;
}

bool Bind::visit(NewTypeIdAST *ast)
{
    (void) ast;
    assert(!"unreachable");
    return false;
}

FullySpecifiedType Bind::newTypeId(NewTypeIdAST *ast)
{
    FullySpecifiedType type;

    if (! ast)
        return type;

    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);

    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    for (PtrOperatorListAST *it = ast->ptr_operator_list; it; it = it->next) {
        type = this->ptrOperator(it->value, type);
    }
    for (NewArrayDeclaratorListAST *it = ast->new_array_declarator_list; it; it = it->next) {
        type = this->newArrayDeclarator(it->value, type);
    }
    return type;
}

bool Bind::visit(OperatorAST *ast)
{
    (void) ast;
    assert(!"unreachable");
    return false;
}

void Bind::cppOperator(OperatorAST *ast)
{
    if (! ast)
        return;

    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned op_token = ast->op_token;
    // unsigned open_token = ast->open_token;
    // unsigned close_token = ast->close_token;
}

bool Bind::visit(ParameterDeclarationClauseAST *ast)
{
    (void) ast;
    assert(!"unreachable");
    return false;
}

void Bind::parameterDeclarationClause(ParameterDeclarationClauseAST *ast)
{
    if (! ast)
        return;

    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    for (DeclarationListAST *it = ast->parameter_declaration_list; it; it = it->next) {
        this->declaration(it->value);
    }
    // unsigned dot_dot_dot_token = ast->dot_dot_dot_token;
}

bool Bind::visit(TranslationUnitAST *ast)
{
    (void) ast;
    assert(!"unreachable");
    return false;
}

void Bind::translationUnit(TranslationUnitAST *ast)
{
    if (! ast)
        return;

    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    for (DeclarationListAST *it = ast->declaration_list; it; it = it->next) {
        this->declaration(it->value);
    }
}

bool Bind::visit(ObjCProtocolRefsAST *ast)
{
    (void) ast;
    assert(!"unreachable");
    return false;
}

void Bind::objCProtocolRefs(ObjCProtocolRefsAST *ast)
{
    if (! ast)
        return;

    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned less_token = ast->less_token;
    for (NameListAST *it = ast->identifier_list; it; it = it->next) {
        /*const Name *value =*/ this->name(it->value);
    }
    // unsigned greater_token = ast->greater_token;
}

bool Bind::visit(ObjCMessageArgumentAST *ast)
{
    (void) ast;
    assert(!"unreachable");
    return false;
}

void Bind::objCMessageArgument(ObjCMessageArgumentAST *ast)
{
    if (! ast)
        return;

    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    ExpressionTy parameter_value_expression = this->expression(ast->parameter_value_expression);
}

bool Bind::visit(ObjCTypeNameAST *ast)
{
    (void) ast;
    assert(!"unreachable");
    return false;
}

void Bind::objCTypeName(ObjCTypeNameAST *ast)
{
    if (! ast)
        return;

    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned lparen_token = ast->lparen_token;
    // unsigned type_qualifier_token = ast->type_qualifier_token;
    ExpressionTy type_id = this->expression(ast->type_id);
    // unsigned rparen_token = ast->rparen_token;
}

bool Bind::visit(ObjCInstanceVariablesDeclarationAST *ast)
{
    (void) ast;
    assert(!"unreachable");
    return false;
}

void Bind::objCInstanceVariablesDeclaration(ObjCInstanceVariablesDeclarationAST *ast)
{
    if (! ast)
        return;

    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned lbrace_token = ast->lbrace_token;
    for (DeclarationListAST *it = ast->instance_variable_list; it; it = it->next) {
        this->declaration(it->value);
    }
    // unsigned rbrace_token = ast->rbrace_token;
}

bool Bind::visit(ObjCPropertyAttributeAST *ast)
{
    (void) ast;
    assert(!"unreachable");
    return false;
}

void Bind::objCPropertyAttribute(ObjCPropertyAttributeAST *ast)
{
    if (! ast)
        return;

    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned attribute_identifier_token = ast->attribute_identifier_token;
    // unsigned equals_token = ast->equals_token;
    /*const Name *method_selector =*/ this->name(ast->method_selector);
}

bool Bind::visit(ObjCMessageArgumentDeclarationAST *ast)
{
    (void) ast;
    assert(!"unreachable");
    return false;
}

void Bind::objCMessageArgumentDeclaration(ObjCMessageArgumentDeclarationAST *ast)
{
    if (! ast)
        return;

    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    this->objCTypeName(ast->type_name);
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    /*const Name *param_name =*/ this->name(ast->param_name);
    // Argument *argument = ast->argument;
}

bool Bind::visit(ObjCMethodPrototypeAST *ast)
{
    (void) ast;
    assert(!"unreachable");
    return false;
}

void Bind::objCMethodPrototype(ObjCMethodPrototypeAST *ast)
{
    if (! ast)
        return;

    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned method_type_token = ast->method_type_token;
    this->objCTypeName(ast->type_name);
    /*const Name *selector =*/ this->name(ast->selector);
    for (ObjCMessageArgumentDeclarationListAST *it = ast->argument_list; it; it = it->next) {
        this->objCMessageArgumentDeclaration(it->value);
    }
    // unsigned dot_dot_dot_token = ast->dot_dot_dot_token;
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    // ObjCMethod *symbol = ast->symbol;
}

bool Bind::visit(ObjCSynthesizedPropertyAST *ast)
{
    (void) ast;
    assert(!"unreachable");
    return false;
}

void Bind::objCSynthesizedProperty(ObjCSynthesizedPropertyAST *ast)
{
    if (! ast)
        return;

    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned property_identifier_token = ast->property_identifier_token;
    // unsigned equals_token = ast->equals_token;
    // unsigned alias_identifier_token = ast->alias_identifier_token;
}

bool Bind::visit(LambdaIntroducerAST *ast)
{
    (void) ast;
    assert(!"unreachable");
    return false;
}

void Bind::lambdaIntroducer(LambdaIntroducerAST *ast)
{
    if (! ast)
        return;

    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned lbracket_token = ast->lbracket_token;
    this->lambdaCapture(ast->lambda_capture);
    // unsigned rbracket_token = ast->rbracket_token;
}

bool Bind::visit(LambdaCaptureAST *ast)
{
    (void) ast;
    assert(!"unreachable");
    return false;
}

void Bind::lambdaCapture(LambdaCaptureAST *ast)
{
    if (! ast)
        return;

    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned default_capture_token = ast->default_capture_token;
    for (CaptureListAST *it = ast->capture_list; it; it = it->next) {
        this->capture(it->value);
    }
}

bool Bind::visit(CaptureAST *ast)
{
    (void) ast;
    assert(!"unreachable");
    return false;
}

void Bind::capture(CaptureAST *ast)
{
    if (! ast)
        return;

    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
}

bool Bind::visit(LambdaDeclaratorAST *ast)
{
    (void) ast;
    assert(!"unreachable");
    return false;
}

void Bind::lambdaDeclarator(LambdaDeclaratorAST *ast)
{
    if (! ast)
        return;

    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned lparen_token = ast->lparen_token;
    FullySpecifiedType type;
    this->parameterDeclarationClause(ast->parameter_declaration_clause);
    // unsigned rparen_token = ast->rparen_token;
    for (SpecifierListAST *it = ast->attributes; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    // unsigned mutable_token = ast->mutable_token;
    type = this->exceptionSpecification(ast->exception_specification, type);
    type = this->trailingReturnType(ast->trailing_return_type, type);
}

bool Bind::visit(TrailingReturnTypeAST *ast)
{
    (void) ast;
    assert(!"unreachable");
    return false;
}

FullySpecifiedType Bind::trailingReturnType(TrailingReturnTypeAST *ast, const FullySpecifiedType &init)
{
    FullySpecifiedType type = init;

    if (! ast)
        return type;

    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned arrow_token = ast->arrow_token;
    for (SpecifierListAST *it = ast->attributes; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    for (SpecifierListAST *it = ast->type_specifiers; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    type = this->declarator(ast->declarator, type);
    return type;
}


// StatementAST
bool Bind::visit(QtMemberDeclarationAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned q_token = ast->q_token;
    // unsigned lparen_token = ast->lparen_token;
    ExpressionTy type_id = this->expression(ast->type_id);
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool Bind::visit(CaseStatementAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned case_token = ast->case_token;
    ExpressionTy expression = this->expression(ast->expression);
    // unsigned colon_token = ast->colon_token;
    this->statement(ast->statement);
    return false;
}

bool Bind::visit(CompoundStatementAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned lbrace_token = ast->lbrace_token;
    for (StatementListAST *it = ast->statement_list; it; it = it->next) {
        this->statement(it->value);
    }
    // unsigned rbrace_token = ast->rbrace_token;
    // Block *symbol = ast->symbol;
    return false;
}

bool Bind::visit(DeclarationStatementAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    this->declaration(ast->declaration);
    return false;
}

bool Bind::visit(DoStatementAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned do_token = ast->do_token;
    this->statement(ast->statement);
    // unsigned while_token = ast->while_token;
    // unsigned lparen_token = ast->lparen_token;
    ExpressionTy expression = this->expression(ast->expression);
    // unsigned rparen_token = ast->rparen_token;
    // unsigned semicolon_token = ast->semicolon_token;
    return false;
}

bool Bind::visit(ExpressionOrDeclarationStatementAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    this->statement(ast->expression);
    this->statement(ast->declaration);
    return false;
}

bool Bind::visit(ExpressionStatementAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    ExpressionTy expression = this->expression(ast->expression);
    // unsigned semicolon_token = ast->semicolon_token;
    return false;
}

bool Bind::visit(ForeachStatementAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned foreach_token = ast->foreach_token;
    // unsigned lparen_token = ast->lparen_token;
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    type = this->declarator(ast->declarator, type);
    ExpressionTy initializer = this->expression(ast->initializer);
    // unsigned comma_token = ast->comma_token;
    ExpressionTy expression = this->expression(ast->expression);
    // unsigned rparen_token = ast->rparen_token;
    this->statement(ast->statement);
    // Block *symbol = ast->symbol;
    return false;
}

bool Bind::visit(ForStatementAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned for_token = ast->for_token;
    // unsigned lparen_token = ast->lparen_token;
    this->statement(ast->initializer);
    ExpressionTy condition = this->expression(ast->condition);
    // unsigned semicolon_token = ast->semicolon_token;
    ExpressionTy expression = this->expression(ast->expression);
    // unsigned rparen_token = ast->rparen_token;
    this->statement(ast->statement);
    // Block *symbol = ast->symbol;
    return false;
}

bool Bind::visit(IfStatementAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned if_token = ast->if_token;
    // unsigned lparen_token = ast->lparen_token;
    ExpressionTy condition = this->expression(ast->condition);
    // unsigned rparen_token = ast->rparen_token;
    this->statement(ast->statement);
    // unsigned else_token = ast->else_token;
    this->statement(ast->else_statement);
    // Block *symbol = ast->symbol;
    return false;
}

bool Bind::visit(LabeledStatementAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned label_token = ast->label_token;
    // unsigned colon_token = ast->colon_token;
    this->statement(ast->statement);
    return false;
}

bool Bind::visit(BreakStatementAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned break_token = ast->break_token;
    // unsigned semicolon_token = ast->semicolon_token;
    return false;
}

bool Bind::visit(ContinueStatementAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned continue_token = ast->continue_token;
    // unsigned semicolon_token = ast->semicolon_token;
    return false;
}

bool Bind::visit(GotoStatementAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned goto_token = ast->goto_token;
    // unsigned identifier_token = ast->identifier_token;
    // unsigned semicolon_token = ast->semicolon_token;
    return false;
}

bool Bind::visit(ReturnStatementAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned return_token = ast->return_token;
    ExpressionTy expression = this->expression(ast->expression);
    // unsigned semicolon_token = ast->semicolon_token;
    return false;
}

bool Bind::visit(SwitchStatementAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned switch_token = ast->switch_token;
    // unsigned lparen_token = ast->lparen_token;
    ExpressionTy condition = this->expression(ast->condition);
    // unsigned rparen_token = ast->rparen_token;
    this->statement(ast->statement);
    // Block *symbol = ast->symbol;
    return false;
}

bool Bind::visit(TryBlockStatementAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned try_token = ast->try_token;
    this->statement(ast->statement);
    for (CatchClauseListAST *it = ast->catch_clause_list; it; it = it->next) {
        this->statement(it->value);
    }
    return false;
}

bool Bind::visit(CatchClauseAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned catch_token = ast->catch_token;
    // unsigned lparen_token = ast->lparen_token;
    this->declaration(ast->exception_declaration);
    // unsigned rparen_token = ast->rparen_token;
    this->statement(ast->statement);
    // Block *symbol = ast->symbol;
    return false;
}

bool Bind::visit(WhileStatementAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned while_token = ast->while_token;
    // unsigned lparen_token = ast->lparen_token;
    ExpressionTy condition = this->expression(ast->condition);
    // unsigned rparen_token = ast->rparen_token;
    this->statement(ast->statement);
    // Block *symbol = ast->symbol;
    return false;
}

bool Bind::visit(ObjCFastEnumerationAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned for_token = ast->for_token;
    // unsigned lparen_token = ast->lparen_token;
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    type = this->declarator(ast->declarator, type);
    ExpressionTy initializer = this->expression(ast->initializer);
    // unsigned in_token = ast->in_token;
    ExpressionTy fast_enumeratable_expression = this->expression(ast->fast_enumeratable_expression);
    // unsigned rparen_token = ast->rparen_token;
    this->statement(ast->statement);
    // Block *symbol = ast->symbol;
    return false;
}

bool Bind::visit(ObjCSynchronizedStatementAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned synchronized_token = ast->synchronized_token;
    // unsigned lparen_token = ast->lparen_token;
    ExpressionTy synchronized_object = this->expression(ast->synchronized_object);
    // unsigned rparen_token = ast->rparen_token;
    this->statement(ast->statement);
    return false;
}


// ExpressionAST
bool Bind::visit(IdExpressionAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    /*const Name *name =*/ this->name(ast->name);
    return false;
}

bool Bind::visit(CompoundExpressionAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned lparen_token = ast->lparen_token;
    this->statement(ast->statement);
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool Bind::visit(CompoundLiteralAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned lparen_token = ast->lparen_token;
    ExpressionTy type_id = this->expression(ast->type_id);
    // unsigned rparen_token = ast->rparen_token;
    ExpressionTy initializer = this->expression(ast->initializer);
    return false;
}

bool Bind::visit(QtMethodAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned method_token = ast->method_token;
    // unsigned lparen_token = ast->lparen_token;
    FullySpecifiedType type;
    type = this->declarator(ast->declarator, type);
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool Bind::visit(BinaryExpressionAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    ExpressionTy left_expression = this->expression(ast->left_expression);
    // unsigned binary_op_token = ast->binary_op_token;
    ExpressionTy right_expression = this->expression(ast->right_expression);
    return false;
}

bool Bind::visit(CastExpressionAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned lparen_token = ast->lparen_token;
    ExpressionTy type_id = this->expression(ast->type_id);
    // unsigned rparen_token = ast->rparen_token;
    ExpressionTy expression = this->expression(ast->expression);
    return false;
}

bool Bind::visit(ConditionAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    type = this->declarator(ast->declarator, type);
    return false;
}

bool Bind::visit(ConditionalExpressionAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    ExpressionTy condition = this->expression(ast->condition);
    // unsigned question_token = ast->question_token;
    ExpressionTy left_expression = this->expression(ast->left_expression);
    // unsigned colon_token = ast->colon_token;
    ExpressionTy right_expression = this->expression(ast->right_expression);
    return false;
}

bool Bind::visit(CppCastExpressionAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned cast_token = ast->cast_token;
    // unsigned less_token = ast->less_token;
    ExpressionTy type_id = this->expression(ast->type_id);
    // unsigned greater_token = ast->greater_token;
    // unsigned lparen_token = ast->lparen_token;
    ExpressionTy expression = this->expression(ast->expression);
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool Bind::visit(DeleteExpressionAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned scope_token = ast->scope_token;
    // unsigned delete_token = ast->delete_token;
    // unsigned lbracket_token = ast->lbracket_token;
    // unsigned rbracket_token = ast->rbracket_token;
    ExpressionTy expression = this->expression(ast->expression);
    return false;
}

bool Bind::visit(ArrayInitializerAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned lbrace_token = ast->lbrace_token;
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        ExpressionTy value = this->expression(it->value);
    }
    // unsigned rbrace_token = ast->rbrace_token;
    return false;
}

bool Bind::visit(NewExpressionAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned scope_token = ast->scope_token;
    // unsigned new_token = ast->new_token;
    this->newPlacement(ast->new_placement);
    // unsigned lparen_token = ast->lparen_token;
    ExpressionTy type_id = this->expression(ast->type_id);
    // unsigned rparen_token = ast->rparen_token;
    this->newTypeId(ast->new_type_id);
    this->newInitializer(ast->new_initializer);
    return false;
}

bool Bind::visit(TypeidExpressionAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned typeid_token = ast->typeid_token;
    // unsigned lparen_token = ast->lparen_token;
    ExpressionTy expression = this->expression(ast->expression);
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool Bind::visit(TypenameCallExpressionAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned typename_token = ast->typename_token;
    /*const Name *name =*/ this->name(ast->name);
    // unsigned lparen_token = ast->lparen_token;
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        ExpressionTy value = this->expression(it->value);
    }
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool Bind::visit(TypeConstructorCallAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    // unsigned lparen_token = ast->lparen_token;
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        ExpressionTy value = this->expression(it->value);
    }
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool Bind::visit(SizeofExpressionAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned sizeof_token = ast->sizeof_token;
    // unsigned dot_dot_dot_token = ast->dot_dot_dot_token;
    // unsigned lparen_token = ast->lparen_token;
    ExpressionTy expression = this->expression(ast->expression);
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool Bind::visit(NumericLiteralAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned literal_token = ast->literal_token;
    return false;
}

bool Bind::visit(BoolLiteralAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned literal_token = ast->literal_token;
    return false;
}

bool Bind::visit(ThisExpressionAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned this_token = ast->this_token;
    return false;
}

bool Bind::visit(NestedExpressionAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned lparen_token = ast->lparen_token;
    ExpressionTy expression = this->expression(ast->expression);
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool Bind::visit(StringLiteralAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned literal_token = ast->literal_token;
    ExpressionTy next = this->expression(ast->next);
    return false;
}

bool Bind::visit(ThrowExpressionAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned throw_token = ast->throw_token;
    ExpressionTy expression = this->expression(ast->expression);
    return false;
}

bool Bind::visit(TypeIdAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    type = this->declarator(ast->declarator, type);
    return false;
}

bool Bind::visit(UnaryExpressionAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned unary_op_token = ast->unary_op_token;
    ExpressionTy expression = this->expression(ast->expression);
    return false;
}

bool Bind::visit(ObjCMessageExpressionAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned lbracket_token = ast->lbracket_token;
    /*ExpressionTy receiver_expression =*/ this->expression(ast->receiver_expression);
    /*const Name *selector =*/ this->name(ast->selector);
    for (ObjCMessageArgumentListAST *it = ast->argument_list; it; it = it->next) {
        this->objCMessageArgument(it->value);
    }
    // unsigned rbracket_token = ast->rbracket_token;
    return false;
}

bool Bind::visit(ObjCProtocolExpressionAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned protocol_token = ast->protocol_token;
    // unsigned lparen_token = ast->lparen_token;
    // unsigned identifier_token = ast->identifier_token;
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool Bind::visit(ObjCEncodeExpressionAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned encode_token = ast->encode_token;
    this->objCTypeName(ast->type_name);
    return false;
}

bool Bind::visit(ObjCSelectorExpressionAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned selector_token = ast->selector_token;
    // unsigned lparen_token = ast->lparen_token;
    /*const Name *selector =*/ this->name(ast->selector);
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool Bind::visit(LambdaExpressionAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    this->lambdaIntroducer(ast->lambda_introducer);
    this->lambdaDeclarator(ast->lambda_declarator);
    this->statement(ast->statement);
    return false;
}

bool Bind::visit(BracedInitializerAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned lbrace_token = ast->lbrace_token;
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        ExpressionTy value = this->expression(it->value);
    }
    // unsigned comma_token = ast->comma_token;
    // unsigned rbrace_token = ast->rbrace_token;
    return false;
}


// DeclarationAST
bool Bind::visit(SimpleDeclarationAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned qt_invokable_token = ast->qt_invokable_token;
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->decl_specifier_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    for (DeclaratorListAST *it = ast->declarator_list; it; it = it->next) {
        FullySpecifiedType declTy = this->declarator(it->value, type);
    }
    // unsigned semicolon_token = ast->semicolon_token;
    // List<Declaration *> *symbols = ast->symbols;
    return false;
}

bool Bind::visit(EmptyDeclarationAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned semicolon_token = ast->semicolon_token;
    return false;
}

bool Bind::visit(AccessDeclarationAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned access_specifier_token = ast->access_specifier_token;
    // unsigned slots_token = ast->slots_token;
    // unsigned colon_token = ast->colon_token;
    return false;
}

bool Bind::visit(QtObjectTagAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned q_object_token = ast->q_object_token;
    return false;
}

bool Bind::visit(QtPrivateSlotAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned q_private_slot_token = ast->q_private_slot_token;
    // unsigned lparen_token = ast->lparen_token;
    // unsigned dptr_token = ast->dptr_token;
    // unsigned dptr_lparen_token = ast->dptr_lparen_token;
    // unsigned dptr_rparen_token = ast->dptr_rparen_token;
    // unsigned comma_token = ast->comma_token;
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->type_specifiers; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    type = this->declarator(ast->declarator, type);
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool Bind::visit(QtPropertyDeclarationAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned property_specifier_token = ast->property_specifier_token;
    // unsigned lparen_token = ast->lparen_token;
    ExpressionTy type_id = this->expression(ast->type_id);
    /*const Name *property_name =*/ this->name(ast->property_name);
    for (QtPropertyDeclarationItemListAST *it = ast->property_declaration_items; it; it = it->next) {
        this->qtPropertyDeclarationItem(it->value);
    }
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool Bind::visit(QtEnumDeclarationAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned enum_specifier_token = ast->enum_specifier_token;
    // unsigned lparen_token = ast->lparen_token;
    for (NameListAST *it = ast->enumerator_list; it; it = it->next) {
        /*const Name *value =*/ this->name(it->value);
    }
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool Bind::visit(QtFlagsDeclarationAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned flags_specifier_token = ast->flags_specifier_token;
    // unsigned lparen_token = ast->lparen_token;
    for (NameListAST *it = ast->flag_enums_list; it; it = it->next) {
        /*const Name *value =*/ this->name(it->value);
    }
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool Bind::visit(QtInterfacesDeclarationAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned interfaces_token = ast->interfaces_token;
    // unsigned lparen_token = ast->lparen_token;
    for (QtInterfaceNameListAST *it = ast->interface_name_list; it; it = it->next) {
        this->qtInterfaceName(it->value);
    }
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool Bind::visit(AsmDefinitionAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned asm_token = ast->asm_token;
    // unsigned volatile_token = ast->volatile_token;
    // unsigned lparen_token = ast->lparen_token;
    // unsigned rparen_token = ast->rparen_token;
    // unsigned semicolon_token = ast->semicolon_token;
    return false;
}

bool Bind::visit(ExceptionDeclarationAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    type = this->declarator(ast->declarator, type);
    // unsigned dot_dot_dot_token = ast->dot_dot_dot_token;
    return false;
}

bool Bind::visit(FunctionDefinitionAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned qt_invokable_token = ast->qt_invokable_token;
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->decl_specifier_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    type = this->declarator(ast->declarator, type);
    this->ctorInitializer(ast->ctor_initializer);
    this->statement(ast->function_body);
    // Function *symbol = ast->symbol;
    return false;
}

bool Bind::visit(LinkageBodyAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned lbrace_token = ast->lbrace_token;
    for (DeclarationListAST *it = ast->declaration_list; it; it = it->next) {
        this->declaration(it->value);
    }
    // unsigned rbrace_token = ast->rbrace_token;
    return false;
}

bool Bind::visit(LinkageSpecificationAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned extern_token = ast->extern_token;
    // unsigned extern_type_token = ast->extern_type_token;
    this->declaration(ast->declaration);
    return false;
}

bool Bind::visit(NamespaceAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned namespace_token = ast->namespace_token;
    // unsigned identifier_token = ast->identifier_token;
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    this->declaration(ast->linkage_body);
    // Namespace *symbol = ast->symbol;
    return false;
}

bool Bind::visit(NamespaceAliasDefinitionAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned namespace_token = ast->namespace_token;
    // unsigned namespace_name_token = ast->namespace_name_token;
    // unsigned equal_token = ast->equal_token;
    /*const Name *name =*/ this->name(ast->name);
    // unsigned semicolon_token = ast->semicolon_token;
    return false;
}

bool Bind::visit(ParameterDeclarationAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    type = this->declarator(ast->declarator, type);
    // unsigned equal_token = ast->equal_token;
    ExpressionTy expression = this->expression(ast->expression);
    // Argument *symbol = ast->symbol;
    return false;
}

bool Bind::visit(TemplateDeclarationAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned export_token = ast->export_token;
    // unsigned template_token = ast->template_token;
    // unsigned less_token = ast->less_token;
    for (DeclarationListAST *it = ast->template_parameter_list; it; it = it->next) {
        this->declaration(it->value);
    }
    // unsigned greater_token = ast->greater_token;
    this->declaration(ast->declaration);
    // Template *symbol = ast->symbol;
    return false;
}

bool Bind::visit(TypenameTypeParameterAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned classkey_token = ast->classkey_token;
    // unsigned dot_dot_dot_token = ast->dot_dot_dot_token;
    /*const Name *name =*/ this->name(ast->name);
    // unsigned equal_token = ast->equal_token;
    ExpressionTy type_id = this->expression(ast->type_id);
    // TypenameArgument *symbol = ast->symbol;
    return false;
}

bool Bind::visit(TemplateTypeParameterAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned template_token = ast->template_token;
    // unsigned less_token = ast->less_token;
    for (DeclarationListAST *it = ast->template_parameter_list; it; it = it->next) {
        this->declaration(it->value);
    }
    // unsigned greater_token = ast->greater_token;
    // unsigned class_token = ast->class_token;
    // unsigned dot_dot_dot_token = ast->dot_dot_dot_token;
    /*const Name *name =*/ this->name(ast->name);
    // unsigned equal_token = ast->equal_token;
    ExpressionTy type_id = this->expression(ast->type_id);
    // TypenameArgument *symbol = ast->symbol;
    return false;
}

bool Bind::visit(UsingAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned using_token = ast->using_token;
    // unsigned typename_token = ast->typename_token;
    /*const Name *name =*/ this->name(ast->name);
    // unsigned semicolon_token = ast->semicolon_token;
    // UsingDeclaration *symbol = ast->symbol;
    return false;
}

bool Bind::visit(UsingDirectiveAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned using_token = ast->using_token;
    // unsigned namespace_token = ast->namespace_token;
    /*const Name *name =*/ this->name(ast->name);
    // unsigned semicolon_token = ast->semicolon_token;
    // UsingNamespaceDirective *symbol = ast->symbol;
    return false;
}

bool Bind::visit(ObjCClassForwardDeclarationAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    // unsigned class_token = ast->class_token;
    for (NameListAST *it = ast->identifier_list; it; it = it->next) {
        /*const Name *value =*/ this->name(it->value);
    }
    // unsigned semicolon_token = ast->semicolon_token;
    // List<ObjCForwardClassDeclaration *> *symbols = ast->symbols;
    return false;
}

bool Bind::visit(ObjCClassDeclarationAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    // unsigned interface_token = ast->interface_token;
    // unsigned implementation_token = ast->implementation_token;
    /*const Name *class_name =*/ this->name(ast->class_name);
    // unsigned lparen_token = ast->lparen_token;
    /*const Name *category_name =*/ this->name(ast->category_name);
    // unsigned rparen_token = ast->rparen_token;
    // unsigned colon_token = ast->colon_token;
    /*const Name *superclass =*/ this->name(ast->superclass);
    this->objCProtocolRefs(ast->protocol_refs);
    this->objCInstanceVariablesDeclaration(ast->inst_vars_decl);
    for (DeclarationListAST *it = ast->member_declaration_list; it; it = it->next) {
        this->declaration(it->value);
    }
    // unsigned end_token = ast->end_token;
    // ObjCClass *symbol = ast->symbol;
    return false;
}

bool Bind::visit(ObjCProtocolForwardDeclarationAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    // unsigned protocol_token = ast->protocol_token;
    for (NameListAST *it = ast->identifier_list; it; it = it->next) {
        /*const Name *value =*/ this->name(it->value);
    }
    // unsigned semicolon_token = ast->semicolon_token;
    // List<ObjCForwardProtocolDeclaration *> *symbols = ast->symbols;
    return false;
}

bool Bind::visit(ObjCProtocolDeclarationAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    // unsigned protocol_token = ast->protocol_token;
    /*const Name *name =*/ this->name(ast->name);
    this->objCProtocolRefs(ast->protocol_refs);
    for (DeclarationListAST *it = ast->member_declaration_list; it; it = it->next) {
        this->declaration(it->value);
    }
    // unsigned end_token = ast->end_token;
    // ObjCProtocol *symbol = ast->symbol;
    return false;
}

bool Bind::visit(ObjCVisibilityDeclarationAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned visibility_token = ast->visibility_token;
    return false;
}

bool Bind::visit(ObjCPropertyDeclarationAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    // unsigned property_token = ast->property_token;
    // unsigned lparen_token = ast->lparen_token;
    for (ObjCPropertyAttributeListAST *it = ast->property_attribute_list; it; it = it->next) {
        this->objCPropertyAttribute(it->value);
    }
    // unsigned rparen_token = ast->rparen_token;
    this->declaration(ast->simple_declaration);
    // List<ObjCPropertyDeclaration *> *symbols = ast->symbols;
    return false;
}

bool Bind::visit(ObjCMethodDeclarationAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    this->objCMethodPrototype(ast->method_prototype);
    this->statement(ast->function_body);
    // unsigned semicolon_token = ast->semicolon_token;
    return false;
}

bool Bind::visit(ObjCSynthesizedPropertiesDeclarationAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned synthesized_token = ast->synthesized_token;
    for (ObjCSynthesizedPropertyListAST *it = ast->property_identifier_list; it; it = it->next) {
        this->objCSynthesizedProperty(it->value);
    }
    // unsigned semicolon_token = ast->semicolon_token;
    return false;
}

bool Bind::visit(ObjCDynamicPropertiesDeclarationAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned dynamic_token = ast->dynamic_token;
    for (NameListAST *it = ast->property_identifier_list; it; it = it->next) {
        /*const Name *value =*/ this->name(it->value);
    }
    // unsigned semicolon_token = ast->semicolon_token;
    return false;
}


// NameAST
bool Bind::visit(ObjCSelectorAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    for (ObjCSelectorArgumentListAST *it = ast->selector_argument_list; it; it = it->next) {
        this->objCSelectorArgument(it->value);
    }
    return false;
}

bool Bind::visit(QualifiedNameAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned global_scope_token = ast->global_scope_token;
    for (NestedNameSpecifierListAST *it = ast->nested_name_specifier_list; it; it = it->next) {
        this->nestedNameSpecifier(it->value);
    }
    /*const Name *unqualified_name =*/ this->name(ast->unqualified_name);
    return false;
}

bool Bind::visit(OperatorFunctionIdAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned operator_token = ast->operator_token;
    this->cppOperator(ast->op);
    return false;
}

bool Bind::visit(ConversionFunctionIdAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned operator_token = ast->operator_token;
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    for (PtrOperatorListAST *it = ast->ptr_operator_list; it; it = it->next) {
        type = this->ptrOperator(it->value, type);
    }
    return false;
}

bool Bind::visit(SimpleNameAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned identifier_token = ast->identifier_token;
    return false;
}

bool Bind::visit(DestructorNameAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned tilde_token = ast->tilde_token;
    // unsigned identifier_token = ast->identifier_token;
    return false;
}

bool Bind::visit(TemplateIdAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned template_token = ast->template_token;
    // unsigned identifier_token = ast->identifier_token;
    // unsigned less_token = ast->less_token;
    for (ExpressionListAST *it = ast->template_argument_list; it; it = it->next) {
        ExpressionTy value = this->expression(it->value);
    }
    // unsigned greater_token = ast->greater_token;
    return false;
}


// SpecifierAST
bool Bind::visit(SimpleSpecifierAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned specifier_token = ast->specifier_token;
    return false;
}

bool Bind::visit(AttributeSpecifierAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned attribute_token = ast->attribute_token;
    // unsigned first_lparen_token = ast->first_lparen_token;
    // unsigned second_lparen_token = ast->second_lparen_token;
    for (AttributeListAST *it = ast->attribute_list; it; it = it->next) {
        this->attribute(it->value);
    }
    // unsigned first_rparen_token = ast->first_rparen_token;
    // unsigned second_rparen_token = ast->second_rparen_token;
    return false;
}

bool Bind::visit(TypeofSpecifierAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned typeof_token = ast->typeof_token;
    // unsigned lparen_token = ast->lparen_token;
    ExpressionTy expression = this->expression(ast->expression);
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool Bind::visit(ClassSpecifierAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned classkey_token = ast->classkey_token;
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    /*const Name *name =*/ this->name(ast->name);
    // unsigned colon_token = ast->colon_token;
    for (BaseSpecifierListAST *it = ast->base_clause_list; it; it = it->next) {
        this->baseSpecifier(it->value);
    }
    // unsigned dot_dot_dot_token = ast->dot_dot_dot_token;
    // unsigned lbrace_token = ast->lbrace_token;
    for (DeclarationListAST *it = ast->member_specifier_list; it; it = it->next) {
        this->declaration(it->value);
    }
    // unsigned rbrace_token = ast->rbrace_token;
    // Class *symbol = ast->symbol;
    return false;
}

bool Bind::visit(NamedTypeSpecifierAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    /*const Name *name =*/ this->name(ast->name);
    return false;
}

bool Bind::visit(ElaboratedTypeSpecifierAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned classkey_token = ast->classkey_token;
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    /*const Name *name =*/ this->name(ast->name);
    return false;
}

bool Bind::visit(EnumSpecifierAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned enum_token = ast->enum_token;
    /*const Name *name =*/ this->name(ast->name);
    // unsigned lbrace_token = ast->lbrace_token;
    for (EnumeratorListAST *it = ast->enumerator_list; it; it = it->next) {
        this->enumerator(it->value);
    }
    // unsigned rbrace_token = ast->rbrace_token;
    // Enum *symbol = ast->symbol;
    return false;
}


// PtrOperatorAST
bool Bind::visit(PointerToMemberAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned global_scope_token = ast->global_scope_token;
    for (NestedNameSpecifierListAST *it = ast->nested_name_specifier_list; it; it = it->next) {
        this->nestedNameSpecifier(it->value);
    }
    FullySpecifiedType type;
    // unsigned star_token = ast->star_token;
    for (SpecifierListAST *it = ast->cv_qualifier_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    return false;
}

bool Bind::visit(PointerAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned star_token = ast->star_token;
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->cv_qualifier_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    return false;
}

bool Bind::visit(ReferenceAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned reference_token = ast->reference_token;
    return false;
}


// PostfixAST
bool Bind::visit(CallAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    ExpressionTy base_expression = this->expression(ast->base_expression);
    // unsigned lparen_token = ast->lparen_token;
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        ExpressionTy value = this->expression(it->value);
    }
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool Bind::visit(ArrayAccessAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    ExpressionTy base_expression = this->expression(ast->base_expression);
    // unsigned lbracket_token = ast->lbracket_token;
    ExpressionTy expression = this->expression(ast->expression);
    // unsigned rbracket_token = ast->rbracket_token;
    return false;
}

bool Bind::visit(PostIncrDecrAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    ExpressionTy base_expression = this->expression(ast->base_expression);
    // unsigned incr_decr_token = ast->incr_decr_token;
    return false;
}

bool Bind::visit(MemberAccessAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    ExpressionTy base_expression = this->expression(ast->base_expression);
    // unsigned access_token = ast->access_token;
    // unsigned template_token = ast->template_token;
    /*const Name *member_name =*/ this->name(ast->member_name);
    return false;
}


// CoreDeclaratorAST
bool Bind::visit(DeclaratorIdAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned dot_dot_dot_token = ast->dot_dot_dot_token;
    /*const Name *name =*/ this->name(ast->name);
    return false;
}

bool Bind::visit(NestedDeclaratorAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned lparen_token = ast->lparen_token;
    FullySpecifiedType type;
    type = this->declarator(ast->declarator, type);
    // unsigned rparen_token = ast->rparen_token;
    return false;
}


// PostfixDeclaratorAST
bool Bind::visit(FunctionDeclaratorAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned lparen_token = ast->lparen_token;
    this->parameterDeclarationClause(ast->parameters);
    // unsigned rparen_token = ast->rparen_token;
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->cv_qualifier_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    this->exceptionSpecification(ast->exception_specification, type);
    this->trailingReturnType(ast->trailing_return_type, type);
    ExpressionTy as_cpp_initializer = this->expression(ast->as_cpp_initializer);
    // Function *symbol = ast->symbol;
    return false;
}

bool Bind::visit(ArrayDeclaratorAST *ast)
{
    if (debug_todo)
        translationUnit()->warning(ast->firstToken(), "TODO: %s", __func__);
    // unsigned lbracket_token = ast->lbracket_token;
    ExpressionTy expression = this->expression(ast->expression);
    // unsigned rbracket_token = ast->rbracket_token;
    return false;
}

