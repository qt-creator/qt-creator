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

#include <QtGui/QApplication>
#include <QtCore/QDebug>

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
    , _allowAnyProperty(false)
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

    const bool oldAllowAnyProperty = _allowAnyProperty;

    if (! _context.lookupType(_doc.data(), typeId)) {
        warning(typeId->identifierToken, tr(Messages::unknown_type));
        _allowAnyProperty = true; // suppress subsequent "unknown property" errors
    }

    QList<const ObjectValue *> oldScopeObjects = _context.scopeChain().qmlScopeObjects;

    _context.scopeChain().qmlScopeObjects.clear();
    const ObjectValue *scopeObject = _doc->bind()->findQmlObject(ast);
    _context.scopeChain().qmlScopeObjects += scopeObject;
    _context.scopeChain().update();

#ifndef NO_DECLARATIVE_BACKEND
    // check if the object has a Qt.ListElement ancestor
    const ObjectValue *prototype = scopeObject->prototype(&_context);
    while (prototype) {
        if (const QmlObjectValue *qmlMetaObject = dynamic_cast<const QmlObjectValue *>(prototype)) {
            // ### Also check for Qt package. Involves changes in QmlObjectValue.
            if (qmlMetaObject->qmlTypeName() == QLatin1String("ListElement")) {
                _allowAnyProperty = true;
                break;
            }
        }
        prototype = prototype->prototype(&_context);
    }

    // check if the object has a Qt.PropertyChanges ancestor
    prototype = scopeObject->prototype(&_context);
    while (prototype) {
        if (const QmlObjectValue *qmlMetaObject = dynamic_cast<const QmlObjectValue *>(prototype)) {
            // ### Also check for Qt package. Involves changes in QmlObjectValue.
            if (qmlMetaObject->qmlTypeName() == QLatin1String("PropertyChanges"))
                break;
        }
        prototype = prototype->prototype(&_context);
    }
    // find the target script binding
    if (prototype && initializer) {
        for (UiObjectMemberList *m = initializer->members; m; m = m->next) {
            if (UiScriptBinding *scriptBinding = cast<UiScriptBinding *>(m->member)) {
                if (scriptBinding->qualifiedId
                        && scriptBinding->qualifiedId->name->asString() == QLatin1String("target")
                        && ! scriptBinding->qualifiedId->next) {
                    if (ExpressionStatement *expStmt = cast<ExpressionStatement *>(scriptBinding->statement)) {
                        Evaluate evaluator(&_context);
                        const Value *targetValue = evaluator(expStmt->expression);

                        if (const ObjectValue *target = value_cast<const ObjectValue *>(targetValue)) {
                            _context.scopeChain().qmlScopeObjects.prepend(target);
                        } else {
                            _allowAnyProperty = true;
                        }
                    }
                }
            }
        }
    }
#endif

    Node::accept(initializer, this);

    _context.scopeChain().qmlScopeObjects = oldScopeObjects;
    _allowAnyProperty = oldAllowAnyProperty;
}

bool Check::visit(UiScriptBinding *ast)
{
    const Value *lhsValue = checkScopeObjectMember(ast->qualifiedId);
    if (lhsValue) {
        // ### Fix the evaluator to accept statements!
        if (ExpressionStatement *expStmt = cast<ExpressionStatement *>(ast->statement)) {
            Evaluate evaluator(&_context);
            const Value *rhsValue = evaluator(expStmt->expression);

            const SourceLocation loc = locationFromRange(expStmt->firstSourceLocation(), expStmt->lastSourceLocation());
            checkPropertyAssignment(loc, lhsValue, rhsValue, expStmt->expression);
        }

    }

    return true;
}

void Check::checkPropertyAssignment(const SourceLocation &location,
                                    const Interpreter::Value *lhsValue,
                                    const Interpreter::Value *rhsValue,
                                    ExpressionNode *ast)
{
    if (lhsValue->asEasingCurveNameValue()) {
        const StringValue *rhsStringValue = rhsValue->asStringValue();
        if (!rhsStringValue) {
            if (rhsValue->asUndefinedValue())
                warning(location, tr(Messages::value_might_be_undefined));
            else
                error(location, tr(Messages::easing_curve_not_a_string));
            return;
        }

        if (StringLiteral *string = cast<StringLiteral *>(ast)) {
            const QString value = string->value->asString();
            // ### do something with easing-curve attributes.
            // ### Incomplete documentation at: http://qt.nokia.com/doc/4.7-snapshot/qml-propertyanimation.html#easing-prop
            // ### The implementation is at: src/declarative/util/qmlanimation.cpp
            const QString curveName = value.left(value.indexOf(QLatin1Char('(')));
            if (!EasingCurveNameValue::curveNames().contains(curveName)) {
                error(location, tr(Messages::unknown_easing_curve_name));
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
    if (_allowAnyProperty)
        return 0;

    QList<const ObjectValue *> scopeObjects = _context.scopeChain().qmlScopeObjects;

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
