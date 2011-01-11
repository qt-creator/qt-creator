/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "parser/qmljsast_p.h"
#include "qmljsbind.h"
#include "qmljscheck.h"
#include "qmljsdocument.h"

#include <languageutils/componentversion.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>

using namespace LanguageUtils;
using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJS::Interpreter;

/*!
    \class QmlJS::Bind
    \brief Collected information about a single Document.
    \sa QmlJS::Document QmlJS::Link

    Each QmlJS::Document owns a instance of Bind. It provides access to data
    that can be derived by looking at the document in isolation. If you need
    information that goes beyond that, you need to create a
    \l{QmlJS::Interpreter::Context} using \l{QmlJS::Link}.

    The document's imports are classified and available through imports().

    It allows AST to code model lookup through findQmlObject() and findFunctionScope().
*/

Bind::Bind(Document *doc, QList<DiagnosticMessage> *messages)
    : _doc(doc),
      _currentObjectValue(0),
      _idEnvironment(0),
      _rootObjectValue(0),
      _diagnosticMessages(messages)
{
    if (_doc)
        accept(_doc->ast());
}

Bind::~Bind()
{
}

QList<ImportInfo> Bind::imports() const
{
    return _imports;
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
                            const Context *context) const
{
    if (!prototype)
        return false;

    const QString componentName = prototype->className();

    // all component objects have classname set
    if (componentName.isEmpty())
        return false;

    // get a list of all the names that may refer to this component
    // this can only happen for file imports with an 'as' clause
    // if there aren't any, possibleNames will be left empty
    QSet<QString> possibleNames;
    foreach (const ImportInfo &import, imports()) {
        if (import.type() == ImportInfo::FileImport
                && !import.id().isEmpty()
                && import.name().contains(componentName)) {
            possibleNames.insert(import.id());
        }
    }
    if (!possibleNames.isEmpty())
        possibleNames.insert(componentName);

    // if there are no renamed imports and the document does not use
    // the className string anywhere, it's out
    if (possibleNames.isEmpty()) {
        NameId nameId(componentName.data(), componentName.size());
        if (!_doc->engine()->literals().contains(nameId))
            return false;
    }

    QHashIterator<Node *, ObjectValue *> it(_qmlObjects);
    while (it.hasNext()) {
        it.next();

        // if the type id does not contain one of the possible names, skip
        Node *node = it.key();
        UiQualifiedId *id = 0;
        if (UiObjectDefinition *n = cast<UiObjectDefinition *>(node)) {
            id = n->qualifiedTypeNameId;
        } else if (UiObjectBinding *n = cast<UiObjectBinding *>(node)) {
            id = n->qualifiedTypeNameId;
        }
        if (!id)
            continue;

        bool skip = false;
        // optimize the common case of no renamed imports
        if (possibleNames.isEmpty()) {
            for (UiQualifiedId *idIt = id; idIt; idIt = idIt->next) {
                if (!idIt->next && idIt->name->asString() != componentName)
                    skip = true;
            }
        } else {
            for (UiQualifiedId *idIt = id; idIt; idIt = idIt->next) {
                if (!idIt->next && !possibleNames.contains(idIt->name->asString()))
                    skip = true;
            }
        }
        if (skip)
            continue;

        // resolve and check the prototype
        const ObjectValue *object = it.value();
        const ObjectValue *resolvedPrototype = object->prototype(context);
        if (resolvedPrototype == prototype)
            return true;
    }

    return false;
}

Interpreter::ObjectValue *Bind::findFunctionScope(AST::FunctionExpression *node) const
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
    ComponentVersion version;
    ImportInfo::Type type = ImportInfo::InvalidImport;
    QString name;

    if (ast->versionToken.isValid()) {
        const QString versionString = _doc->source().mid(ast->versionToken.offset, ast->versionToken.length);
        const int dotIdx = versionString.indexOf(QLatin1Char('.'));
        if (dotIdx != -1) {
            version = ComponentVersion(versionString.left(dotIdx).toInt(),
                                       versionString.mid(dotIdx + 1).toInt());
        } else {
            _diagnosticMessages->append(
                        errorMessage(ast->versionToken, tr("expected two numbers separated by a dot")));
        }
    }

    if (ast->importUri) {
        type = ImportInfo::LibraryImport;
        name = toString(ast->importUri, QDir::separator());

        if (!version.isValid()) {
            _diagnosticMessages->append(
                        errorMessage(ast, tr("package import requires a version number")));
        }
    } else if (ast->fileName) {
        const QFileInfo importFileInfo(_doc->path() + QDir::separator() + ast->fileName->asString());
        name = importFileInfo.absoluteFilePath();
        if (importFileInfo.isFile())
            type = ImportInfo::FileImport;
        else if (importFileInfo.isDir())
            type = ImportInfo::DirectoryImport;
        else {
            _diagnosticMessages->append(
                        errorMessage(ast, tr("file or directory not found")));
            type = ImportInfo::UnknownFileImport;
        }
    }
    _imports += ImportInfo(type, name, version, ast);

    return false;
}

bool Bind::visit(UiPublicMember *)
{
    // nothing to do.
    return true;
}

bool Bind::visit(UiObjectDefinition *ast)
{
    // an UiObjectDefinition may be used to group property bindings
    // think anchors { ... }
    bool isGroupedBinding = false;
    for (UiQualifiedId *it = ast->qualifiedTypeNameId; it; it = it->next) {
        if (!it->next && it->name)
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

    return true;
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
    return true;
}

bool Bind::visit(FunctionExpression *ast)
{
    // ### FIXME: the first declaration counts
    //if (_currentObjectValue->property(ast->name->asString(), 0))
    //    return false;

    ASTFunctionValue *function = new ASTFunctionValue(ast, _doc, &_engine);
    if (ast->name && cast<FunctionDeclaration *>(ast))
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

bool Bind::visit(FunctionDeclaration *ast)
{
    return visit(static_cast<FunctionExpression *>(ast));
}
