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
#include "qmljslink.h"
#include "qmljscheck.h"
#include "qmljsmetatypesystem.h"

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
      _functionEnvironment(0),
      _rootObjectValue(0)
{
    if (_doc)
        accept(_doc->ast());
}

Bind::~Bind()
{
}

QStringList Bind::includedScripts() const
{
    return _includedScripts;
}

QStringList Bind::localImports() const
{
    return _localImports;
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

void Bind::processScript(AST::UiQualifiedId *qualifiedId, AST::UiObjectInitializer *initializer)
{
    Q_UNUSED(qualifiedId);

    if (! initializer)
        return;

    for (UiObjectMemberList *it = initializer->members; it; it = it->next) {
        if (UiScriptBinding *binding = cast<UiScriptBinding *>(it->member)) {
            const QString bindingName = toString(binding->qualifiedId);
            if (bindingName == QLatin1String("source")) {
                if (StringLiteral *literal = cast<StringLiteral *>(expression(binding))) {
                    QFileInfo fileInfo(QDir(_doc->path()), literal->value->asString());
                    const QString scriptPath = fileInfo.absoluteFilePath();
                    _includedScripts.append(scriptPath);
                }
            }
        } else if (UiSourceElement *binding = cast<UiSourceElement *>(it->member)) {
            if (FunctionDeclaration *decl = cast<FunctionDeclaration *>(binding->sourceElement)) {
                accept(decl); // process the function declaration
            } else {
                // ### unexpected source element
            }
        } else {
            // ### unexpected binding.
        }
    }
}

ObjectValue *Bind::bindObject(UiQualifiedId *qualifiedTypeNameId, UiObjectInitializer *initializer)
{
    ObjectValue *parentObjectValue = 0;
    const QString typeName = toString(qualifiedTypeNameId);

    if (typeName == QLatin1String("Script")) {
        // Script blocks all contribute to the same scope
        parentObjectValue = switchObjectValue(_functionEnvironment);
        processScript(qualifiedTypeNameId, initializer);
        return switchObjectValue(parentObjectValue);
    }

    // normal component instance
    ASTObjectValue *objectValue = new ASTObjectValue(qualifiedTypeNameId, initializer, &_interp);
    parentObjectValue = switchObjectValue(objectValue);

    if (parentObjectValue)
        objectValue->setProperty(QLatin1String("parent"), parentObjectValue);
    else
        _rootObjectValue = objectValue;

    accept(initializer);

    return switchObjectValue(parentObjectValue);
}

void Bind::accept(Node *node)
{
    Node::accept(node, this);
}

bool Bind::visit(AST::UiProgram *)
{
    _idEnvironment = _interp.newObject(/*prototype =*/ 0);
    _functionEnvironment = _interp.newObject(/*prototype =*/ 0);
    return true;
}

bool Bind::visit(AST::Program *)
{
    _currentObjectValue = _interp.globalObject();
    _rootObjectValue = _interp.globalObject();
    return true;
}

bool Bind::visit(UiImport *ast)
{
    if (ast->fileName) {
        QString path = _doc->path();
        path += QLatin1Char('/');
        path += ast->fileName->asString();
        path = QDir::cleanPath(path);
        _localImports.append(path);
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
    ObjectValue *value = bindObject(ast->qualifiedTypeNameId, ast->initializer);
    _qmlObjects.insert(ast, value);

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

    ASTVariableReference *ref = new ASTVariableReference(ast, &_interp);
    _currentObjectValue->setProperty(ast->name->asString(), ref);
    return false;
}

bool Bind::visit(FunctionDeclaration *ast)
{
    if (!ast->name)
        return false;
    // the first declaration counts
    if (_currentObjectValue->property(ast->name->asString()))
        return false;

    ASTFunctionValue *function = new ASTFunctionValue(ast, &_interp);
    // ### set the function's scope.

    _currentObjectValue->setProperty(ast->name->asString(), function);

    return false; // ### eventually want to visit function bodies
}
