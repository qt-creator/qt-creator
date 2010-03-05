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

#include "qmljsevaluate.h"
#include "qmljsinterpreter.h"
#include "parser/qmljsparser_p.h"
#include "parser/qmljsast_p.h"
#include <QtCore/QDebug>

using namespace QmlJS;
using namespace QmlJS::Interpreter;

Evaluate::Evaluate(Context *context)
    : _engine(context->engine()),
      _context(context),
      _scope(_engine->globalObject()),
      _result(0)
{
}

Evaluate::~Evaluate()
{
}

const Interpreter::Value *Evaluate::operator()(AST::Node *ast)
{
    const Value *result = reference(ast);

    if (const Reference *ref = value_cast<const Reference *>(result))
        result = ref->value(_context);

    if (! result)
        result = _engine->undefinedValue();

    return result;
}

const Interpreter::Value *Evaluate::reference(AST::Node *ast)
{
    // save the result
    const Value *previousResult = switchResult(0);

    // process the expression
    accept(ast);

    // restore the previous result
    return switchResult(previousResult);
}

Interpreter::Engine *Evaluate::switchEngine(Interpreter::Engine *engine)
{
    Interpreter::Engine *previousEngine = _engine;
    _engine = engine;
    return previousEngine;
}

const Interpreter::Value *Evaluate::switchResult(const Interpreter::Value *result)
{
    const Interpreter::Value *previousResult = _result;
    _result = result;
    return previousResult;
}

const Interpreter::ObjectValue *Evaluate::switchScope(const Interpreter::ObjectValue *scope)
{
    const Interpreter::ObjectValue *previousScope = _scope;
    _scope = scope;
    return previousScope;
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

bool Evaluate::visit(AST::UiSignature *)
{
    return false;
}

bool Evaluate::visit(AST::UiFormalList *)
{
    return false;
}

bool Evaluate::visit(AST::UiFormal *)
{
    return false;
}

bool Evaluate::visit(AST::ThisExpression *)
{
    return false;
}

bool Evaluate::visit(AST::IdentifierExpression *ast)
{
    if (! ast->name)
        return false;

    _result = _context->lookup(ast->name->asString());
    return false;
}

bool Evaluate::visit(AST::NullExpression *)
{
    _result = _engine->nullValue();
    return false;
}

bool Evaluate::visit(AST::TrueLiteral *)
{
    _result = _engine->booleanValue();
    return false;
}

bool Evaluate::visit(AST::FalseLiteral *)
{
    _result = _engine->booleanValue();
    return false;
}

bool Evaluate::visit(AST::StringLiteral *)
{
    _result = _engine->stringValue();
    return false;
}

bool Evaluate::visit(AST::NumericLiteral *)
{
    _result = _engine->numberValue();
    return false;
}

bool Evaluate::visit(AST::RegExpLiteral *)
{
    _result = _engine->regexpCtor()->construct();
    return false;
}

bool Evaluate::visit(AST::ArrayLiteral *)
{
    _result = _engine->arrayCtor()->construct();
    return false;
}

bool Evaluate::visit(AST::ObjectLiteral *)
{
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
    if (! ast->name)
        return false;

    if (const Interpreter::Value *base = _engine->convertToObject(reference(ast->base))) {
        if (const Interpreter::ObjectValue *obj = base->asObjectValue()) {
            _result = obj->property(ast->name->asString(), _context);
        }
    }

    return false;
}

bool Evaluate::visit(AST::NewMemberExpression *ast)
{
    if (const FunctionValue *ctor = value_cast<const FunctionValue *>(reference(ast->base))) {
        _result = ctor->construct();
    }
    return false;
}

bool Evaluate::visit(AST::NewExpression *ast)
{
    if (const FunctionValue *ctor = value_cast<const FunctionValue *>(reference(ast->expression))) {
        _result = ctor->construct();
    }
    return false;
}

bool Evaluate::visit(AST::CallExpression *ast)
{
    if (const Interpreter::Value *base = reference(ast->base)) {
        if (const Interpreter::FunctionValue *obj = base->asFunctionValue()) {
            _result = obj->returnValue();
        }
    }
    return false;
}

bool Evaluate::visit(AST::ArgumentList *)
{
    return false;
}

bool Evaluate::visit(AST::PostIncrementExpression *)
{
    return false;
}

bool Evaluate::visit(AST::PostDecrementExpression *)
{
    return false;
}

bool Evaluate::visit(AST::DeleteExpression *)
{
    return false;
}

bool Evaluate::visit(AST::VoidExpression *)
{
    return false;
}

bool Evaluate::visit(AST::TypeOfExpression *)
{
    return false;
}

bool Evaluate::visit(AST::PreIncrementExpression *)
{
    return false;
}

bool Evaluate::visit(AST::PreDecrementExpression *)
{
    return false;
}

bool Evaluate::visit(AST::UnaryPlusExpression *)
{
    return false;
}

bool Evaluate::visit(AST::UnaryMinusExpression *)
{
    return false;
}

bool Evaluate::visit(AST::TildeExpression *)
{
    return false;
}

bool Evaluate::visit(AST::NotExpression *)
{
    return false;
}

bool Evaluate::visit(AST::BinaryExpression *)
{
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
    return false;
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
    return false;
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
