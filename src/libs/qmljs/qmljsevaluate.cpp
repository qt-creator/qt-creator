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

#include "qmljsevaluate.h"
#include "qmljscontext.h"
#include "qmljsvalueowner.h"
#include "qmljsscopechain.h"
#include "parser/qmljsast_p.h"
#include <QDebug>

using namespace QmlJS;

/*!
    \class QmlJS::Evaluate
    \brief Evaluates \l{AST::Node}s to \l{Value}s.
    \sa Value ScopeChain

    The Evaluate visitor is constructed with a ScopeChain and accepts JavaScript
    expressions as well as some other AST::Nodes. It evaluates the expression in
    the given ScopeChain and returns a Value representing the result.

    Example: Pass in the AST for "1 + 2" and NumberValue will be returned.

    In normal cases only the call operator (or the equivalent value() method)
    will be used.

    The reference() method has the special behavior of not resolving \l{Reference}s
    which can be useful when interested in the identity of a variable instead
    of its value.

    Example: In a scope where "var a = 1"
    \list
    \o value(Identifier-a) will return NumberValue
    \o reference(Identifier-a) will return the ASTVariableReference for the declaration of "a"
    \endlist
*/

Evaluate::Evaluate(const ScopeChain *scopeChain, ReferenceContext *referenceContext)
    : _valueOwner(scopeChain->context()->valueOwner()),
      _context(scopeChain->context()),
      _referenceContext(referenceContext),
      _scopeChain(scopeChain),
      _result(0)
{
}

Evaluate::~Evaluate()
{
}

const Value *Evaluate::operator()(AST::Node *ast)
{
    return value(ast);
}

const Value *Evaluate::value(AST::Node *ast)
{
    const Value *result = reference(ast);

    if (const Reference *ref = value_cast<Reference>(result)) {
        if (_referenceContext)
            result = _referenceContext->lookupReference(ref);
        else
            result = _context->lookupReference(ref);
    }

    // if evaluation fails, return an unknown value
    if (! result)
        result = _valueOwner->unknownValue();

    return result;
}

const Value *Evaluate::reference(AST::Node *ast)
{
    // save the result
    const Value *previousResult = switchResult(0);

    // process the expression
    accept(ast);

    // restore the previous result
    return switchResult(previousResult);
}

const Value *Evaluate::switchResult(const Value *result)
{
    const Value *previousResult = _result;
    _result = result;
    return previousResult;
}

void Evaluate::accept(AST::Node *node)
{
    AST::Node::accept(node, this);
}

bool Evaluate::visit(AST::UiProgram *)
{
    return false;
}

bool Evaluate::visit(AST::UiImportList *)
{
    return false;
}

bool Evaluate::visit(AST::UiImport *)
{
    return false;
}

bool Evaluate::visit(AST::UiPublicMember *)
{
    return false;
}

bool Evaluate::visit(AST::UiSourceElement *)
{
    return false;
}

bool Evaluate::visit(AST::UiObjectDefinition *)
{
    return false;
}

bool Evaluate::visit(AST::UiObjectInitializer *)
{
    return false;
}

bool Evaluate::visit(AST::UiObjectBinding *)
{
    return false;
}

bool Evaluate::visit(AST::UiScriptBinding *)
{
    return false;
}

bool Evaluate::visit(AST::UiArrayBinding *)
{
    return false;
}

bool Evaluate::visit(AST::UiObjectMemberList *)
{
    return false;
}

bool Evaluate::visit(AST::UiArrayMemberList *)
{
    return false;
}

bool Evaluate::visit(AST::UiQualifiedId *ast)
{
    if (ast->name.isEmpty())
         return false;

    const Value *value = _scopeChain->lookup(ast->name.toString());
    if (! ast->next) {
        _result = value;

    } else {
        const ObjectValue *base = value_cast<ObjectValue>(value);

        for (AST::UiQualifiedId *it = ast->next; base && it; it = it->next) {
            const QString &name = it->name.toString();
            if (name.isEmpty())
                break;

            const Value *value = base->lookupMember(name, _context);
            if (! it->next)
                _result = value;
            else
                base = value_cast<ObjectValue>(value);
        }
    }

    return false;
}

bool Evaluate::visit(AST::ThisExpression *)
{
    return false;
}

bool Evaluate::visit(AST::IdentifierExpression *ast)
{
    if (ast->name.isEmpty())
        return false;

    _result = _scopeChain->lookup(ast->name.toString());
    return false;
}

bool Evaluate::visit(AST::NullExpression *)
{
    _result = _valueOwner->nullValue();
    return false;
}

bool Evaluate::visit(AST::TrueLiteral *)
{
    _result = _valueOwner->booleanValue();
    return false;
}

bool Evaluate::visit(AST::FalseLiteral *)
{
    _result = _valueOwner->booleanValue();
    return false;
}

bool Evaluate::visit(AST::StringLiteral *)
{
    _result = _valueOwner->stringValue();
    return false;
}

bool Evaluate::visit(AST::NumericLiteral *)
{
    _result = _valueOwner->numberValue();
    return false;
}

bool Evaluate::visit(AST::RegExpLiteral *)
{
    _result = _valueOwner->regexpCtor()->returnValue();
    return false;
}

bool Evaluate::visit(AST::ArrayLiteral *)
{
    _result = _valueOwner->arrayCtor()->returnValue();
    return false;
}

bool Evaluate::visit(AST::ObjectLiteral *)
{
    // ### properties
    _result = _valueOwner->newObject();
    return false;
}

bool Evaluate::visit(AST::ElementList *)
{
    return false;
}

bool Evaluate::visit(AST::Elision *)
{
    return false;
}

bool Evaluate::visit(AST::PropertyNameAndValueList *)
{
    return false;
}

bool Evaluate::visit(AST::NestedExpression *)
{
    return true; // visit the child expression
}

bool Evaluate::visit(AST::IdentifierPropertyName *)
{
    return false;
}

bool Evaluate::visit(AST::StringLiteralPropertyName *)
{
    return false;
}

bool Evaluate::visit(AST::NumericLiteralPropertyName *)
{
    return false;
}

bool Evaluate::visit(AST::ArrayMemberExpression *)
{
    return false;
}

bool Evaluate::visit(AST::FieldMemberExpression *ast)
{
    if (ast->name.isEmpty())
        return false;

    if (const Value *base = _valueOwner->convertToObject(value(ast->base))) {
        if (const ObjectValue *obj = base->asObjectValue())
            _result = obj->lookupMember(ast->name.toString(), _context);
    }

    return false;
}

bool Evaluate::visit(AST::NewMemberExpression *ast)
{
    if (const FunctionValue *ctor = value_cast<FunctionValue>(value(ast->base)))
        _result = ctor->returnValue();
    return false;
}

bool Evaluate::visit(AST::NewExpression *ast)
{
    if (const FunctionValue *ctor = value_cast<FunctionValue>(value(ast->expression)))
        _result = ctor->returnValue();
    return false;
}

bool Evaluate::visit(AST::CallExpression *ast)
{
    if (const Value *base = value(ast->base)) {
        if (const FunctionValue *obj = base->asFunctionValue())
            _result = obj->returnValue();
    }
    return false;
}

bool Evaluate::visit(AST::ArgumentList *)
{
    return false;
}

bool Evaluate::visit(AST::PostIncrementExpression *)
{
    _result = _valueOwner->numberValue();
    return false;
}

bool Evaluate::visit(AST::PostDecrementExpression *)
{
    _result = _valueOwner->numberValue();
    return false;
}

bool Evaluate::visit(AST::DeleteExpression *)
{
    _result = _valueOwner->booleanValue();
    return false;
}

bool Evaluate::visit(AST::VoidExpression *)
{
    _result = _valueOwner->undefinedValue();
    return false;
}

bool Evaluate::visit(AST::TypeOfExpression *)
{
    _result = _valueOwner->stringValue();
    return false;
}

bool Evaluate::visit(AST::PreIncrementExpression *)
{
    _result = _valueOwner->numberValue();
    return false;
}

bool Evaluate::visit(AST::PreDecrementExpression *)
{
    _result = _valueOwner->numberValue();
    return false;
}

bool Evaluate::visit(AST::UnaryPlusExpression *)
{
    _result = _valueOwner->numberValue();
    return false;
}

bool Evaluate::visit(AST::UnaryMinusExpression *)
{
    _result = _valueOwner->numberValue();
    return false;
}

bool Evaluate::visit(AST::TildeExpression *)
{
    _result = _valueOwner->numberValue();
    return false;
}

bool Evaluate::visit(AST::NotExpression *)
{
    _result = _valueOwner->booleanValue();
    return false;
}

bool Evaluate::visit(AST::BinaryExpression *ast)
{
    const Value *lhs = 0;
    const Value *rhs = 0;
    switch (ast->op) {
    case QSOperator::Add:
    case QSOperator::InplaceAdd:
    //case QSOperator::And: // ### enable once implemented below
    //case QSOperator::Or:
        lhs = value(ast->left);
        // fallthrough
    case QSOperator::Assign:
        rhs = value(ast->right);
        break;
    default:
        break;
    }

    switch (ast->op) {
    case QSOperator::Add:
    case QSOperator::InplaceAdd:
        if (lhs->asStringValue() || rhs->asStringValue())
            _result = _valueOwner->stringValue();
        else
            _result = _valueOwner->numberValue();
        break;

    case QSOperator::Sub:
    case QSOperator::InplaceSub:
    case QSOperator::Mul:
    case QSOperator::InplaceMul:
    case QSOperator::Div:
    case QSOperator::InplaceDiv:
    case QSOperator::Mod:
    case QSOperator::InplaceMod:
    case QSOperator::BitAnd:
    case QSOperator::InplaceAnd:
    case QSOperator::BitXor:
    case QSOperator::InplaceXor:
    case QSOperator::BitOr:
    case QSOperator::InplaceOr:
    case QSOperator::LShift:
    case QSOperator::InplaceLeftShift:
    case QSOperator::RShift:
    case QSOperator::InplaceRightShift:
    case QSOperator::URShift:
    case QSOperator::InplaceURightShift:
        _result = _valueOwner->numberValue();
        break;

    case QSOperator::Le:
    case QSOperator::Ge:
    case QSOperator::Lt:
    case QSOperator::Gt:
    case QSOperator::Equal:
    case QSOperator::NotEqual:
    case QSOperator::StrictEqual:
    case QSOperator::StrictNotEqual:
    case QSOperator::InstanceOf:
    case QSOperator::In:
        _result = _valueOwner->booleanValue();
        break;

    case QSOperator::And:
    case QSOperator::Or:
        // ### either lhs or rhs
        _result = _valueOwner->unknownValue();
        break;

    case QSOperator::Assign:
        _result = rhs;
        break;

    default:
        break;
    }

    return false;
}

bool Evaluate::visit(AST::ConditionalExpression *)
{
    return false;
}

bool Evaluate::visit(AST::Expression *)
{
    return false;
}

bool Evaluate::visit(AST::Block *)
{
    return false;
}

bool Evaluate::visit(AST::StatementList *)
{
    return false;
}

bool Evaluate::visit(AST::VariableStatement *)
{
    return false;
}

bool Evaluate::visit(AST::VariableDeclarationList *)
{
    return false;
}

bool Evaluate::visit(AST::VariableDeclaration *)
{
    return false;
}

bool Evaluate::visit(AST::EmptyStatement *)
{
    return false;
}

bool Evaluate::visit(AST::ExpressionStatement *)
{
    return true;
}

bool Evaluate::visit(AST::IfStatement *)
{
    return false;
}

bool Evaluate::visit(AST::DoWhileStatement *)
{
    return false;
}

bool Evaluate::visit(AST::WhileStatement *)
{
    return false;
}

bool Evaluate::visit(AST::ForStatement *)
{
    return false;
}

bool Evaluate::visit(AST::LocalForStatement *)
{
    return false;
}

bool Evaluate::visit(AST::ForEachStatement *)
{
    return false;
}

bool Evaluate::visit(AST::LocalForEachStatement *)
{
    return false;
}

bool Evaluate::visit(AST::ContinueStatement *)
{
    return false;
}

bool Evaluate::visit(AST::BreakStatement *)
{
    return false;
}

bool Evaluate::visit(AST::ReturnStatement *)
{
    return true;
}

bool Evaluate::visit(AST::WithStatement *)
{
    return false;
}

bool Evaluate::visit(AST::SwitchStatement *)
{
    return false;
}

bool Evaluate::visit(AST::CaseBlock *)
{
    return false;
}

bool Evaluate::visit(AST::CaseClauses *)
{
    return false;
}

bool Evaluate::visit(AST::CaseClause *)
{
    return false;
}

bool Evaluate::visit(AST::DefaultClause *)
{
    return false;
}

bool Evaluate::visit(AST::LabelledStatement *)
{
    return false;
}

bool Evaluate::visit(AST::ThrowStatement *)
{
    return false;
}

bool Evaluate::visit(AST::TryStatement *)
{
    return false;
}

bool Evaluate::visit(AST::Catch *)
{
    return false;
}

bool Evaluate::visit(AST::Finally *)
{
    return false;
}

bool Evaluate::visit(AST::FunctionDeclaration *)
{
    return false;
}

bool Evaluate::visit(AST::FunctionExpression *)
{
    return false;
}

bool Evaluate::visit(AST::FormalParameterList *)
{
    return false;
}

bool Evaluate::visit(AST::FunctionBody *)
{
    return false;
}

bool Evaluate::visit(AST::Program *)
{
    return false;
}

bool Evaluate::visit(AST::SourceElements *)
{
    return false;
}

bool Evaluate::visit(AST::FunctionSourceElement *)
{
    return false;
}

bool Evaluate::visit(AST::StatementSourceElement *)
{
    return false;
}

bool Evaluate::visit(AST::DebuggerStatement *)
{
    return false;
}
