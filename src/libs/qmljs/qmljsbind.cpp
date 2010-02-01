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
#include "qmljsmetatypesystem.h"
#include <QtCore/QDebug>

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJS::Interpreter;

Bind::Bind(Document::Ptr doc, Interpreter::Engine *interp)
    : _doc(doc),
      _interp(interp),
      _currentObjectValue(0),
      _typeEnvironment(0),
      _idEnvironment(0),
      _functionEnvironment(0),
      _rootObjectValue(0)
{
    if (_doc && _doc->qmlProgram() != 0) {
        UiProgram *program = _doc->qmlProgram();

        _currentObjectValue = 0;
        _typeEnvironment = _interp->newObject(/*prototype =*/ 0);
        _idEnvironment = _interp->newObject(/*prototype =*/ 0);
        _functionEnvironment = _interp->newObject(/*prototype =*/ 0);
        _rootObjectValue = 0;

        _qmlObjectDefinitions.clear();
        _qmlObjectBindings.clear();

        accept(program);
    }
}

Bind::~Bind()
{
}

ObjectValue *Bind::switchObjectValue(ObjectValue *newObjectValue)
{
    ObjectValue *oldObjectValue = _currentObjectValue;
    _currentObjectValue = newObjectValue;
    return oldObjectValue;
}

ObjectValue *Bind::scopeChainAt(Document::Ptr currentDocument, const Snapshot &snapshot,
                                Interpreter::Engine *interp, AST::UiObjectMember *currentObject)
{
    Bind *currentBind = 0;
    QList<Bind *> binds;

    Snapshot::const_iterator end = snapshot.end();
    for (Snapshot::const_iterator iter = snapshot.begin(); iter != end; ++iter) {
        Document::Ptr doc = *iter;
        Bind *newBind = new Bind(doc, interp);
        binds += newBind;
        if (doc == currentDocument)
            currentBind = newBind;
    }

    LinkImports linkImports;
    Link link;

    linkImports(binds);
    ObjectValue *scope = interp->globalObject();
    if (currentBind)
        scope = link(binds, currentBind, currentObject);
    qDeleteAll(binds);

    return scope;
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

ObjectValue *Bind::bindObject(UiQualifiedId *qualifiedTypeNameId, UiObjectInitializer *initializer)
{
    ObjectValue *parentObjectValue;

    if (toString(qualifiedTypeNameId) == QLatin1String("Script")) {
        // Script blocks all contribute to the same scope
        parentObjectValue = switchObjectValue(_functionEnvironment);
    } else { // normal component instance
        ObjectValue *objectValue = _interp->newObject(/*prototype =*/0);
        parentObjectValue = switchObjectValue(objectValue);
        if (parentObjectValue)
            objectValue->setProperty("parent", parentObjectValue);
        else
            _rootObjectValue = objectValue;
    }

    accept(initializer);

    return switchObjectValue(parentObjectValue);
}

void Bind::accept(Node *node)
{
    Node::accept(node, this);
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
// ### TODO: Move to LinkImports
bool Bind::visit(UiImport *ast)
{
    if (! (ast->importUri || ast->fileName))
        return false; // nothing to do.

    ObjectValue *namespaceObject = 0;

    if (ast->asToken.isValid()) { // with namespace we insert an object in the type env. to hold the imported types
        if (!ast->importId)
            return false; // this should never happen, but better be safe than sorry

        namespaceObject = _interp->newObject(/*prototype */ 0);

        _typeEnvironment->setProperty(ast->importId->asString(), namespaceObject);

    } else { // without namespace we insert all types directly into the type env.
        namespaceObject = _typeEnvironment;
    }

    // look at files first

    // else try the metaobject system
    if (ast->importUri) {
        const QString package = toString(ast->importUri, '/');
        int majorVersion = -1; // ### TODO: Check these magic version numbers
        int minorVersion = -1; // ### TODO: Check these magic version numbers

        if (ast->versionToken.isValid()) {
            const QString versionString = _doc->source().mid(ast->versionToken.offset, ast->versionToken.length);
            const int dotIdx = versionString.indexOf(QLatin1Char('.'));
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
    }

    return false;
}

bool Bind::visit(UiPublicMember *ast)
{
    if (_currentObjectValue && ast->name && ast->memberType) {
        const QString propName = ast->name->asString();
        const QString propType = ast->memberType->asString();

        _currentObjectValue->setProperty(propName, _interp->defaultValueForBuiltinType(propType));
    }

    return false;
}

bool Bind::visit(UiObjectDefinition *ast)
{
    ObjectValue *value = bindObject(ast->qualifiedTypeNameId, ast->initializer);
    _qmlObjectDefinitions.insert(ast, value);

    return false;
}

bool Bind::visit(UiObjectBinding *ast)
{
//    const QString name = serialize(ast->qualifiedId);
    ObjectValue *value = bindObject(ast->qualifiedTypeNameId, ast->initializer);
    _qmlObjectBindings.insert(ast, value);
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

bool Bind::visit(FunctionDeclaration *ast)
{
    if (!ast->name)
        return false;
    // the first declaration counts
    if (_currentObjectValue->property(ast->name->asString()))
        return false;

    Function *function = _interp->newFunction();
    for (FormalParameterList *iter = ast->formals; iter; iter = iter->next) {
        function->addArgument(_interp->undefinedValue()); // ### introduce unknownValue
        // ### store argument names
    }
    _currentObjectValue->setProperty(ast->name->asString(), function);
    return false; // ### eventually want to visit function bodies
}
