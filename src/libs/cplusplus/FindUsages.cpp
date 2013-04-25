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

#include "FindUsages.h"

#include "Overview.h"

#include <cplusplus/AST.h>
#include <cplusplus/TranslationUnit.h>
#include <cplusplus/Control.h>
#include <cplusplus/Names.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/CoreTypes.h>
#include <cplusplus/Literals.h>
#include <cplusplus/Scope.h>

#include <QDebug>

using namespace CPlusPlus;

FindUsages::FindUsages(const QByteArray &originalSource, Document::Ptr doc, const Snapshot &snapshot)
    : ASTVisitor(doc->translationUnit()),
      _id(0),
      _declSymbol(0),
      _doc(doc),
      _snapshot(snapshot),
      _context(doc, snapshot),
      _originalSource(originalSource),
      _source(_doc->utf8Source()),
      _currentScope(0)
{
    _snapshot.insert(_doc);
    typeofExpression.init(_doc, _snapshot, _context.bindings());

    prepareLines(_originalSource);
}

FindUsages::FindUsages(const LookupContext &context)
    : ASTVisitor(context.thisDocument()->translationUnit()),
      _id(0),
      _declSymbol(0),
      _doc(context.thisDocument()),
      _snapshot(context.snapshot()),
      _context(context),
      _originalSource(_doc->utf8Source()),
      _source(_doc->utf8Source()),
      _currentScope(0)
{
    typeofExpression.init(_doc, _snapshot, _context.bindings());

    prepareLines(_originalSource);
}

QList<Usage> FindUsages::usages() const
{ return _usages; }

QList<int> FindUsages::references() const
{ return _references; }

void FindUsages::operator()(Symbol *symbol)
{
    if (! symbol)
        return;

    _id = symbol->identifier();

    if (! _id)
        return;

    _processed.clear();
    _references.clear();
    _usages.clear();
    _declSymbol = symbol;
    _declSymbolFullyQualifiedName = LookupContext::fullyQualifiedName(symbol);

    // get the canonical id
    _id = _doc->control()->identifier(_id->chars(), _id->size());

    if (AST *ast = _doc->translationUnit()->ast())
        translationUnit(ast->asTranslationUnit());
}

void FindUsages::reportResult(unsigned tokenIndex, const Name *name, Scope *scope)
{
    if (! (tokenIndex && name != 0))
        return;

    if (name->identifier() != _id)
        return;

    if (! scope)
        scope = _currentScope;

    const QList<LookupItem> candidates = _context.lookup(name, scope);
    reportResult(tokenIndex, candidates);
}

void FindUsages::reportResult(unsigned tokenIndex, const Identifier *id, Scope *scope)
{
    reportResult(tokenIndex, static_cast<const Name *>(id), scope);
}

void FindUsages::reportResult(unsigned tokenIndex, const QList<LookupItem> &candidates)
{
    if (_processed.contains(tokenIndex))
        return;

    const bool isStrongResult = checkCandidates(candidates);

    if (isStrongResult)
        reportResult(tokenIndex);
}

QString FindUsages::matchingLine(const Token &tk) const
{
    const char *beg = _source.constData();
    const char *cp = beg + tk.offset;
    for (; cp != beg - 1; --cp) {
        if (*cp == '\n')
            break;
    }

    ++cp;

    const char *lineEnd = cp + 1;
    for (; *lineEnd; ++lineEnd) {
        if (*lineEnd == '\n')
            break;
    }

    return QString::fromUtf8(cp, lineEnd - cp);
}

void FindUsages::reportResult(unsigned tokenIndex)
{
    const Token &tk = tokenAt(tokenIndex);
    if (tk.generated())
        return;
    else if (_processed.contains(tokenIndex))
        return;

    _processed.insert(tokenIndex);

    unsigned line, col;
    getTokenStartPosition(tokenIndex, &line, &col);
    QString lineText;
    if (line < _sourceLineEnds.size())
        lineText = fetchLine(line);
    else
        lineText = matchingLine(tk);

    if (col)
        --col;  // adjust the column position.

    const int len = tk.f.length;

    const Usage u(_doc->fileName(), lineText, line, col, len);
    _usages.append(u);
    _references.append(tokenIndex);
}

bool FindUsages::isLocalScope(Scope *scope)
{
    if (scope) {
        if (scope->isBlock() || scope->isTemplate() || scope->isFunction())
            return true;
    }

    return false;
}

bool FindUsages::checkCandidates(const QList<LookupItem> &candidates) const
{
    for (int i = candidates.size() - 1; i != -1; --i) {
        const LookupItem &r = candidates.at(i);

        if (Symbol *s = r.declaration()) {
            if (_declSymbol->isTypenameArgument()) {
                if (s != _declSymbol)
                    return false;
            }

            if (isLocalScope(_declSymbol->enclosingScope()) || isLocalScope(s->enclosingScope())) {
                if (s->enclosingScope()->isTemplate()) {
                    if (s->enclosingScope()->enclosingScope() != _declSymbol->enclosingScope())
                        return false;
                } else if (! s->isUsingDeclaration() && s->enclosingScope() != _declSymbol->enclosingScope()) {
                    return false;
                }
            }

            if (compareFullyQualifiedName(LookupContext::fullyQualifiedName(s), _declSymbolFullyQualifiedName))
                return true;
        }
    }

    return false;
}

void FindUsages::checkExpression(unsigned startToken, unsigned endToken, Scope *scope)
{
    const unsigned begin = tokenAt(startToken).begin();
    const unsigned end = tokenAt(endToken).end();

    const QByteArray expression = _source.mid(begin, end - begin);
    // qDebug() << "*** check expression:" << expression;

    if (! scope)
        scope = _currentScope;

    // make possible to instantiate templates
    typeofExpression.setExpandTemplates(true);
    const QList<LookupItem> results = typeofExpression(expression, scope, TypeOfExpression::Preprocess);
    reportResult(endToken, results);
}

Scope *FindUsages::switchScope(Scope *scope)
{
    if (! scope)
        return _currentScope;

    Scope *previousScope = _currentScope;
    _currentScope = scope;
    return previousScope;
}

void FindUsages::statement(StatementAST *ast)
{
    accept(ast);
}

void FindUsages::expression(ExpressionAST *ast)
{
    accept(ast);
}

void FindUsages::declaration(DeclarationAST *ast)
{
    accept(ast);
}

const Name *FindUsages::name(NameAST *ast)
{
    if (ast) {
        accept(ast);
        return ast->name;
    }

    return 0;
}

void FindUsages::specifier(SpecifierAST *ast)
{
    accept(ast);
}

void FindUsages::ptrOperator(PtrOperatorAST *ast)
{
    accept(ast);
}

void FindUsages::coreDeclarator(CoreDeclaratorAST *ast)
{
    accept(ast);
}

void FindUsages::postfixDeclarator(PostfixDeclaratorAST *ast)
{
    accept(ast);
}

// AST
bool FindUsages::visit(ObjCSelectorArgumentAST *ast)
{
    (void) ast;
    Q_ASSERT(!"unreachable");
    return false;
}

void FindUsages::objCSelectorArgument(ObjCSelectorArgumentAST *ast)
{
    if (! ast)
        return;

    // unsigned name_token = ast->name_token;
    // unsigned colon_token = ast->colon_token;
}

bool FindUsages::visit(AttributeAST *ast)
{
    (void) ast;
    Q_ASSERT(!"unreachable");
    return false;
}

void FindUsages::attribute(AttributeAST *ast)
{
    if (! ast)
        return;

    // unsigned identifier_token = ast->identifier_token;
    // unsigned lparen_token = ast->lparen_token;
    // unsigned tag_token = ast->tag_token;
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        this->expression(it->value);
    }
    // unsigned rparen_token = ast->rparen_token;
}

bool FindUsages::visit(DeclaratorAST *ast)
{
    (void) ast;
    Q_ASSERT(!"unreachable");
    return false;
}

void FindUsages::declarator(DeclaratorAST *ast, Scope *symbol)
{
    if (! ast)
        return;

    for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
        this->specifier(it->value);
    }
    for (PtrOperatorListAST *it = ast->ptr_operator_list; it; it = it->next) {
        this->ptrOperator(it->value);
    }

    Scope *previousScope = switchScope(symbol);

    this->coreDeclarator(ast->core_declarator);
    for (PostfixDeclaratorListAST *it = ast->postfix_declarator_list; it; it = it->next) {
        this->postfixDeclarator(it->value);
    }
    for (SpecifierListAST *it = ast->post_attribute_list; it; it = it->next) {
        this->specifier(it->value);
    }
    // unsigned equals_token = ast->equals_token;
    this->expression(ast->initializer);
    (void) switchScope(previousScope);
}

bool FindUsages::visit(QtPropertyDeclarationItemAST *ast)
{
    (void) ast;
    Q_ASSERT(!"unreachable");
    return false;
}

void FindUsages::qtPropertyDeclarationItem(QtPropertyDeclarationItemAST *ast)
{
    if (! ast)
        return;

    // unsigned item_name_token = ast->item_name_token;
    this->expression(ast->expression);
}

bool FindUsages::visit(QtInterfaceNameAST *ast)
{
    (void) ast;
    Q_ASSERT(!"unreachable");
    return false;
}

void FindUsages::qtInterfaceName(QtInterfaceNameAST *ast)
{
    if (! ast)
        return;

    /*const Name *interface_name =*/ this->name(ast->interface_name);
    for (NameListAST *it = ast->constraint_list; it; it = it->next) {
        /*const Name *value =*/ this->name(it->value);
    }
}

bool FindUsages::visit(BaseSpecifierAST *ast)
{
    (void) ast;
    Q_ASSERT(!"unreachable");
    return false;
}

void FindUsages::baseSpecifier(BaseSpecifierAST *ast)
{
    if (! ast)
        return;

    // unsigned virtual_token = ast->virtual_token;
    // unsigned access_specifier_token = ast->access_specifier_token;
    /*const Name *name =*/ this->name(ast->name);
    // BaseClass *symbol = ast->symbol;
}

bool FindUsages::visit(CtorInitializerAST *ast)
{
    (void) ast;
    Q_ASSERT(!"unreachable");
    return false;
}

void FindUsages::ctorInitializer(CtorInitializerAST *ast)
{
    if (! ast)
        return;

    // unsigned colon_token = ast->colon_token;
    for (MemInitializerListAST *it = ast->member_initializer_list; it; it = it->next) {
        this->memInitializer(it->value);
    }
    // unsigned dot_dot_dot_token = ast->dot_dot_dot_token;
}

bool FindUsages::visit(EnumeratorAST *ast)
{
    (void) ast;
    Q_ASSERT(!"unreachable");
    return false;
}

void FindUsages::enumerator(EnumeratorAST *ast)
{
    if (! ast)
        return;

    // unsigned identifier_token = ast->identifier_token;
    reportResult(ast->identifier_token, identifier(ast->identifier_token));
    // unsigned equal_token = ast->equal_token;
    this->expression(ast->expression);
}

bool FindUsages::visit(DynamicExceptionSpecificationAST *ast)
{
    (void) ast;
    Q_ASSERT(!"unreachable");
    return false;
}

void FindUsages::exceptionSpecification(ExceptionSpecificationAST *ast)
{
    if (! ast)
        return;

    if (DynamicExceptionSpecificationAST *dyn = ast->asDynamicExceptionSpecification()) {
        // unsigned throw_token = ast->throw_token;
        // unsigned lparen_token = ast->lparen_token;
        // unsigned dot_dot_dot_token = ast->dot_dot_dot_token;
        for (ExpressionListAST *it = dyn->type_id_list; it; it = it->next) {
            this->expression(it->value);
        }
        // unsigned rparen_token = ast->rparen_token;
    } else if (NoExceptSpecificationAST *no = ast->asNoExceptSpecification()) {
        this->expression(no->expression);
    }
}

bool FindUsages::visit(MemInitializerAST *ast)
{
    (void) ast;
    Q_ASSERT(!"unreachable");
    return false;
}

void FindUsages::memInitializer(MemInitializerAST *ast)
{
    if (! ast)
        return;

    if (_currentScope->isFunction()) {
        Class *classScope = _currentScope->enclosingClass();
        if (! classScope) {
            if (ClassOrNamespace *binding = _context.lookupType(_currentScope)) {
                foreach (Symbol *s, binding->symbols()) {
                    if (Class *k = s->asClass()) {
                        classScope = k;
                        break;
                    }
                }
            }
        }

        if (classScope) {
            Scope *previousScope = switchScope(classScope);
            /*const Name *name =*/ this->name(ast->name);
            (void) switchScope(previousScope);
        }
    }
    this->expression(ast->expression);
}

bool FindUsages::visit(NestedNameSpecifierAST *ast)
{
    (void) ast;
    Q_ASSERT(!"unreachable");
    return false;
}

void FindUsages::nestedNameSpecifier(NestedNameSpecifierAST *ast)
{
    if (! ast)
        return;

    /*const Name *class_or_namespace_name =*/ this->name(ast->class_or_namespace_name);
    // unsigned scope_token = ast->scope_token;
}

bool FindUsages::visit(ExpressionListParenAST *ast)
{
    // unsigned lparen_token = ast->lparen_token;
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        this->expression(it->value);
    }
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

void FindUsages::newPlacement(ExpressionListParenAST *ast)
{
    if (! ast)
        return;

    // unsigned lparen_token = ast->lparen_token;
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        this->expression(it->value);
    }
    // unsigned rparen_token = ast->rparen_token;
}

bool FindUsages::visit(NewArrayDeclaratorAST *ast)
{
    (void) ast;
    Q_ASSERT(!"unreachable");
    return false;
}

void FindUsages::newArrayDeclarator(NewArrayDeclaratorAST *ast)
{
    if (! ast)
        return;

    // unsigned lbracket_token = ast->lbracket_token;
    this->expression(ast->expression);
    // unsigned rbracket_token = ast->rbracket_token;
}

bool FindUsages::visit(NewTypeIdAST *ast)
{
    (void) ast;
    Q_ASSERT(!"unreachable");
    return false;
}

void FindUsages::newTypeId(NewTypeIdAST *ast)
{
    if (! ast)
        return;

    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
        this->specifier(it->value);
    }
    for (PtrOperatorListAST *it = ast->ptr_operator_list; it; it = it->next) {
        this->ptrOperator(it->value);
    }
    for (NewArrayDeclaratorListAST *it = ast->new_array_declarator_list; it; it = it->next) {
        this->newArrayDeclarator(it->value);
    }
}

bool FindUsages::visit(OperatorAST *ast)
{
    (void) ast;
    Q_ASSERT(!"unreachable");
    return false;
}

void FindUsages::cppOperator(OperatorAST *ast)
{
    if (! ast)
        return;

    // unsigned op_token = ast->op_token;
    // unsigned open_token = ast->open_token;
    // unsigned close_token = ast->close_token;
}

bool FindUsages::visit(ParameterDeclarationClauseAST *ast)
{
    (void) ast;
    Q_ASSERT(!"unreachable");
    return false;
}

void FindUsages::parameterDeclarationClause(ParameterDeclarationClauseAST *ast)
{
    if (! ast)
        return;

    for (ParameterDeclarationListAST *it = ast->parameter_declaration_list; it; it = it->next) {
        this->declaration(it->value);
    }
    // unsigned dot_dot_dot_token = ast->dot_dot_dot_token;
}

bool FindUsages::visit(TranslationUnitAST *ast)
{
    (void) ast;
    Q_ASSERT(!"unreachable");
    return false;
}

void FindUsages::translationUnit(TranslationUnitAST *ast)
{
    if (! ast)
        return;

    Scope *previousScope = switchScope(_doc->globalNamespace());
    for (DeclarationListAST *it = ast->declaration_list; it; it = it->next) {
        this->declaration(it->value);
    }
    (void) switchScope(previousScope);
}

bool FindUsages::visit(ObjCProtocolRefsAST *ast)
{
    (void) ast;
    Q_ASSERT(!"unreachable");
    return false;
}

void FindUsages::objCProtocolRefs(ObjCProtocolRefsAST *ast)
{
    if (! ast)
        return;

    // unsigned less_token = ast->less_token;
    for (NameListAST *it = ast->identifier_list; it; it = it->next) {
        /*const Name *value =*/ this->name(it->value);
    }
    // unsigned greater_token = ast->greater_token;
}

bool FindUsages::visit(ObjCMessageArgumentAST *ast)
{
    (void) ast;
    Q_ASSERT(!"unreachable");
    return false;
}

void FindUsages::objCMessageArgument(ObjCMessageArgumentAST *ast)
{
    if (! ast)
        return;

    this->expression(ast->parameter_value_expression);
}

bool FindUsages::visit(ObjCTypeNameAST *ast)
{
    (void) ast;
    Q_ASSERT(!"unreachable");
    return false;
}

void FindUsages::objCTypeName(ObjCTypeNameAST *ast)
{
    if (! ast)
        return;

    // unsigned lparen_token = ast->lparen_token;
    // unsigned type_qualifier_token = ast->type_qualifier_token;
    this->expression(ast->type_id);
    // unsigned rparen_token = ast->rparen_token;
}

bool FindUsages::visit(ObjCInstanceVariablesDeclarationAST *ast)
{
    (void) ast;
    Q_ASSERT(!"unreachable");
    return false;
}

void FindUsages::objCInstanceVariablesDeclaration(ObjCInstanceVariablesDeclarationAST *ast)
{
    if (! ast)
        return;

    // unsigned lbrace_token = ast->lbrace_token;
    for (DeclarationListAST *it = ast->instance_variable_list; it; it = it->next) {
        this->declaration(it->value);
    }
    // unsigned rbrace_token = ast->rbrace_token;
}

bool FindUsages::visit(ObjCPropertyAttributeAST *ast)
{
    (void) ast;
    Q_ASSERT(!"unreachable");
    return false;
}

void FindUsages::objCPropertyAttribute(ObjCPropertyAttributeAST *ast)
{
    if (! ast)
        return;

    // unsigned attribute_identifier_token = ast->attribute_identifier_token;
    // unsigned equals_token = ast->equals_token;
    /*const Name *method_selector =*/ this->name(ast->method_selector);
}

bool FindUsages::visit(ObjCMessageArgumentDeclarationAST *ast)
{
    (void) ast;
    Q_ASSERT(!"unreachable");
    return false;
}

void FindUsages::objCMessageArgumentDeclaration(ObjCMessageArgumentDeclarationAST *ast)
{
    if (! ast)
        return;

    this->objCTypeName(ast->type_name);
    for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
        this->specifier(it->value);
    }
    /*const Name *param_name =*/ this->name(ast->param_name);
    // Argument *argument = ast->argument;
}

bool FindUsages::visit(ObjCMethodPrototypeAST *ast)
{
    (void) ast;
    Q_ASSERT(!"unreachable");
    return false;
}

void FindUsages::objCMethodPrototype(ObjCMethodPrototypeAST *ast)
{
    if (! ast)
        return;

    // unsigned method_type_token = ast->method_type_token;
    this->objCTypeName(ast->type_name);
    /*const Name *selector =*/ this->name(ast->selector);
    Scope *previousScope = switchScope(ast->symbol);
    for (ObjCMessageArgumentDeclarationListAST *it = ast->argument_list; it; it = it->next) {
        this->objCMessageArgumentDeclaration(it->value);
    }
    // unsigned dot_dot_dot_token = ast->dot_dot_dot_token;
    for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
        this->specifier(it->value);
    }
    // ObjCMethod *symbol = ast->symbol;
    (void) switchScope(previousScope);
}

bool FindUsages::visit(ObjCSynthesizedPropertyAST *ast)
{
    (void) ast;
    Q_ASSERT(!"unreachable");
    return false;
}

void FindUsages::objCSynthesizedProperty(ObjCSynthesizedPropertyAST *ast)
{
    if (! ast)
        return;

    // unsigned property_identifier_token = ast->property_identifier_token;
    // unsigned equals_token = ast->equals_token;
    // unsigned alias_identifier_token = ast->alias_identifier_token;
}

bool FindUsages::visit(LambdaIntroducerAST *ast)
{
    (void) ast;
    Q_ASSERT(!"unreachable");
    return false;
}

void FindUsages::lambdaIntroducer(LambdaIntroducerAST *ast)
{
    if (! ast)
        return;

    // unsigned lbracket_token = ast->lbracket_token;
    this->lambdaCapture(ast->lambda_capture);
    // unsigned rbracket_token = ast->rbracket_token;
}

bool FindUsages::visit(LambdaCaptureAST *ast)
{
    (void) ast;
    Q_ASSERT(!"unreachable");
    return false;
}

void FindUsages::lambdaCapture(LambdaCaptureAST *ast)
{
    if (! ast)
        return;

    // unsigned default_capture_token = ast->default_capture_token;
    for (CaptureListAST *it = ast->capture_list; it; it = it->next) {
        this->capture(it->value);
    }
}

bool FindUsages::visit(CaptureAST *ast)
{
    (void) ast;
    Q_ASSERT(!"unreachable");
    return false;
}

void FindUsages::capture(CaptureAST *ast)
{
    if (! ast)
        return;

    this->name(ast->identifier);
}

bool FindUsages::visit(LambdaDeclaratorAST *ast)
{
    (void) ast;
    Q_ASSERT(!"unreachable");
    return false;
}

void FindUsages::lambdaDeclarator(LambdaDeclaratorAST *ast)
{
    if (! ast)
        return;

    // unsigned lparen_token = ast->lparen_token;
    this->parameterDeclarationClause(ast->parameter_declaration_clause);
    // unsigned rparen_token = ast->rparen_token;
    for (SpecifierListAST *it = ast->attributes; it; it = it->next) {
        this->specifier(it->value);
    }
    // unsigned mutable_token = ast->mutable_token;
    this->exceptionSpecification(ast->exception_specification);
    this->trailingReturnType(ast->trailing_return_type);
}

bool FindUsages::visit(TrailingReturnTypeAST *ast)
{
    (void) ast;
    Q_ASSERT(!"unreachable");
    return false;
}

void FindUsages::trailingReturnType(TrailingReturnTypeAST *ast)
{
    if (! ast)
        return;

    // unsigned arrow_token = ast->arrow_token;
    for (SpecifierListAST *it = ast->attributes; it; it = it->next) {
        this->specifier(it->value);
    }
    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
        this->specifier(it->value);
    }
    this->declarator(ast->declarator);
}


// StatementAST
bool FindUsages::visit(QtMemberDeclarationAST *ast)
{
    // unsigned q_token = ast->q_token;
    // unsigned lparen_token = ast->lparen_token;
    this->expression(ast->type_id);
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool FindUsages::visit(CaseStatementAST *ast)
{
    // unsigned case_token = ast->case_token;
    this->expression(ast->expression);
    // unsigned colon_token = ast->colon_token;
    this->statement(ast->statement);
    return false;
}

bool FindUsages::visit(CompoundStatementAST *ast)
{
    // unsigned lbrace_token = ast->lbrace_token;
    Scope *previousScope = switchScope(ast->symbol);
    for (StatementListAST *it = ast->statement_list; it; it = it->next) {
        this->statement(it->value);
    }
    // unsigned rbrace_token = ast->rbrace_token;
    // Block *symbol = ast->symbol;
    (void) switchScope(previousScope);
    return false;
}

bool FindUsages::visit(DeclarationStatementAST *ast)
{
    this->declaration(ast->declaration);
    return false;
}

bool FindUsages::visit(DoStatementAST *ast)
{
    // unsigned do_token = ast->do_token;
    this->statement(ast->statement);
    // unsigned while_token = ast->while_token;
    // unsigned lparen_token = ast->lparen_token;
    this->expression(ast->expression);
    // unsigned rparen_token = ast->rparen_token;
    // unsigned semicolon_token = ast->semicolon_token;
    return false;
}

bool FindUsages::visit(ExpressionOrDeclarationStatementAST *ast)
{
    this->statement(ast->expression);
    this->statement(ast->declaration);
    return false;
}

bool FindUsages::visit(ExpressionStatementAST *ast)
{
    this->expression(ast->expression);
    // unsigned semicolon_token = ast->semicolon_token;
    return false;
}

bool FindUsages::visit(ForeachStatementAST *ast)
{
    // unsigned foreach_token = ast->foreach_token;
    // unsigned lparen_token = ast->lparen_token;
    Scope *previousScope = switchScope(ast->symbol);
    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
        this->specifier(it->value);
    }
    this->declarator(ast->declarator);
    this->expression(ast->initializer);
    // unsigned comma_token = ast->comma_token;
    this->expression(ast->expression);
    // unsigned rparen_token = ast->rparen_token;
    this->statement(ast->statement);
    // Block *symbol = ast->symbol;
    (void) switchScope(previousScope);
    return false;
}

bool FindUsages::visit(RangeBasedForStatementAST *ast)
{
    Scope *previousScope = switchScope(ast->symbol);
    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
        this->specifier(it->value);
    }
    this->declarator(ast->declarator);
    // unsigned comma_token = ast->comma_token;
    this->expression(ast->expression);
    // unsigned rparen_token = ast->rparen_token;
    this->statement(ast->statement);
    // Block *symbol = ast->symbol;
    (void) switchScope(previousScope);
    return false;
}

bool FindUsages::visit(ForStatementAST *ast)
{
    // unsigned for_token = ast->for_token;
    // unsigned lparen_token = ast->lparen_token;
    Scope *previousScope = switchScope(ast->symbol);
    this->statement(ast->initializer);
    this->expression(ast->condition);
    // unsigned semicolon_token = ast->semicolon_token;
    this->expression(ast->expression);
    // unsigned rparen_token = ast->rparen_token;
    this->statement(ast->statement);
    // Block *symbol = ast->symbol;
    (void) switchScope(previousScope);
    return false;
}

bool FindUsages::visit(IfStatementAST *ast)
{
    // unsigned if_token = ast->if_token;
    // unsigned lparen_token = ast->lparen_token;
    Scope *previousScope = switchScope(ast->symbol);
    this->expression(ast->condition);
    // unsigned rparen_token = ast->rparen_token;
    this->statement(ast->statement);
    // unsigned else_token = ast->else_token;
    this->statement(ast->else_statement);
    // Block *symbol = ast->symbol;
    (void) switchScope(previousScope);
    return false;
}

bool FindUsages::visit(LabeledStatementAST *ast)
{
    // unsigned label_token = ast->label_token;
    // unsigned colon_token = ast->colon_token;
    this->statement(ast->statement);
    return false;
}

bool FindUsages::visit(BreakStatementAST *ast)
{
    (void) ast;
    // unsigned break_token = ast->break_token;
    // unsigned semicolon_token = ast->semicolon_token;
    return false;
}

bool FindUsages::visit(ContinueStatementAST *ast)
{
    (void) ast;
    // unsigned continue_token = ast->continue_token;
    // unsigned semicolon_token = ast->semicolon_token;
    return false;
}

bool FindUsages::visit(GotoStatementAST *ast)
{
    (void) ast;
    // unsigned goto_token = ast->goto_token;
    // unsigned identifier_token = ast->identifier_token;
    // unsigned semicolon_token = ast->semicolon_token;
    return false;
}

bool FindUsages::visit(ReturnStatementAST *ast)
{
    // unsigned return_token = ast->return_token;
    this->expression(ast->expression);
    // unsigned semicolon_token = ast->semicolon_token;
    return false;
}

bool FindUsages::visit(SwitchStatementAST *ast)
{
    // unsigned switch_token = ast->switch_token;
    // unsigned lparen_token = ast->lparen_token;
    Scope *previousScope = switchScope(ast->symbol);
    this->expression(ast->condition);
    // unsigned rparen_token = ast->rparen_token;
    this->statement(ast->statement);
    // Block *symbol = ast->symbol;
    (void) switchScope(previousScope);
    return false;
}

bool FindUsages::visit(TryBlockStatementAST *ast)
{
    // unsigned try_token = ast->try_token;
    this->statement(ast->statement);
    for (CatchClauseListAST *it = ast->catch_clause_list; it; it = it->next) {
        this->statement(it->value);
    }
    return false;
}

bool FindUsages::visit(CatchClauseAST *ast)
{
    // unsigned catch_token = ast->catch_token;
    // unsigned lparen_token = ast->lparen_token;
    Scope *previousScope = switchScope(ast->symbol);
    this->declaration(ast->exception_declaration);
    // unsigned rparen_token = ast->rparen_token;
    this->statement(ast->statement);
    // Block *symbol = ast->symbol;
    (void) switchScope(previousScope);
    return false;
}

bool FindUsages::visit(WhileStatementAST *ast)
{
    // unsigned while_token = ast->while_token;
    // unsigned lparen_token = ast->lparen_token;
    Scope *previousScope = switchScope(ast->symbol);
    this->expression(ast->condition);
    // unsigned rparen_token = ast->rparen_token;
    this->statement(ast->statement);
    // Block *symbol = ast->symbol;
    (void) switchScope(previousScope);
    return false;
}

bool FindUsages::visit(ObjCFastEnumerationAST *ast)
{
    // unsigned for_token = ast->for_token;
    // unsigned lparen_token = ast->lparen_token;
    Scope *previousScope = switchScope(ast->symbol);
    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
        this->specifier(it->value);
    }
    this->declarator(ast->declarator);
    this->expression(ast->initializer);
    // unsigned in_token = ast->in_token;
    this->expression(ast->fast_enumeratable_expression);
    // unsigned rparen_token = ast->rparen_token;
    this->statement(ast->statement);
    // Block *symbol = ast->symbol;
    (void) switchScope(previousScope);
    return false;
}

bool FindUsages::visit(ObjCSynchronizedStatementAST *ast)
{
    // unsigned synchronized_token = ast->synchronized_token;
    // unsigned lparen_token = ast->lparen_token;
    this->expression(ast->synchronized_object);
    // unsigned rparen_token = ast->rparen_token;
    this->statement(ast->statement);
    return false;
}


// ExpressionAST
bool FindUsages::visit(IdExpressionAST *ast)
{
    /*const Name *name =*/ this->name(ast->name);
    return false;
}

bool FindUsages::visit(CompoundExpressionAST *ast)
{
    // unsigned lparen_token = ast->lparen_token;
    this->statement(ast->statement);
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool FindUsages::visit(CompoundLiteralAST *ast)
{
    // unsigned lparen_token = ast->lparen_token;
    this->expression(ast->type_id);
    // unsigned rparen_token = ast->rparen_token;
    this->expression(ast->initializer);
    return false;
}

bool FindUsages::visit(QtMethodAST *ast)
{
    // unsigned method_token = ast->method_token;
    // unsigned lparen_token = ast->lparen_token;
    this->declarator(ast->declarator);
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool FindUsages::visit(BinaryExpressionAST *ast)
{
    this->expression(ast->left_expression);
    // unsigned binary_op_token = ast->binary_op_token;
    this->expression(ast->right_expression);
    return false;
}

bool FindUsages::visit(CastExpressionAST *ast)
{
    // unsigned lparen_token = ast->lparen_token;
    this->expression(ast->type_id);
    // unsigned rparen_token = ast->rparen_token;
    this->expression(ast->expression);
    return false;
}

bool FindUsages::visit(ConditionAST *ast)
{
    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
        this->specifier(it->value);
    }
    this->declarator(ast->declarator);
    return false;
}

bool FindUsages::visit(ConditionalExpressionAST *ast)
{
    this->expression(ast->condition);
    // unsigned question_token = ast->question_token;
    this->expression(ast->left_expression);
    // unsigned colon_token = ast->colon_token;
    this->expression(ast->right_expression);
    return false;
}

bool FindUsages::visit(CppCastExpressionAST *ast)
{
    // unsigned cast_token = ast->cast_token;
    // unsigned less_token = ast->less_token;
    this->expression(ast->type_id);
    // unsigned greater_token = ast->greater_token;
    // unsigned lparen_token = ast->lparen_token;
    this->expression(ast->expression);
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool FindUsages::visit(DeleteExpressionAST *ast)
{
    // unsigned scope_token = ast->scope_token;
    // unsigned delete_token = ast->delete_token;
    // unsigned lbracket_token = ast->lbracket_token;
    // unsigned rbracket_token = ast->rbracket_token;
    this->expression(ast->expression);
    return false;
}

bool FindUsages::visit(ArrayInitializerAST *ast)
{
    // unsigned lbrace_token = ast->lbrace_token;
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        this->expression(it->value);
    }
    // unsigned rbrace_token = ast->rbrace_token;
    return false;
}

bool FindUsages::visit(NewExpressionAST *ast)
{
    // unsigned scope_token = ast->scope_token;
    // unsigned new_token = ast->new_token;
    this->newPlacement(ast->new_placement);
    // unsigned lparen_token = ast->lparen_token;
    this->expression(ast->type_id);
    // unsigned rparen_token = ast->rparen_token;
    this->newTypeId(ast->new_type_id);
    this->expression(ast->new_initializer);
    return false;
}

bool FindUsages::visit(TypeidExpressionAST *ast)
{
    // unsigned typeid_token = ast->typeid_token;
    // unsigned lparen_token = ast->lparen_token;
    this->expression(ast->expression);
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool FindUsages::visit(TypenameCallExpressionAST *ast)
{
    // unsigned typename_token = ast->typename_token;
    /*const Name *name =*/ this->name(ast->name);
    this->expression(ast->expression);
    return false;
}

bool FindUsages::visit(TypeConstructorCallAST *ast)
{
    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
        this->specifier(it->value);
    }
    this->expression(ast->expression);
    return false;
}

bool FindUsages::visit(SizeofExpressionAST *ast)
{
    // unsigned sizeof_token = ast->sizeof_token;
    // unsigned dot_dot_dot_token = ast->dot_dot_dot_token;
    // unsigned lparen_token = ast->lparen_token;
    this->expression(ast->expression);
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool FindUsages::visit(PointerLiteralAST *ast)
{
    (void) ast;
    // unsigned literal_token = ast->literal_token;
    return false;
}

bool FindUsages::visit(NumericLiteralAST *ast)
{
    (void) ast;
    // unsigned literal_token = ast->literal_token;
    return false;
}

bool FindUsages::visit(BoolLiteralAST *ast)
{
    (void) ast;
    // unsigned literal_token = ast->literal_token;
    return false;
}

bool FindUsages::visit(ThisExpressionAST *ast)
{
    (void) ast;
    // unsigned this_token = ast->this_token;
    return false;
}

bool FindUsages::visit(NestedExpressionAST *ast)
{
    // unsigned lparen_token = ast->lparen_token;
    this->expression(ast->expression);
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool FindUsages::visit(StringLiteralAST *ast)
{
    // unsigned literal_token = ast->literal_token;
    this->expression(ast->next);
    return false;
}

bool FindUsages::visit(ThrowExpressionAST *ast)
{
    // unsigned throw_token = ast->throw_token;
    this->expression(ast->expression);
    return false;
}

bool FindUsages::visit(TypeIdAST *ast)
{
    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
        this->specifier(it->value);
    }
    this->declarator(ast->declarator);
    return false;
}

bool FindUsages::visit(UnaryExpressionAST *ast)
{
    // unsigned unary_op_token = ast->unary_op_token;
    this->expression(ast->expression);
    return false;
}

bool FindUsages::visit(ObjCMessageExpressionAST *ast)
{
    // unsigned lbracket_token = ast->lbracket_token;
    this->expression(ast->receiver_expression);
    /*const Name *selector =*/ this->name(ast->selector);
    for (ObjCMessageArgumentListAST *it = ast->argument_list; it; it = it->next) {
        this->objCMessageArgument(it->value);
    }
    // unsigned rbracket_token = ast->rbracket_token;
    return false;
}

bool FindUsages::visit(ObjCProtocolExpressionAST *ast)
{
    (void) ast;
    // unsigned protocol_token = ast->protocol_token;
    // unsigned lparen_token = ast->lparen_token;
    // unsigned identifier_token = ast->identifier_token;
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool FindUsages::visit(ObjCEncodeExpressionAST *ast)
{
    // unsigned encode_token = ast->encode_token;
    this->objCTypeName(ast->type_name);
    return false;
}

bool FindUsages::visit(ObjCSelectorExpressionAST *ast)
{
    // unsigned selector_token = ast->selector_token;
    // unsigned lparen_token = ast->lparen_token;
    /*const Name *selector =*/ this->name(ast->selector);
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool FindUsages::visit(LambdaExpressionAST *ast)
{
    this->lambdaIntroducer(ast->lambda_introducer);
    this->lambdaDeclarator(ast->lambda_declarator);
    this->statement(ast->statement);
    return false;
}

bool FindUsages::visit(BracedInitializerAST *ast)
{
    // unsigned lbrace_token = ast->lbrace_token;
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        this->expression(it->value);
    }
    // unsigned comma_token = ast->comma_token;
    // unsigned rbrace_token = ast->rbrace_token;
    return false;
}


// DeclarationAST
bool FindUsages::visit(SimpleDeclarationAST *ast)
{
    // unsigned qt_invokable_token = ast->qt_invokable_token;
    for (SpecifierListAST *it = ast->decl_specifier_list; it; it = it->next) {
        this->specifier(it->value);
    }
    for (DeclaratorListAST *it = ast->declarator_list; it; it = it->next) {
        this->declarator(it->value);
    }
    // unsigned semicolon_token = ast->semicolon_token;
    // List<Declaration *> *symbols = ast->symbols;
    return false;
}

bool FindUsages::visit(EmptyDeclarationAST *ast)
{
    (void) ast;
    // unsigned semicolon_token = ast->semicolon_token;
    return false;
}

bool FindUsages::visit(AccessDeclarationAST *ast)
{
    (void) ast;
    // unsigned access_specifier_token = ast->access_specifier_token;
    // unsigned slots_token = ast->slots_token;
    // unsigned colon_token = ast->colon_token;
    return false;
}

bool FindUsages::visit(QtObjectTagAST *ast)
{
    (void) ast;
    // unsigned q_object_token = ast->q_object_token;
    return false;
}

bool FindUsages::visit(QtPrivateSlotAST *ast)
{
    // unsigned q_private_slot_token = ast->q_private_slot_token;
    // unsigned lparen_token = ast->lparen_token;
    // unsigned dptr_token = ast->dptr_token;
    // unsigned dptr_lparen_token = ast->dptr_lparen_token;
    // unsigned dptr_rparen_token = ast->dptr_rparen_token;
    // unsigned comma_token = ast->comma_token;
    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
        this->specifier(it->value);
    }
    this->declarator(ast->declarator);
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool FindUsages::visit(QtPropertyDeclarationAST *ast)
{
    // unsigned property_specifier_token = ast->property_specifier_token;
    // unsigned lparen_token = ast->lparen_token;
    this->expression(ast->type_id);
    /*const Name *property_name =*/ this->name(ast->property_name);
    for (QtPropertyDeclarationItemListAST *it = ast->property_declaration_item_list; it; it = it->next) {
        this->qtPropertyDeclarationItem(it->value);
    }
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool FindUsages::visit(QtEnumDeclarationAST *ast)
{
    // unsigned enum_specifier_token = ast->enum_specifier_token;
    // unsigned lparen_token = ast->lparen_token;
    for (NameListAST *it = ast->enumerator_list; it; it = it->next) {
        /*const Name *value =*/ this->name(it->value);
    }
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool FindUsages::visit(QtFlagsDeclarationAST *ast)
{
    // unsigned flags_specifier_token = ast->flags_specifier_token;
    // unsigned lparen_token = ast->lparen_token;
    for (NameListAST *it = ast->flag_enums_list; it; it = it->next) {
        /*const Name *value =*/ this->name(it->value);
    }
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool FindUsages::visit(QtInterfacesDeclarationAST *ast)
{
    // unsigned interfaces_token = ast->interfaces_token;
    // unsigned lparen_token = ast->lparen_token;
    for (QtInterfaceNameListAST *it = ast->interface_name_list; it; it = it->next) {
        this->qtInterfaceName(it->value);
    }
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool FindUsages::visit(AsmDefinitionAST *ast)
{
    (void) ast;
    // unsigned asm_token = ast->asm_token;
    // unsigned volatile_token = ast->volatile_token;
    // unsigned lparen_token = ast->lparen_token;
    // unsigned rparen_token = ast->rparen_token;
    // unsigned semicolon_token = ast->semicolon_token;
    return false;
}

bool FindUsages::visit(ExceptionDeclarationAST *ast)
{
    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
        this->specifier(it->value);
    }
    this->declarator(ast->declarator);
    // unsigned dot_dot_dot_token = ast->dot_dot_dot_token;
    return false;
}

bool FindUsages::visit(FunctionDefinitionAST *ast)
{
    // unsigned qt_invokable_token = ast->qt_invokable_token;
    for (SpecifierListAST *it = ast->decl_specifier_list; it; it = it->next) {
        this->specifier(it->value);
    }
    this->declarator(ast->declarator, ast->symbol);
    Scope *previousScope = switchScope(ast->symbol);
    this->ctorInitializer(ast->ctor_initializer);
    this->statement(ast->function_body);
    // Function *symbol = ast->symbol;
    (void) switchScope(previousScope);
    return false;
}

bool FindUsages::visit(LinkageBodyAST *ast)
{
    // unsigned lbrace_token = ast->lbrace_token;
    for (DeclarationListAST *it = ast->declaration_list; it; it = it->next) {
        this->declaration(it->value);
    }
    // unsigned rbrace_token = ast->rbrace_token;
    return false;
}

bool FindUsages::visit(LinkageSpecificationAST *ast)
{
    // unsigned extern_token = ast->extern_token;
    // unsigned extern_type_token = ast->extern_type_token;
    this->declaration(ast->declaration);
    return false;
}

bool FindUsages::visit(NamespaceAST *ast)
{
    // unsigned namespace_token = ast->namespace_token;
    // unsigned identifier_token = ast->identifier_token;
    reportResult(ast->identifier_token, identifier(ast->identifier_token));
    Scope *previousScope = switchScope(ast->symbol);
    for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
        this->specifier(it->value);
    }
    this->declaration(ast->linkage_body);
    // Namespace *symbol = ast->symbol;
    (void) switchScope(previousScope);
    return false;
}

bool FindUsages::visit(NamespaceAliasDefinitionAST *ast)
{
    // unsigned namespace_token = ast->namespace_token;
    // unsigned namespace_name_token = ast->namespace_name_token;
    // unsigned equal_token = ast->equal_token;
    /*const Name *name =*/ this->name(ast->name);
    // unsigned semicolon_token = ast->semicolon_token;
    return false;
}

bool FindUsages::visit(ParameterDeclarationAST *ast)
{
    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
        this->specifier(it->value);
    }
    this->declarator(ast->declarator);
    // unsigned equal_token = ast->equal_token;
    this->expression(ast->expression);
    // Argument *symbol = ast->symbol;
    return false;
}

bool FindUsages::visit(StaticAssertDeclarationAST *ast)
{
    this->expression(ast->expression);
    return false;
}

bool FindUsages::visit(TemplateDeclarationAST *ast)
{
    // unsigned export_token = ast->export_token;
    // unsigned template_token = ast->template_token;
    // unsigned less_token = ast->less_token;

    Scope *previousScope = switchScope(ast->symbol);

    for (DeclarationListAST *it = ast->template_parameter_list; it; it = it->next) {
        this->declaration(it->value);
    }
    // unsigned greater_token = ast->greater_token;
    this->declaration(ast->declaration);

    (void) switchScope(previousScope);
    return false;
}

bool FindUsages::visit(TypenameTypeParameterAST *ast)
{
    // unsigned classkey_token = ast->classkey_token;
    // unsigned dot_dot_dot_token = ast->dot_dot_dot_token;
    /*const Name *name =*/ this->name(ast->name);
    // unsigned equal_token = ast->equal_token;
    this->expression(ast->type_id);
    // TypenameArgument *symbol = ast->symbol;
    return false;
}

bool FindUsages::visit(TemplateTypeParameterAST *ast)
{
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
    this->expression(ast->type_id);
    // TypenameArgument *symbol = ast->symbol;
    return false;
}

bool FindUsages::visit(UsingAST *ast)
{
    // unsigned using_token = ast->using_token;
    // unsigned typename_token = ast->typename_token;
    /*const Name *name =*/ this->name(ast->name);
    // unsigned semicolon_token = ast->semicolon_token;
    // UsingDeclaration *symbol = ast->symbol;
    return false;
}

bool FindUsages::visit(UsingDirectiveAST *ast)
{
    // unsigned using_token = ast->using_token;
    // unsigned namespace_token = ast->namespace_token;
    /*const Name *name =*/ this->name(ast->name);
    // unsigned semicolon_token = ast->semicolon_token;
    // UsingNamespaceDirective *symbol = ast->symbol;
    return false;
}

bool FindUsages::visit(ObjCClassForwardDeclarationAST *ast)
{
    for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
        this->specifier(it->value);
    }
    // unsigned class_token = ast->class_token;
    for (NameListAST *it = ast->identifier_list; it; it = it->next) {
        /*const Name *value =*/ this->name(it->value);
    }
    // unsigned semicolon_token = ast->semicolon_token;
    // List<ObjCForwardClassDeclaration *> *symbols = ast->symbols;
    return false;
}

bool FindUsages::visit(ObjCClassDeclarationAST *ast)
{
    for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
        this->specifier(it->value);
    }
    // unsigned interface_token = ast->interface_token;
    // unsigned implementation_token = ast->implementation_token;
    /*const Name *class_name =*/ this->name(ast->class_name);

    Scope *previousScope = switchScope(ast->symbol);

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
    (void) switchScope(previousScope);
    return false;
}

bool FindUsages::visit(ObjCProtocolForwardDeclarationAST *ast)
{
    for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
        this->specifier(it->value);
    }
    // unsigned protocol_token = ast->protocol_token;
    for (NameListAST *it = ast->identifier_list; it; it = it->next) {
        /*const Name *value =*/ this->name(it->value);
    }
    // unsigned semicolon_token = ast->semicolon_token;
    // List<ObjCForwardProtocolDeclaration *> *symbols = ast->symbols;
    return false;
}

bool FindUsages::visit(ObjCProtocolDeclarationAST *ast)
{
    for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
        this->specifier(it->value);
    }
    // unsigned protocol_token = ast->protocol_token;
    /*const Name *name =*/ this->name(ast->name);

    Scope *previousScope = switchScope(ast->symbol);

    this->objCProtocolRefs(ast->protocol_refs);
    for (DeclarationListAST *it = ast->member_declaration_list; it; it = it->next) {
        this->declaration(it->value);
    }
    // unsigned end_token = ast->end_token;
    // ObjCProtocol *symbol = ast->symbol;
    (void) switchScope(previousScope);
    return false;
}

bool FindUsages::visit(ObjCVisibilityDeclarationAST *ast)
{
    (void) ast;
    // unsigned visibility_token = ast->visibility_token;
    return false;
}

bool FindUsages::visit(ObjCPropertyDeclarationAST *ast)
{
    for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
        this->specifier(it->value);
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

bool FindUsages::visit(ObjCMethodDeclarationAST *ast)
{
    this->objCMethodPrototype(ast->method_prototype);
    this->statement(ast->function_body);
    // unsigned semicolon_token = ast->semicolon_token;
    return false;
}

bool FindUsages::visit(ObjCSynthesizedPropertiesDeclarationAST *ast)
{
    // unsigned synthesized_token = ast->synthesized_token;
    for (ObjCSynthesizedPropertyListAST *it = ast->property_identifier_list; it; it = it->next) {
        this->objCSynthesizedProperty(it->value);
    }
    // unsigned semicolon_token = ast->semicolon_token;
    return false;
}

bool FindUsages::visit(ObjCDynamicPropertiesDeclarationAST *ast)
{
    // unsigned dynamic_token = ast->dynamic_token;
    for (NameListAST *it = ast->property_identifier_list; it; it = it->next) {
        /*const Name *value =*/ this->name(it->value);
    }
    // unsigned semicolon_token = ast->semicolon_token;
    return false;
}

// NameAST
bool FindUsages::visit(ObjCSelectorAST *ast)
{
    if (ast->name)
        reportResult(ast->firstToken(), ast->name);

    for (ObjCSelectorArgumentListAST *it = ast->selector_argument_list; it; it = it->next) {
        this->objCSelectorArgument(it->value);
    }
    return false;
}

bool FindUsages::visit(QualifiedNameAST *ast)
{
#if 0
    // unsigned global_scope_token = ast->global_scope_token;
    for (NestedNameSpecifierListAST *it = ast->nested_name_specifier_list; it; it = it->next) {
        this->nestedNameSpecifier(it->value);
    }
    const Name *unqualified_name = this->name(ast->unqualified_name);
#endif

    for (NestedNameSpecifierListAST *it = ast->nested_name_specifier_list; it; it = it->next) {
        NestedNameSpecifierAST *nested_name_specifier = it->value;

        if (NameAST *class_or_namespace_name = nested_name_specifier->class_or_namespace_name) {
            SimpleNameAST *simple_name = class_or_namespace_name->asSimpleName();

            TemplateIdAST *template_id = 0;
            if (! simple_name) {
                template_id = class_or_namespace_name->asTemplateId();

                if (template_id) {
                    for (ExpressionListAST *arg_it = template_id->template_argument_list; arg_it; arg_it = arg_it->next) {
                        this->expression(arg_it->value);
                    }
                }
            }

            if (simple_name || template_id) {
                const unsigned identifier_token = simple_name
                           ? simple_name->identifier_token
                           : template_id->identifier_token;

                if (identifier(identifier_token) == _id)
                    checkExpression(ast->firstToken(), identifier_token);
            }
        }
    }

    if (NameAST *unqualified_name = ast->unqualified_name) {
        unsigned identifier_token = 0;

        if (SimpleNameAST *simple_name = unqualified_name->asSimpleName())
            identifier_token = simple_name->identifier_token;
        else if (DestructorNameAST *dtor = unqualified_name->asDestructorName())
            identifier_token = dtor->unqualified_name->firstToken();

        TemplateIdAST *template_id = 0;
        if (! identifier_token) {
            template_id = unqualified_name->asTemplateId();

            if (template_id) {
                identifier_token = template_id->identifier_token;

                for (ExpressionListAST *template_arguments = template_id->template_argument_list;
                     template_arguments; template_arguments = template_arguments->next) {
                    this->expression(template_arguments->value);
                }
            }
        }

        if (identifier_token && identifier(identifier_token) == _id)
            checkExpression(ast->firstToken(), identifier_token);
    }

    return false;

    return false;
}

bool FindUsages::visit(OperatorFunctionIdAST *ast)
{
    // unsigned operator_token = ast->operator_token;
    this->cppOperator(ast->op);
    return false;
}

bool FindUsages::visit(ConversionFunctionIdAST *ast)
{
    // unsigned operator_token = ast->operator_token;
    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
        this->specifier(it->value);
    }
    for (PtrOperatorListAST *it = ast->ptr_operator_list; it; it = it->next) {
        this->ptrOperator(it->value);
    }
    return false;
}

bool FindUsages::visit(SimpleNameAST *ast)
{
    // unsigned identifier_token = ast->identifier_token;
    reportResult(ast->identifier_token, ast->name);
    return false;
}

bool FindUsages::visit(TemplateIdAST *ast)
{
    // unsigned identifier_token = ast->identifier_token;
    reportResult(ast->identifier_token, ast->name);
    // unsigned less_token = ast->less_token;
    for (ExpressionListAST *it = ast->template_argument_list; it; it = it->next) {
        this->expression(it->value);
    }
    // unsigned greater_token = ast->greater_token;
    return false;
}


// SpecifierAST
bool FindUsages::visit(SimpleSpecifierAST *ast)
{
    (void) ast;
    // unsigned specifier_token = ast->specifier_token;
    return false;
}

bool FindUsages::visit(AttributeSpecifierAST *ast)
{
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

bool FindUsages::visit(TypeofSpecifierAST *ast)
{
    // unsigned typeof_token = ast->typeof_token;
    // unsigned lparen_token = ast->lparen_token;
    this->expression(ast->expression);
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool FindUsages::visit(DecltypeSpecifierAST *ast)
{
    // unsigned typeof_token = ast->typeof_token;
    // unsigned lparen_token = ast->lparen_token;
    this->expression(ast->expression);
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool FindUsages::visit(ClassSpecifierAST *ast)
{
    // unsigned classkey_token = ast->classkey_token;
    for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
        this->specifier(it->value);
    }
    /*const Name *name =*/ this->name(ast->name);

    Scope *previousScope = switchScope(ast->symbol);

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
    (void) switchScope(previousScope);
    return false;
}

bool FindUsages::visit(NamedTypeSpecifierAST *ast)
{
    /*const Name *name =*/ this->name(ast->name);
    return false;
}

bool FindUsages::visit(ElaboratedTypeSpecifierAST *ast)
{
    // unsigned classkey_token = ast->classkey_token;
    for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
        this->specifier(it->value);
    }
    /*const Name *name =*/ this->name(ast->name);
    return false;
}

bool FindUsages::visit(EnumSpecifierAST *ast)
{
    // unsigned enum_token = ast->enum_token;
    /*const Name *name =*/ this->name(ast->name);
    // unsigned lbrace_token = ast->lbrace_token;
    Scope *previousScope = switchScope(ast->symbol);
    for (EnumeratorListAST *it = ast->enumerator_list; it; it = it->next) {
        this->enumerator(it->value);
    }
    // unsigned rbrace_token = ast->rbrace_token;
    // Enum *symbol = ast->symbol;
    (void) switchScope(previousScope);
    return false;
}


// PtrOperatorAST
bool FindUsages::visit(PointerToMemberAST *ast)
{
    // unsigned global_scope_token = ast->global_scope_token;
    for (NestedNameSpecifierListAST *it = ast->nested_name_specifier_list; it; it = it->next) {
        this->nestedNameSpecifier(it->value);
    }
    // unsigned star_token = ast->star_token;
    for (SpecifierListAST *it = ast->cv_qualifier_list; it; it = it->next) {
        this->specifier(it->value);
    }
    return false;
}

bool FindUsages::visit(PointerAST *ast)
{
    // unsigned star_token = ast->star_token;
    for (SpecifierListAST *it = ast->cv_qualifier_list; it; it = it->next) {
        this->specifier(it->value);
    }
    return false;
}

bool FindUsages::visit(ReferenceAST *ast)
{
    (void) ast;
    // unsigned reference_token = ast->reference_token;
    return false;
}


// PostfixAST
bool FindUsages::visit(CallAST *ast)
{
    this->expression(ast->base_expression);
    // unsigned lparen_token = ast->lparen_token;
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        this->expression(it->value);
    }
    // unsigned rparen_token = ast->rparen_token;
    return false;
}

bool FindUsages::visit(ArrayAccessAST *ast)
{
    this->expression(ast->base_expression);
    // unsigned lbracket_token = ast->lbracket_token;
    this->expression(ast->expression);
    // unsigned rbracket_token = ast->rbracket_token;
    return false;
}

bool FindUsages::visit(PostIncrDecrAST *ast)
{
    this->expression(ast->base_expression);
    // unsigned incr_decr_token = ast->incr_decr_token;
    return false;
}

bool FindUsages::visit(MemberAccessAST *ast)
{
    this->expression(ast->base_expression);
    // unsigned access_token = ast->access_token;
    // unsigned template_token = ast->template_token;

    if (ast->member_name) {
        if (SimpleNameAST *simple = ast->member_name->asSimpleName()) {
            if (identifier(simple->identifier_token) == _id) {
                checkExpression(ast->firstToken(), simple->identifier_token);
                return false;
            }
        }
    }

    return false;
}


// CoreDeclaratorAST
bool FindUsages::visit(DeclaratorIdAST *ast)
{
    // unsigned dot_dot_dot_token = ast->dot_dot_dot_token;
    /*const Name *name =*/ this->name(ast->name);
    return false;
}

bool FindUsages::visit(NestedDeclaratorAST *ast)
{
    // unsigned lparen_token = ast->lparen_token;
    this->declarator(ast->declarator);
    // unsigned rparen_token = ast->rparen_token;
    return false;
}


// PostfixDeclaratorAST
bool FindUsages::visit(FunctionDeclaratorAST *ast)
{
    // unsigned lparen_token = ast->lparen_token;
    this->parameterDeclarationClause(ast->parameter_declaration_clause);
    // unsigned rparen_token = ast->rparen_token;
    for (SpecifierListAST *it = ast->cv_qualifier_list; it; it = it->next) {
        this->specifier(it->value);
    }
    this->exceptionSpecification(ast->exception_specification);
    this->trailingReturnType(ast->trailing_return_type);
    this->expression(ast->as_cpp_initializer);
    // Function *symbol = ast->symbol;
    return false;
}

bool FindUsages::visit(ArrayDeclaratorAST *ast)
{
    // unsigned lbracket_token = ast->lbracket_token;
    this->expression(ast->expression);
    // unsigned rbracket_token = ast->rbracket_token;
    return false;
}

void FindUsages::prepareLines(const QByteArray &bytes)
{
    _sourceLineEnds.reserve(1000);
    const char *s = bytes.constData();
    _sourceLineEnds.push_back(s - 1); // we start counting at line 1, so line 0 is always empty.

    for (; *s; ++s)
        if (*s == '\n')
            _sourceLineEnds.push_back(s);
    if (s != _sourceLineEnds.back() + 1) // no newline at the end of the file
        _sourceLineEnds.push_back(s);
}

QString FindUsages::fetchLine(unsigned lineNr) const
{
    Q_ASSERT(lineNr < _sourceLineEnds.size());
    if (lineNr == 0)
        return QString();

    const char *start = _sourceLineEnds.at(lineNr - 1) + 1;
    const char *end = _sourceLineEnds.at(lineNr);
    return QString::fromUtf8(start, end - start);
}

