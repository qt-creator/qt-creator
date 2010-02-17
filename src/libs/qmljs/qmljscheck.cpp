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

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJS::Interpreter;

Check::Check(Document::Ptr doc, const Snapshot &snapshot)
    : _doc(doc)
    , _snapshot(snapshot)
    , _context(&_engine)
    , _link(&_context, doc, snapshot)
    , _extraScope(0)
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

bool Check::visit(UiProgram *ast)
{
    // build the initial scope chain
    if (ast->members && ast->members->member)
        _link.scopeChainAt(_doc, ast->members->member);

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
    if (! _context.lookupType(_doc.data(), typeId)) {
        warning(typeId->identifierToken, QLatin1String("unknown type"));
        return;
    }

    const ObjectValue *oldScopeObject = _context.qmlScopeObject();
    const ObjectValue *oldExtraScope = _extraScope;
    const bool oldAllowAnyProperty = _allowAnyProperty;
    const ObjectValue *scopeObject = _doc->bind()->findQmlObject(ast);
    _context.setQmlScopeObject(scopeObject);

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
                            _extraScope = target;
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

    _context.setQmlScopeObject(oldScopeObject);
    _extraScope = oldExtraScope;
    _allowAnyProperty = oldAllowAnyProperty;
}

bool Check::visit(UiScriptBinding *ast)
{
    checkScopeObjectMember(ast->qualifiedId);

    return true;
}

bool Check::visit(UiArrayBinding *ast)
{
    checkScopeObjectMember(ast->qualifiedId);

    return true;
}

void Check::checkScopeObjectMember(const AST::UiQualifiedId *id)
{
    if (_allowAnyProperty)
        return;

    const ObjectValue *scopeObject = _context.qmlScopeObject();

    if (! id)
        return; // ### error?

    const QString propertyName = id->name->asString();

    if (propertyName == QLatin1String("id") && ! id->next)
        return;

    // attached properties
    if (! propertyName.isEmpty() && propertyName[0].isUpper())
        scopeObject = _context.typeEnvironment(_doc.data());

    const Value *value = scopeObject->lookupMember(propertyName, &_context);
    if (_extraScope && !value)
        value = _extraScope->lookupMember(propertyName, &_context);
    if (!value) {
        error(id->identifierToken,
              QString("'%1' is not a valid property name").arg(propertyName));
    }

    // ### check for rest of qualifiedId
}

void Check::error(const AST::SourceLocation &loc, const QString &message)
{
    _messages.append(DiagnosticMessage(DiagnosticMessage::Error, loc, message));
}

void Check::warning(const AST::SourceLocation &loc, const QString &message)
{
    _messages.append(DiagnosticMessage(DiagnosticMessage::Warning, loc, message));
}
