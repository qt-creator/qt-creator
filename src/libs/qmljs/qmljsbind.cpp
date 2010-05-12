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

#include "parser/qmljsast_p.h"
#include "qmljsbind.h"
#include "qmljsdocument.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJS::Interpreter;

Bind::Bind(Document *doc)
    : _doc(doc),
      _currentObjectValue(0),
      _idEnvironment(0),
      _rootObjectValue(0)
{
    if (_doc)
        accept(_doc->ast());
}

Bind::~Bind()
{
}

QStringList Bind::fileImports() const
{
    return _fileImports;
}

QStringList Bind::directoryImports() const
{
    return _directoryImports;
}

QStringList Bind::libraryImports() const
{
    return _libraryImports;
}

Interpreter::ObjectValue *Bind::currentObjectValue() const
{
    return _currentObjectValue;
}

Interpreter::ObjectValue *Bind::idEnvironment() const
{
    return _idEnvironment;
}

Interpreter::ObjectValue *Bind::rootObjectValue() const
{
    return _rootObjectValue;
}

Interpreter::ObjectValue *Bind::findQmlObject(AST::Node *node) const
{
    return _qmlObjects.value(node);
}

bool Bind::usesQmlPrototype(ObjectValue *prototype,
                            Context *context) const
{
    foreach (ObjectValue *object, _qmlObjects.values()) {
        const ObjectValue *resolvedPrototype = object->prototype(context);

        if (resolvedPrototype == prototype)
            return true;
    }

    return false;
}

Interpreter::ObjectValue *Bind::findFunctionScope(AST::FunctionDeclaration *node) const
{
    return _functionScopes.value(node);
}

bool Bind::isGroupedPropertyBinding(AST::Node *node) const
{
    return _groupedPropertyBindings.contains(node);
}

ObjectValue *Bind::switchObjectValue(ObjectValue *newObjectValue)
{
    ObjectValue *oldObjectValue = _currentObjectValue;
    _currentObjectValue = newObjectValue;
    return oldObjectValue;
}

QString Bind::toString(UiQualifiedId *qualifiedId, QChar delimiter)
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

ExpressionNode *Bind::expression(UiScriptBinding *ast) const
{
    if (ExpressionStatement *statement = cast<ExpressionStatement *>(ast->statement))
        return statement->expression;

    return 0;
}

ObjectValue *Bind::bindObject(UiQualifiedId *qualifiedTypeNameId, UiObjectInitializer *initializer)
{
    ObjectValue *parentObjectValue = 0;

    // normal component instance
    ASTObjectValue *objectValue = new ASTObjectValue(qualifiedTypeNameId, initializer, _doc, &_engine);
    QmlPrototypeReference *prototypeReference =
            new QmlPrototypeReference(qualifiedTypeNameId, _doc, &_engine);
    objectValue->setPrototype(prototypeReference);

    parentObjectValue = switchObjectValue(objectValue);

    if (parentObjectValue)
        objectValue->setProperty(QLatin1String("parent"), parentObjectValue);
    else {
        _rootObjectValue = objectValue;
        _rootObjectValue->setClassName(_doc->componentName());
    }

    accept(initializer);

    return switchObjectValue(parentObjectValue);
}

void Bind::accept(Node *node)
{
    Node::accept(node, this);
}

bool Bind::visit(AST::UiProgram *)
{
    _idEnvironment = _engine.newObject(/*prototype =*/ 0);
    return true;
}

bool Bind::visit(AST::Program *)
{
    _currentObjectValue = _engine.newObject(/*prototype =*/ 0);
    _rootObjectValue = _currentObjectValue;
    return true;
}

bool Bind::visit(UiImport *ast)
{
    if (ast->importUri) {
        _libraryImports += toString(ast->importUri, QLatin1Char('/'));
    } else if (ast->fileName) {
        const QFileInfo importFileInfo(_doc->path() + QLatin1Char('/') + ast->fileName->asString());
        if (importFileInfo.isFile())
            _fileImports += importFileInfo.absoluteFilePath();
        else if (importFileInfo.isDir())
            _directoryImports += importFileInfo.absoluteFilePath();
        //else
        //    error: file or directory does not exist
    }

    return false;
}

bool Bind::visit(UiPublicMember *)
{
    // nothing to do.
    return false;
}

bool Bind::visit(UiObjectDefinition *ast)
{
    // an UiObjectDefinition may be used to group property bindings
    // think anchors { ... }
    bool isGroupedBinding = false;
    for (UiQualifiedId *it = ast->qualifiedTypeNameId; it; it = it->next) {
        if (!it->next)
            isGroupedBinding = it->name->asString().at(0).isLower();
    }

    if (!isGroupedBinding) {
        ObjectValue *value = bindObject(ast->qualifiedTypeNameId, ast->initializer);
        _qmlObjects.insert(ast, value);
    } else {
        _groupedPropertyBindings.insert(ast);
    }

    return false;
}

bool Bind::visit(UiObjectBinding *ast)
{
//    const QString name = serialize(ast->qualifiedId);
    ObjectValue *value = bindObject(ast->qualifiedTypeNameId, ast->initializer);
    _qmlObjects.insert(ast, value);
    // ### FIXME: we don't handle dot-properties correctly (i.e. font.size)
//    _currentObjectValue->setProperty(name, value);

    return false;
}

bool Bind::visit(UiScriptBinding *ast)
{
    if (toString(ast->qualifiedId) == QLatin1String("id")) {
        if (ExpressionStatement *e = cast<ExpressionStatement*>(ast->statement))
            if (IdentifierExpression *i = cast<IdentifierExpression*>(e->expression))
                if (i->name)
                    _idEnvironment->setProperty(i->name->asString(), _currentObjectValue);
    }

    return false;
}

bool Bind::visit(UiArrayBinding *)
{
    // ### FIXME: do we need to store the members into the property? Or, maybe the property type is an JS Array?

    return true;
}

bool Bind::visit(VariableDeclaration *ast)
{
    if (! ast->name)
        return false;

    ASTVariableReference *ref = new ASTVariableReference(ast, &_engine);
    _currentObjectValue->setProperty(ast->name->asString(), ref);
    return false;
}

bool Bind::visit(FunctionDeclaration *ast)
{
    if (!ast->name)
        return false;
    // ### FIXME: the first declaration counts
    //if (_currentObjectValue->property(ast->name->asString(), 0))
    //    return false;

    ASTFunctionValue *function = new ASTFunctionValue(ast, &_engine);
    _currentObjectValue->setProperty(ast->name->asString(), function);

    // build function scope
    ObjectValue *functionScope = _engine.newObject(/*prototype=*/0);
    _functionScopes.insert(ast, functionScope);
    ObjectValue *parent = switchObjectValue(functionScope);

    // The order of the following is important. Example: A function with name "arguments"
    // overrides the arguments object, a variable doesn't.

    // 1. Function formal arguments
    for (FormalParameterList *it = ast->formals; it; it = it->next) {
        if (it->name)
            functionScope->setProperty(it->name->asString(), _engine.undefinedValue());
    }

    // 2. Functions defined inside the function body
    // ### TODO, currently covered by the accept(body)

    // 3. Arguments object
    ObjectValue *arguments = _engine.newObject(/*prototype=*/0);
    arguments->setProperty(QLatin1String("callee"), function);
    arguments->setProperty(QLatin1String("length"), _engine.numberValue());
    functionScope->setProperty(QLatin1String("arguments"), arguments);

    // 4. Variables defined inside the function body
    // ### TODO, currently covered by the accept(body)

    // visit body
    accept(ast->body);
    switchObjectValue(parent);

    return false;
}
