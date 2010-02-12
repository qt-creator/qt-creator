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

#include "qmljscheck.h"
#include "qmljsinterpreter.h"
#include "parser/qmljsparser_p.h"
#include "parser/qmljsast_p.h"
#include <QtCore/QDebug>

using namespace QmlJS;
using namespace QmlJS::Interpreter;

Check::Check(Context *context)
    : _engine(context->engine()),
      _context(context),
      _scope(_engine->globalObject()),
      _result(0)
{
}

Check::~Check()
{
}

const Interpreter::Value *Check::operator()(AST::Node *ast)
{
    const Value *result = reference(ast);

    if (const Reference *ref = value_cast<const Reference *>(result))
        result = ref->value(_context);

    if (! result)
        result = _engine->undefinedValue();

    return result;
}

const Interpreter::Value *Check::reference(AST::Node *ast)
{
    // save the result
    const Value *previousResult = switchResult(0);

    // process the expression
    accept(ast);

    // restore the previous result
    return switchResult(previousResult);
}

Interpreter::Engine *Check::switchEngine(Interpreter::Engine *engine)
{
    Interpreter::Engine *previousEngine = _engine;
    _engine = engine;
    return previousEngine;
}

const Interpreter::Value *Check::switchResult(const Interpreter::Value *result)
{
    const Interpreter::Value *previousResult = _result;
    _result = result;
    return previousResult;
}

const Interpreter::ObjectValue *Check::switchScope(const Interpreter::ObjectValue *scope)
{
    const Interpreter::ObjectValue *previousScope = _scope;
    _scope = scope;
    return previousScope;
}

void Check::accept(AST::Node *node)
{
    AST::Node::accept(node, this);
}

bool Check::visit(AST::UiProgram *)
{
    return false;
}

bool Check::visit(AST::UiImportList *)
{
    return false;
}

bool Check::visit(AST::UiImport *)
{
    return false;
}

bool Check::visit(AST::UiPublicMember *)
{
    return false;
}

bool Check::visit(AST::UiSourceElement *)
{
    return false;
}

bool Check::visit(AST::UiObjectDefinition *)
{
    return false;
}

bool Check::visit(AST::UiObjectInitializer *)
{
    return false;
}

bool Check::visit(AST::UiObjectBinding *)
{
    return false;
}

bool Check::visit(AST::UiScriptBinding *)
{
    return false;
}

bool Check::visit(AST::UiArrayBinding *)
{
    return false;
}

bool Check::visit(AST::UiObjectMemberList *)
{
    return false;
}

bool Check::visit(AST::UiArrayMemberList *)
{
    return false;
}

bool Check::visit(AST::UiQualifiedId *ast)
{
    if (! ast->name)
         return false;

    const Value *value = _context->lookup(ast->name->asString());
    if (! ast->next) {
        _result = value;

    } else {
        const ObjectValue *base = value_cast<const ObjectValue *>(value);

        for (AST::UiQualifiedId *it = ast->next; base && it; it = it->next) {
            NameId *name = it->name;
            if (! name)
                break;

            const Value *value = base->property(name->asString(), _context);
            if (! it->next)
                _result = value;
            else
                base = value_cast<const ObjectValue *>(value);
        }
    }

    return false;
}

bool Check::visit(AST::UiSignature *)
{
    return false;
}

bool Check::visit(AST::UiFormalList *)
{
    return false;
}

bool Check::visit(AST::UiFormal *)
{
    return false;
}

bool Check::visit(AST::ThisExpression *)
{
    return false;
}

bool Check::visit(AST::IdentifierExpression *ast)
{
    if (! ast->name)
        return false;

    _result = _context->lookup(ast->name->asString());
    return false;
}

bool Check::visit(AST::NullExpression *)
{
    _result = _engine->nullValue();
    return false;
}

bool Check::visit(AST::TrueLiteral *)
{
    _result = _engine->booleanValue();
    return false;
}

bool Check::visit(AST::FalseLiteral *)
{
    _result = _engine->booleanValue();
    return false;
}

bool Check::visit(AST::StringLiteral *)
{
    _result = _engine->stringValue();
    return false;
}

bool Check::visit(AST::NumericLiteral *)
{
    _result = _engine->numberValue();
    return false;
}

bool Check::visit(AST::RegExpLiteral *)
{
    _result = _engine->regexpCtor()->construct();
    return false;
}

bool Check::visit(AST::ArrayLiteral *)
{
    _result = _engine->arrayCtor()->construct();
    return false;
}

bool Check::visit(AST::ObjectLiteral *)
{
    return false;
}

bool Check::visit(AST::ElementList *)
{
    return false;
}

bool Check::visit(AST::Elision *)
{
    return false;
}

bool Check::visit(AST::PropertyNameAndValueList *)
{
    return false;
}

bool Check::visit(AST::NestedExpression *)
{
    return true; // visit the child expression
}

bool Check::visit(AST::IdentifierPropertyName *)
{
    return false;
}

bool Check::visit(AST::StringLiteralPropertyName *)
{
    return false;
}

bool Check::visit(AST::NumericLiteralPropertyName *)
{
    return false;
}

bool Check::visit(AST::ArrayMemberExpression *)
{
    return false;
}

bool Check::visit(AST::FieldMemberExpression *ast)
{
    if (! ast->name)
        return false;

    if (const Interpreter::Value *base = _engine->convertToObject(reference(ast->base))) {
        if (const Interpreter::ObjectValue *obj = base->asObjectValue()) {
            _result = obj->property(ast->name->asString(), _context);
        }
    }

    return false;
}

bool Check::visit(AST::NewMemberExpression *ast)
{
    if (const FunctionValue *ctor = value_cast<const FunctionValue *>(reference(ast->base))) {
        _result = ctor->construct();
    }
    return false;
}

bool Check::visit(AST::NewExpression *ast)
{
    if (const FunctionValue *ctor = value_cast<const FunctionValue *>(reference(ast->expression))) {
        _result = ctor->construct();
    }
    return false;
}

bool Check::visit(AST::CallExpression *ast)
{
    if (const Interpreter::Value *base = reference(ast->base)) {
        if (const Interpreter::FunctionValue *obj = base->asFunctionValue()) {
            _result = obj->returnValue();
        }
    }
    return false;
}

bool Check::visit(AST::ArgumentList *)
{
    return false;
}

bool Check::visit(AST::PostIncrementExpression *)
{
    return false;
}

bool Check::visit(AST::PostDecrementExpression *)
{
    return false;
}

bool Check::visit(AST::DeleteExpression *)
{
    return false;
}

bool Check::visit(AST::VoidExpression *)
{
    return false;
}

bool Check::visit(AST::TypeOfExpression *)
{
    return false;
}

bool Check::visit(AST::PreIncrementExpression *)
{
    return false;
}

bool Check::visit(AST::PreDecrementExpression *)
{
    return false;
}

bool Check::visit(AST::UnaryPlusExpression *)
{
    return false;
}

bool Check::visit(AST::UnaryMinusExpression *)
{
    return false;
}

bool Check::visit(AST::TildeExpression *)
{
    return false;
}

bool Check::visit(AST::NotExpression *)
{
    return false;
}

bool Check::visit(AST::BinaryExpression *)
{
    return false;
}

bool Check::visit(AST::ConditionalExpression *)
{
    return false;
}

bool Check::visit(AST::Expression *)
{
    return false;
}

bool Check::visit(AST::Block *)
{
    return false;
}

bool Check::visit(AST::StatementList *)
{
    return false;
}

bool Check::visit(AST::VariableStatement *)
{
    return false;
}

bool Check::visit(AST::VariableDeclarationList *)
{
    return false;
}

bool Check::visit(AST::VariableDeclaration *)
{
    return false;
}

bool Check::visit(AST::EmptyStatement *)
{
    return false;
}

bool Check::visit(AST::ExpressionStatement *)
{
    return false;
}

bool Check::visit(AST::IfStatement *)
{
    return false;
}

bool Check::visit(AST::DoWhileStatement *)
{
    return false;
}

bool Check::visit(AST::WhileStatement *)
{
    return false;
}

bool Check::visit(AST::ForStatement *)
{
    return false;
}

bool Check::visit(AST::LocalForStatement *)
{
    return false;
}

bool Check::visit(AST::ForEachStatement *)
{
    return false;
}

bool Check::visit(AST::LocalForEachStatement *)
{
    return false;
}

bool Check::visit(AST::ContinueStatement *)
{
    return false;
}

bool Check::visit(AST::BreakStatement *)
{
    return false;
}

bool Check::visit(AST::ReturnStatement *)
{
    return false;
}

bool Check::visit(AST::WithStatement *)
{
    return false;
}

bool Check::visit(AST::SwitchStatement *)
{
    return false;
}

bool Check::visit(AST::CaseBlock *)
{
    return false;
}

bool Check::visit(AST::CaseClauses *)
{
    return false;
}

bool Check::visit(AST::CaseClause *)
{
    return false;
}

bool Check::visit(AST::DefaultClause *)
{
    return false;
}

bool Check::visit(AST::LabelledStatement *)
{
    return false;
}

bool Check::visit(AST::ThrowStatement *)
{
    return false;
}

bool Check::visit(AST::TryStatement *)
{
    return false;
}

bool Check::visit(AST::Catch *)
{
    return false;
}

bool Check::visit(AST::Finally *)
{
    return false;
}

bool Check::visit(AST::FunctionDeclaration *)
{
    return false;
}

bool Check::visit(AST::FunctionExpression *)
{
    return false;
}

bool Check::visit(AST::FormalParameterList *)
{
    return false;
}

bool Check::visit(AST::FunctionBody *)
{
    return false;
}

bool Check::visit(AST::Program *)
{
    return false;
}

bool Check::visit(AST::SourceElements *)
{
    return false;
}

bool Check::visit(AST::FunctionSourceElement *)
{
    return false;
}

bool Check::visit(AST::StatementSourceElement *)
{
    return false;
}

bool Check::visit(AST::DebuggerStatement *)
{
    return false;
}
