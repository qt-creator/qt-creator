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

#include "parser/qmljsast_p.h"
#include "qmljsbind.h"
#include "qmljsmetatypesystem.h"

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJS::Interpreter;

Bind::Bind()
{
}

Bind::~Bind()
{
}

Interpreter::ObjectValue *Bind::operator()(Document::Ptr doc, Snapshot &snapshot, UiObjectMember *member, Interpreter::Engine &interp)
{
    UiProgram *program = doc->qmlProgram();
    if (!program)
        return 0;

    _doc = doc;
    _snapshot = &snapshot;
    _interestingMember = member;
    _interp = &interp;

    _currentObjectValue = 0;
    _typeEnvironment = _interp->newObject(0);
    _idEnvironment = _interp->newObject(0);
    _interestingObjectValue = 0;
    _rootObjectValue = 0;

    accept(program);

    if (_interestingObjectValue) {
        _idEnvironment->setScope(_interestingObjectValue);

        if (_interestingObjectValue != _rootObjectValue)
            _interestingObjectValue->setScope(_rootObjectValue);
    } else {
        _idEnvironment->setScope(_rootObjectValue);
    }
    _typeEnvironment->setScope(_idEnvironment);

    return _typeEnvironment;
}

void Bind::accept(Node *node)
{
    Node::accept(node, this);
}

bool Bind::visit(UiProgram *)
{
    return true;
}

bool Bind::visit(UiImportList *)
{
    return true;
}

static QString serialize(UiQualifiedId *qualifiedId, QChar delimiter)
{
    QString result;

    for (UiQualifiedId *iter = qualifiedId; iter; iter = iter->next) {
        if (iter != qualifiedId)
            result += delimiter;

        if (iter->name)
            result += iter->name->asString();
    }

    return result;
}

/*
  import Qt 4.6
  import Qt 4.6 as Xxx
  (import com.nokia.qt is the same as the ones above)

  import "content"
  import "content" as Xxx
  import "content" 4.6
  import "content" 4.6 as Xxx

  import "http://www.ovi.com/" as Ovi
 */
bool Bind::visit(UiImport *ast)
{
    ObjectValue *namespaceObject;

    if (ast->asToken.isValid()) { // with namespace we insert an object in the type env. to hold the imported types
        namespaceObject = _interp->newObject(0);
        if (!ast->importId)
            return false; // this should never happen, but better be safe than sorry
        _typeEnvironment->setProperty(ast->importId->asString(), namespaceObject);
    } else { // without namespace we insert all types directly into the type env.
        namespaceObject = _typeEnvironment;
    }

    // look at files first

    // else try the metaobject system
    if (!ast->importUri)
        return false;

    const QString package = serialize(ast->importUri, '/');
    int majorVersion = -1; // ### TODO: Check these magic version numbers
    int minorVersion = -1; // ### TODO: Check these magic version numbers

    if (ast->versionToken.isValid()) {
        const QString versionString = _doc->source().mid(ast->versionToken.offset, ast->versionToken.length);
        int dotIdx = versionString.indexOf('.');
        if (dotIdx == -1) {
            // only major (which is probably invalid, but let's handle it anyway)
            majorVersion = versionString.toInt();
            minorVersion = 0; // ### TODO: Check with magic version numbers above
        } else {
            majorVersion = versionString.left(dotIdx).toInt();
            minorVersion = versionString.mid(dotIdx + 1).toInt();
        }
    }

#ifndef NO_DECLARATIVE_BACKEND
    foreach (QmlObjectValue *object, _interp->metaTypeSystem().staticTypesForImport(package, majorVersion, minorVersion)) {
        namespaceObject->setProperty(object->qmlTypeName(), object);
    }
#endif // NO_DECLARATIVE_BACKEND

    return false;
}

bool Bind::visit(UiPublicMember *ast)
{
    if (! (ast->name && ast->memberType))
        return false;

    const QString propName = ast->name->asString();
    const QString propType = ast->memberType->asString();

    // ### TODO: generalize
    if (propType == QLatin1String("string"))
        _currentObjectValue->setProperty(propName, _interp->stringValue());
    else if (propType == QLatin1String("bool"))
        _currentObjectValue->setProperty(propName, _interp->booleanValue());
    else if (propType == QLatin1String("int") || propType == QLatin1String("real"))
        _currentObjectValue->setProperty(propName, _interp->numberValue());

    return false;
}

bool Bind::visit(UiSourceElement *)
{
    return true;
}

const ObjectValue *Bind::lookupType(UiQualifiedId *qualifiedTypeNameId)
{
    const ObjectValue *objectValue = _typeEnvironment;

    for (UiQualifiedId *iter = qualifiedTypeNameId; iter; iter = iter->next) {
        if (! (iter->name))
            return 0;
        const Value *value = objectValue->property(iter->name->asString());
        if (!value)
            return 0;
        objectValue = value->asObjectValue();
        if (!objectValue)
            return 0;
    }

    return objectValue;
}

ObjectValue *Bind::bindObject(UiQualifiedId *qualifiedTypeNameId, UiObjectInitializer *initializer)
{
    const ObjectValue *prototype = lookupType(qualifiedTypeNameId);
    ObjectValue *objectValue = _interp->newObject(prototype);
    ObjectValue *oldObjectValue = switchObjectValue(objectValue);
    if (oldObjectValue)
        objectValue->setProperty("parent", oldObjectValue);
    else
        _rootObjectValue = objectValue;

    accept(initializer);

    return switchObjectValue(oldObjectValue);
}

bool Bind::visit(UiObjectDefinition *ast)
{
    ObjectValue *value = bindObject(ast->qualifiedTypeNameId, ast->initializer);

    if (_interestingMember == ast)
        _interestingObjectValue = value;
    return false;
}

bool Bind::visit(UiObjectBinding *ast)
{
//    const QString name = serialize(ast->qualifiedId);
    ObjectValue *value = bindObject(ast->qualifiedTypeNameId, ast->initializer);
    // ### FIXME: we don't handle dot-properties correctly (i.e. font.size)
//    _currentObjectValue->setProperty(name, value);

    if (_interestingMember == ast)
        _interestingObjectValue = value;
    return false;
}

bool Bind::visit(UiObjectInitializer *)
{
    return true;
}

bool Bind::visit(UiScriptBinding *ast)
{
    if (!(ast->qualifiedId->next) && ast->qualifiedId->name->asString() == "id")
        if (ExpressionStatement *e = cast<ExpressionStatement*>(ast->statement))
            if (IdentifierExpression *i = cast<IdentifierExpression*>(e->expression))
                if (i->name)
                    _idEnvironment->setProperty(i->name->asString(), _currentObjectValue);

    return false;
}

bool Bind::visit(UiArrayBinding *)
{
    // ### FIXME: do we need to store the members into the property? Or, maybe the property type is an JS Array?

    return true;
}

bool Bind::visit(UiObjectMemberList *)
{
    return true;
}

bool Bind::visit(UiArrayMemberList *)
{
    return true;
}

bool Bind::visit(UiQualifiedId *)
{
    return true;
}

bool Bind::visit(UiSignature *)
{
    return true;
}

bool Bind::visit(UiFormalList *)
{
    return true;
}

bool Bind::visit(UiFormal *)
{
    return true;
}

bool Bind::visit(ThisExpression *)
{
    return true;
}

bool Bind::visit(IdentifierExpression *)
{
    return true;
}

bool Bind::visit(NullExpression *)
{
    return true;
}

bool Bind::visit(TrueLiteral *)
{
    return true;
}

bool Bind::visit(FalseLiteral *)
{
    return true;
}

bool Bind::visit(StringLiteral *)
{
    return true;
}

bool Bind::visit(NumericLiteral *)
{
    return true;
}

bool Bind::visit(RegExpLiteral *)
{
    return true;
}

bool Bind::visit(ArrayLiteral *)
{
    return true;
}

bool Bind::visit(ObjectLiteral *)
{
    return true;
}

bool Bind::visit(ElementList *)
{
    return true;
}

bool Bind::visit(Elision *)
{
    return true;
}

bool Bind::visit(PropertyNameAndValueList *)
{
    return true;
}

bool Bind::visit(NestedExpression *)
{
    return true;
}

bool Bind::visit(IdentifierPropertyName *)
{
    return true;
}

bool Bind::visit(StringLiteralPropertyName *)
{
    return true;
}

bool Bind::visit(NumericLiteralPropertyName *)
{
    return true;
}

bool Bind::visit(ArrayMemberExpression *)
{
    return true;
}

bool Bind::visit(FieldMemberExpression *)
{
    return true;
}

bool Bind::visit(NewMemberExpression *)
{
    return true;
}

bool Bind::visit(NewExpression *)
{
    return true;
}

bool Bind::visit(CallExpression *)
{
    return true;
}

bool Bind::visit(ArgumentList *)
{
    return true;
}

bool Bind::visit(PostIncrementExpression *)
{
    return true;
}

bool Bind::visit(PostDecrementExpression *)
{
    return true;
}

bool Bind::visit(DeleteExpression *)
{
    return true;
}

bool Bind::visit(VoidExpression *)
{
    return true;
}

bool Bind::visit(TypeOfExpression *)
{
    return true;
}

bool Bind::visit(PreIncrementExpression *)
{
    return true;
}

bool Bind::visit(PreDecrementExpression *)
{
    return true;
}

bool Bind::visit(UnaryPlusExpression *)
{
    return true;
}

bool Bind::visit(UnaryMinusExpression *)
{
    return true;
}

bool Bind::visit(TildeExpression *)
{
    return true;
}

bool Bind::visit(NotExpression *)
{
    return true;
}

bool Bind::visit(BinaryExpression *)
{
    return true;
}

bool Bind::visit(ConditionalExpression *)
{
    return true;
}

bool Bind::visit(Expression *)
{
    return true;
}

bool Bind::visit(Block *)
{
    return true;
}

bool Bind::visit(StatementList *)
{
    return true;
}

bool Bind::visit(VariableStatement *)
{
    return true;
}

bool Bind::visit(VariableDeclarationList *)
{
    return true;
}

bool Bind::visit(VariableDeclaration *)
{
    return true;
}

bool Bind::visit(EmptyStatement *)
{
    return true;
}

bool Bind::visit(ExpressionStatement *)
{
    return true;
}

bool Bind::visit(IfStatement *)
{
    return true;
}

bool Bind::visit(DoWhileStatement *)
{
    return true;
}

bool Bind::visit(WhileStatement *)
{
    return true;
}

bool Bind::visit(ForStatement *)
{
    return true;
}

bool Bind::visit(LocalForStatement *)
{
    return true;
}

bool Bind::visit(ForEachStatement *)
{
    return true;
}

bool Bind::visit(LocalForEachStatement *)
{
    return true;
}

bool Bind::visit(ContinueStatement *)
{
    return true;
}

bool Bind::visit(BreakStatement *)
{
    return true;
}

bool Bind::visit(ReturnStatement *)
{
    return true;
}

bool Bind::visit(WithStatement *)
{
    return true;
}

bool Bind::visit(SwitchStatement *)
{
    return true;
}

bool Bind::visit(CaseBlock *)
{
    return true;
}

bool Bind::visit(CaseClauses *)
{
    return true;
}

bool Bind::visit(CaseClause *)
{
    return true;
}

bool Bind::visit(DefaultClause *)
{
    return true;
}

bool Bind::visit(LabelledStatement *)
{
    return true;
}

bool Bind::visit(ThrowStatement *)
{
    return true;
}

bool Bind::visit(TryStatement *)
{
    return true;
}

bool Bind::visit(Catch *)
{
    return true;
}

bool Bind::visit(Finally *)
{
    return true;
}

bool Bind::visit(FunctionDeclaration *)
{
    return true;
}

bool Bind::visit(FunctionExpression *)
{
    return true;
}

bool Bind::visit(FormalParameterList *)
{
    return true;
}

bool Bind::visit(FunctionBody *)
{
    return true;
}

bool Bind::visit(Program *)
{
    return true;
}

bool Bind::visit(SourceElements *)
{
    return true;
}

bool Bind::visit(FunctionSourceElement *)
{
    return true;
}

bool Bind::visit(StatementSourceElement *)
{
    return true;
}

bool Bind::visit(DebuggerStatement *)
{
    return true;
}

ObjectValue *Bind::switchObjectValue(ObjectValue *newObjectValue)
{
    ObjectValue *oldObjectValue = _currentObjectValue;
    _currentObjectValue = newObjectValue;
    return oldObjectValue;
}
