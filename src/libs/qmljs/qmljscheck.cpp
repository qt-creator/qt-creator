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

#include "qmljscheck.h"
#include "qmljsbind.h"
#include "qmljsinterpreter.h"
#include "qmljsevaluate.h"
#include "parser/qmljsast_p.h"

#include <QtCore/QDebug>
#include <QtGui/QColor>
#include <QtGui/QApplication>

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJS::Interpreter;


namespace {

class AssignmentCheck : public ValueVisitor
{
public:
    DiagnosticMessage operator()(
            const SourceLocation &location,
            const Interpreter::Value *lhsValue,
            const Interpreter::Value *rhsValue,
            ExpressionNode *ast)
    {
        _message = DiagnosticMessage(DiagnosticMessage::Error, location, QString());
        _rhsValue = rhsValue;
        _ast = ast;

        if (lhsValue)
            lhsValue->accept(this);

        return _message;
    }

    virtual void visit(const NumberValue *value)
    {
        if (const QmlEnumValue *enumValue = dynamic_cast<const QmlEnumValue *>(value)) {
            if (StringLiteral *stringLiteral = cast<StringLiteral *>(_ast)) {
                const QString valueName = stringLiteral->value->asString();

                if (!enumValue->keys().contains(valueName)) {
                    _message.message = Check::tr("unknown value for enum");
                }
            } else if (_rhsValue->asUndefinedValue()) {
                _message.kind = DiagnosticMessage::Warning;
                _message.message = Check::tr("value might be 'undefined'");
            } else if (! _rhsValue->asStringValue() && ! _rhsValue->asNumberValue()) {
                _message.message = Check::tr("enum value is not a string or number");
            }
        } else {
            if (/*cast<StringLiteral *>(_ast)
                ||*/ _ast->kind == Node::Kind_TrueLiteral
                     || _ast->kind == Node::Kind_FalseLiteral) {
                _message.message = Check::tr("numerical value expected");
            }
        }
    }

    virtual void visit(const BooleanValue *)
    {
        UnaryMinusExpression *unaryMinus = cast<UnaryMinusExpression *>(_ast);

        if (cast<StringLiteral *>(_ast)
                || cast<NumericLiteral *>(_ast)
                || (unaryMinus && cast<NumericLiteral *>(unaryMinus->expression))) {
            _message.message = Check::tr("boolean value expected");
        }
    }

    virtual void visit(const StringValue *)
    {
        UnaryMinusExpression *unaryMinus = cast<UnaryMinusExpression *>(_ast);

        if (cast<NumericLiteral *>(_ast)
                || (unaryMinus && cast<NumericLiteral *>(unaryMinus->expression))
                || _ast->kind == Node::Kind_TrueLiteral
                || _ast->kind == Node::Kind_FalseLiteral) {
            _message.message = Check::tr("string value expected");
        }
    }

    virtual void visit(const ColorValue *)
    {
        if (StringLiteral *stringLiteral = cast<StringLiteral *>(_ast)) {
            const QString colorString = stringLiteral->value->asString();

            bool ok = true;
            if (colorString.size() == 9 && colorString.at(0) == QLatin1Char('#')) {
                // #rgba
                for (int i = 1; i < 9; ++i) {
                    const QChar c = colorString.at(i);
                    if ((c >= QLatin1Char('0') && c <= QLatin1Char('9'))
                        || (c >= QLatin1Char('a') && c <= QLatin1Char('f'))
                        || (c >= QLatin1Char('A') && c <= QLatin1Char('F')))
                        continue;
                    ok = false;
                    break;
                }
            } else {
                ok = QColor::isValidColor(colorString);
            }
            if (!ok)
                _message.message = Check::tr("not a valid color");
        } else {
            visit((StringValue *)0);
        }
    }

    virtual void visit(const AnchorLineValue *)
    {
        if (! (_rhsValue->asAnchorLineValue() || _rhsValue->asUndefinedValue()))
            _message.message = Check::tr("expected anchor line");
    }

    DiagnosticMessage _message;
    const Value *_rhsValue;
    ExpressionNode *_ast;
};

} // end of anonymous namespace


Check::Check(Document::Ptr doc, const Snapshot &snapshot, const QStringList &importPaths)
    : _doc(doc)
    , _snapshot(snapshot)
    , _context(&_engine)
    , _link(&_context, doc, snapshot, importPaths)
    , _scopeBuilder(doc, &_context)
    , _ignoreTypeErrors(_context.documentImportsPlugins(_doc.data()))
{
}

Check::~Check()
{
}

QList<DiagnosticMessage> Check::operator()()
{
    _messages.clear();
    Node::accept(_doc->ast(), this);
    _messages.append(_link.diagnosticMessages());
    return _messages;
}

bool Check::visit(UiProgram *)
{
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
        if (! _ignoreTypeErrors)
            error(typeId->identifierToken,
                  Check::tr("unknown type"));
        // suppress subsequent errors about scope object lookup by clearing
        // the scope object list
        // ### todo: better way?
        _context.scopeChain().qmlScopeObjects.clear();
        _context.scopeChain().update();
    }

    Node::accept(initializer, this);

    _scopeBuilder.pop();
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
            error(loc, Check::tr("expected id"));
            return false;
        }

        QString id;
        if (IdentifierExpression *idExp = cast<IdentifierExpression *>(expStmt->expression)) {
            id = idExp->name->asString();
        } else if (StringLiteral *strExp = cast<StringLiteral *>(expStmt->expression)) {
            id = strExp->value->asString();
            warning(loc, Check::tr("using string literals for ids is discouraged"));
        } else {
            error(loc, Check::tr("expected id"));
            return false;
        }

        if (id.isEmpty() || ! id[0].isLower()) {
            error(loc, Check::tr("ids must be lower case"));
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
            AssignmentCheck assignmentCheck;
            DiagnosticMessage message = assignmentCheck(loc, lhsValue, rhsValue, expr);
            if (! message.message.isEmpty())
                _messages += message;
        }

    }

    return true;
}

bool Check::visit(UiArrayBinding *ast)
{
    checkScopeObjectMember(ast->qualifiedId);

    return true;
}

/// When something is changed here, also change ReadingContext::lookupProperty in
/// texttomodelmerger.cpp
/// ### Maybe put this into the context as a helper method.
const Value *Check::checkScopeObjectMember(const UiQualifiedId *id)
{
    QList<const ObjectValue *> scopeObjects = _context.scopeChain().qmlScopeObjects;
    if (scopeObjects.isEmpty())
        return 0;

    if (! id)
        return 0; // ### error?

    if (! id->name) // possible after error recovery
        return 0;

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
              Check::tr("'%1' is not a valid property name").arg(propertyName));
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
                  Check::tr("'%1' does not have members").arg(propertyName));
            return 0;
        }

        if (! idPart->next->name) {
            // somebody typed "id." and error recovery still gave us a valid tree,
            // so just bail out here.
            return 0;
        }

        idPart = idPart->next;
        propertyName = idPart->name->asString();

        value = objectValue->lookupMember(propertyName, &_context);
        if (! value) {
            error(idPart->identifierToken,
                  Check::tr("'%1' is not a member of '%2'").arg(
                          propertyName, objectValue->className()));
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
