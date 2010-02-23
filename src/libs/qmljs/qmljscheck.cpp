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
#include "qmljsbind.h"
#include "qmljsinterpreter.h"
#include "qmljsevaluate.h"
#include "parser/qmljsast_p.h"

#include <QtCore/QDebug>
#include <QtCore/QCoreApplication>
#include <QtGui/QColor>
#include <QtGui/QApplication>

#ifndef NO_DECLARATIVE_BACKEND
#  include <QtDeclarative/private/qmlstringconverters_p.h> // ### remove me
#endif

namespace QmlJS {
namespace Messages {
static const char *invalid_property_name =  QT_TRANSLATE_NOOP("QmlJS::Check", "'%1' is not a valid property name");
static const char *unknown_type = QT_TRANSLATE_NOOP("QmlJS::Check", "unknown type");
static const char *has_no_members = QT_TRANSLATE_NOOP("QmlJS::Check", "'%1' does not have members");
static const char *is_not_a_member = QT_TRANSLATE_NOOP("QmlJS::Check", "'%1' is not a member of '%2'");
static const char *easing_curve_not_a_string = QT_TRANSLATE_NOOP("QmlJS::Check", "easing-curve name is not a string");
static const char *unknown_easing_curve_name = QT_TRANSLATE_NOOP("QmlJS::Check", "unknown easing-curve name");
static const char *value_might_be_undefined = QT_TRANSLATE_NOOP("QmlJS::Check", "value might be 'undefined'");
} // namespace Messages

static inline QString tr(const char *msg)
{ return qApp->translate("QmlJS::Check", msg); }

} // namespace QmlJS

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJS::Interpreter;

Check::Check(Document::Ptr doc, const Snapshot &snapshot)
    : _doc(doc)
    , _snapshot(snapshot)
    , _context(&_engine)
    , _link(&_context, doc, snapshot)
    , _scopeBuilder(doc, &_context)
{
}

Check::~Check()
{
}

QList<DiagnosticMessage> Check::operator()()
{
    _messages.clear();
    Node::accept(_doc->ast(), this);
    return _messages;
}

bool Check::visit(UiProgram *)
{
    // build the initial scope chain
    _link.scopeChainAt(_doc);

    return true;
}

bool Check::visit(UiObjectDefinition *ast)
{
    visitQmlObject(ast, ast->qualifiedTypeNameId, ast->initializer);
    return false;
}

bool Check::visit(UiObjectBinding *ast)
{
    checkScopeObjectMember(ast->qualifiedId);

    visitQmlObject(ast, ast->qualifiedTypeNameId, ast->initializer);
    return false;
}

void Check::visitQmlObject(Node *ast, UiQualifiedId *typeId,
                           UiObjectInitializer *initializer)
{
    // If the 'typeId' starts with a lower-case letter, it doesn't define
    // a new object instance. For instance: anchors { ... }
    if (typeId->name->asString().at(0).isLower() && ! typeId->next) {
        checkScopeObjectMember(typeId);
        // ### don't give up!
        return;
    }

    _scopeBuilder.push(ast);

    if (! _context.lookupType(_doc.data(), typeId)) {
        warning(typeId->identifierToken, tr(Messages::unknown_type));
        // suppress subsequent errors about scope object lookup by clearing
        // the scope object list
        // ### todo: better way?
        _context.scopeChain().qmlScopeObjects.clear();
        _context.scopeChain().update();
    }

    Node::accept(initializer, this);

    _scopeBuilder.pop();
}

void Check::errorOnWrongRhs(const SourceLocation &loc, const Value *lhsValue)
{
    if (lhsValue->asBooleanValue()) {
        error(loc, QCoreApplication::translate("QmlJS::Check", "boolean value expected"));
    } else if (lhsValue->asNumberValue()) {
        error(loc, QCoreApplication::translate("QmlJS::Check", "numerical value expected"));
    } else if (lhsValue->asStringValue()) {
        error(loc, QCoreApplication::translate("QmlJS::Check", "string value expected"));
    }
}

bool Check::visit(UiScriptBinding *ast)
{
    // special case for id property
    if (ast->qualifiedId->name->asString() == QLatin1String("id") && ! ast->qualifiedId->next) {
        if (! ast->statement)
            return false;

        const SourceLocation loc = locationFromRange(ast->statement->firstSourceLocation(),
                                                     ast->statement->lastSourceLocation());

        ExpressionStatement *expStmt = cast<ExpressionStatement *>(ast->statement);
        if (!expStmt) {
            error(loc, QCoreApplication::translate("QmlJS::Check", "expected id"));
            return false;
        }

        IdentifierExpression *idExp = cast<IdentifierExpression *>(expStmt->expression);
        if (! idExp) {
            error(loc, QCoreApplication::translate("QmlJS::Check", "expected id"));
            return false;
        }

        if (! idExp->name->asString()[0].isLower()) {
            error(loc, QCoreApplication::translate("QmlJS::Check", "ids must be lower case"));
            return false;
        }
    }

    const Value *lhsValue = checkScopeObjectMember(ast->qualifiedId);
    if (lhsValue) {
        // ### Fix the evaluator to accept statements!
        if (ExpressionStatement *expStmt = cast<ExpressionStatement *>(ast->statement)) {
            ExpressionNode *expr = expStmt->expression;

            Evaluate evaluator(&_context);
            const Value *rhsValue = evaluator(expr);

            const SourceLocation loc = locationFromRange(expStmt->firstSourceLocation(),
                                                         expStmt->lastSourceLocation());
            checkPropertyAssignment(loc, lhsValue, rhsValue, expr);
        }

    }

    return true;
}

void Check::checkPropertyAssignment(const SourceLocation &location,
                                    const Interpreter::Value *lhsValue,
                                    const Interpreter::Value *rhsValue,
                                    ExpressionNode *ast)
{
    UnaryMinusExpression *unaryMinus = cast<UnaryMinusExpression *>(ast);

    // Qml is particularly strict with literals
    if (StringLiteral *stringLiteral = cast<StringLiteral *>(ast)) {
        const QString string = stringLiteral->value->asString();

        if (lhsValue->asStringValue()) {
            // okay
        } else if (lhsValue->asColorValue()) {
#ifndef NO_DECLARATIVE_BACKEND
            bool ok = false;
            QmlStringConverters::colorFromString(string, &ok);
            if (!ok)
                error(location, QCoreApplication::translate("QmlJS::Check", "not a valid color"));
#endif
        } else if (lhsValue->asEasingCurveNameValue()) {
            // ### do something with easing-curve attributes.
            // ### Incomplete documentation at: http://qt.nokia.com/doc/4.7-snapshot/qml-propertyanimation.html#easing-prop
            // ### The implementation is at: src/declarative/util/qmlanimation.cpp
            const QString curveName = string.left(string.indexOf(QLatin1Char('(')));
            if (!EasingCurveNameValue::curveNames().contains(curveName)) {
                error(location, tr(Messages::unknown_easing_curve_name));
            }
        } else {
            errorOnWrongRhs(location, lhsValue);
        }
    } else if ((ast->kind == Node::Kind_TrueLiteral
                || ast->kind == Node::Kind_FalseLiteral)
               && ! lhsValue->asBooleanValue()) {
        errorOnWrongRhs(location, lhsValue);
    } else if (cast<NumericLiteral *>(ast)
               && ! lhsValue->asNumberValue()) {
        errorOnWrongRhs(location, lhsValue);
    } else if (unaryMinus && cast<NumericLiteral *>(unaryMinus->expression)
               && ! lhsValue->asNumberValue()) {
        errorOnWrongRhs(location, lhsValue);
    } else {
        // rhs is not a literal
        if (lhsValue->asEasingCurveNameValue()) {
            const StringValue *rhsStringValue = rhsValue->asStringValue();
            if (!rhsStringValue) {
                if (rhsValue->asUndefinedValue())
                    warning(location, tr(Messages::value_might_be_undefined));
                else
                    error(location, tr(Messages::easing_curve_not_a_string));
                return;
            }
        }
    }
}

bool Check::visit(UiArrayBinding *ast)
{
    checkScopeObjectMember(ast->qualifiedId);

    return true;
}

const Value *Check::checkScopeObjectMember(const UiQualifiedId *id)
{
    QList<const ObjectValue *> scopeObjects = _context.scopeChain().qmlScopeObjects;
    if (scopeObjects.isEmpty())
        return 0;

    if (! id)
        return 0; // ### error?

    QString propertyName = id->name->asString();

    if (propertyName == QLatin1String("id") && ! id->next)
        return 0; // ### should probably be a special value

    // attached properties
    bool isAttachedProperty = false;
    if (! propertyName.isEmpty() && propertyName[0].isUpper()) {
        isAttachedProperty = true;
        scopeObjects += _context.scopeChain().qmlTypes;
    }

    if (scopeObjects.isEmpty())
        return 0;

    // global lookup for first part of id
    const Value *value = 0;
    for (int i = scopeObjects.size() - 1; i >= 0; --i) {
        value = scopeObjects[i]->lookupMember(propertyName, &_context);
        if (value)
            break;
    }
    if (!value) {
        error(id->identifierToken,
              tr(Messages::invalid_property_name).arg(propertyName));
    }

    // can't look up members for attached properties
    if (isAttachedProperty)
        return 0;

    // member lookup
    const UiQualifiedId *idPart = id;
    while (idPart->next) {
        const ObjectValue *objectValue = value_cast<const ObjectValue *>(value);
        if (! objectValue) {
            error(idPart->identifierToken,
                  tr(Messages::has_no_members).arg(propertyName));
            return 0;
        }

        idPart = idPart->next;
        propertyName = idPart->name->asString();

        value = objectValue->lookupMember(propertyName, &_context);
        if (! value) {
            error(idPart->identifierToken,
                  tr(Messages::is_not_a_member).arg(propertyName,
                                               objectValue->className()));
            return 0;
        }
    }

    return value;
}

void Check::error(const AST::SourceLocation &loc, const QString &message)
{
    _messages.append(DiagnosticMessage(DiagnosticMessage::Error, loc, message));
}

void Check::warning(const AST::SourceLocation &loc, const QString &message)
{
    _messages.append(DiagnosticMessage(DiagnosticMessage::Warning, loc, message));
}

SourceLocation Check::locationFromRange(const SourceLocation &start,
                                        const SourceLocation &end)
{
    return SourceLocation(start.offset,
                          end.end() - start.begin(),
                          start.startLine,
                          start.startColumn);
}
