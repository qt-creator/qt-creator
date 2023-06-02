// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljsbind.h"

#include "parser/qmljsast_p.h"

#include "qmljsdocument.h"
#include "qmljsmodelmanagerinterface.h"
#include "qmljstr.h"
#include "qmljsutils.h"

#include <QtCore/QVersionNumber>
#include <QtCore/QLibraryInfo>

#include <utils/algorithm.h>

using namespace LanguageUtils;
using namespace QmlJS;
using namespace QmlJS::AST;

/*!
    \class QmlJS::Bind
    \brief The Bind class collects information about a single Document.
    \sa Document Context

    Each Document owns an instance of Bind. It provides access to data
    that can be derived by looking at the document in isolation. If you need
    information that goes beyond that, you need to use a Context.

    The document's imports are classified and available through imports().

    This class makes the structural information found in the AST available
    for analysis through Value instances. See findQmlObject(),
    idEnvironment(), rootObjectValue() and findAttachedJSScope().
*/

Bind::Bind(Document *doc, QList<DiagnosticMessage> *messages, bool isJsLibrary, const QList<ImportInfo> &jsImports)
    : _doc(doc),
      _currentObjectValue(nullptr),
      _idEnvironment(nullptr),
      _rootObjectValue(nullptr),
      _isJsLibrary(isJsLibrary),
      _imports(jsImports),
      _diagnosticMessages(messages)
{
    if (_doc)
        accept(_doc->ast());
}

Bind::~Bind()
{
}

bool Bind::isJsLibrary() const
{
    return _isJsLibrary;
}

const QList<ImportInfo> Bind::imports() const
{
    return _imports;
}

ObjectValue *Bind::idEnvironment() const
{
    return _idEnvironment;
}

ObjectValue *Bind::rootObjectValue() const
{
    return _rootObjectValue;
}

ObjectValue *Bind::findQmlObject(AST::Node *node) const
{
    return _qmlObjects.value(node);
}

bool Bind::usesQmlPrototype(ObjectValue *prototype,
                            const ContextPtr &context) const
{
    if (!prototype)
        return false;

    const QString componentName = prototype->className();

    // all component objects have classname set
    if (componentName.isEmpty())
        return false;

    QList<const ObjectValue *> values = _qmlObjectsByPrototypeName.values(componentName);
    for (const ObjectValue *object : values) {
        // resolve and check the prototype
        const ObjectValue *resolvedPrototype = object->prototype(context);
        if (resolvedPrototype == prototype)
            return true;
    }

    return false;
}

ObjectValue *Bind::findAttachedJSScope(AST::Node *node) const
{
    return _attachedJSScopes.value(node);
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

ObjectValue *Bind::bindObject(UiQualifiedId *qualifiedTypeNameId, UiObjectInitializer *initializer)
{
    ObjectValue *parentObjectValue = nullptr;

    // normal component instance
    ASTObjectValue *objectValue = new ASTObjectValue(qualifiedTypeNameId, initializer, _doc, &_valueOwner);
    QmlPrototypeReference *prototypeReference =
            new QmlPrototypeReference(qualifiedTypeNameId, _doc, &_valueOwner);
    objectValue->setPrototype(prototypeReference);

    // add the prototype name to the prototypes hash
    for (UiQualifiedId *it = qualifiedTypeNameId; it; it = it->next) {
        if (!it->next && !it->name.isEmpty())
            _qmlObjectsByPrototypeName.insert(it->name.toString(), objectValue);
    }

    parentObjectValue = switchObjectValue(objectValue);
    ObjectValue *nextRoot = _rootObjectValue;
    QString parentComponentName = _currentComponentName;
    if (!_rootObjectValue) {
        _rootObjectValue = objectValue;
        _inlineComponents[_currentComponentName] = objectValue;
        if (!_currentComponentName.isEmpty()) {
            if (_currentComponentName.contains('.')) {
                parentComponentName = _currentComponentName.mid(0,_currentComponentName.lastIndexOf('.'));
                nextRoot = _inlineComponents.value(parentComponentName);
            } else {
                parentComponentName = "";
                nextRoot = _rootObjectValue;
            }
            // we add the inline component inside its parent
            nextRoot->setMember(_currentComponentName.mid(_currentComponentName.lastIndexOf('.') + 1), objectValue);
            _rootObjectValue->setClassName(_doc->componentName() + "." + _currentComponentName); // use :: instead of .?
        } else {
            nextRoot = _rootObjectValue;
            _rootObjectValue->setClassName(_doc->componentName());
        }
    } else if (parentObjectValue) {
        objectValue->setMember(QLatin1String("parent"), parentObjectValue);
    }

    accept(initializer);

    _rootObjectValue = nextRoot;
    _currentComponentName = parentComponentName;
    return switchObjectValue(parentObjectValue);
}

void Bind::throwRecursionDepthError()
{
    _diagnosticMessages->append(DiagnosticMessage(Severity::Error, SourceLocation(),
                                                  Tr::tr("Hit maximal recursion depth in AST visit.")));
}

void Bind::accept(Node *node)
{
    Node::accept(node, this);
}

bool Bind::visit(AST::UiProgram *)
{
    _idEnvironment = _valueOwner.newObject(/*prototype =*/ nullptr);
    return true;
}

bool Bind::visit(AST::Program *)
{
    _currentObjectValue = _valueOwner.newObject(/*prototype =*/ nullptr);
    _rootObjectValue = _currentObjectValue;
    return true;
}

void Bind::endVisit(UiProgram *)
{
    if (_doc->language() == Dialect::QmlQbs) {
        static const QString qbsBaseImport = QStringLiteral("qbs");
        static auto isQbsBaseImport = [] (const ImportInfo &ii) {
            return ii.name() == qbsBaseImport; };
        if (!Utils::anyOf(_imports, isQbsBaseImport))
            _imports += ImportInfo::moduleImport(qbsBaseImport, ComponentVersion(), QString());
    }
}

bool Bind::visit(UiImport *ast)
{
    ComponentVersion version;
    if (ast->version)
        version = ComponentVersion(ast->version->majorVersion, ast->version->minorVersion);

    if (auto importUri = ast->importUri) {
        QVersionNumber qtVersion;
        QString uri = toString(importUri);
        if (ModelManagerInterface *model = ModelManagerInterface::instance()) {
            ModelManagerInterface::ProjectInfo pInfo = model->projectInfoForPath(_doc->fileName());
            qtVersion = QVersionNumber::fromString(pInfo.qtVersionString);
            uri = pInfo.moduleMappings.value(uri, uri);
        }
        if (!version.isValid() && (!qtVersion.isNull() && qtVersion.majorVersion() < 6)) {
            _diagnosticMessages->append(
                        errorMessage(ast, Tr::tr("package import requires a version number")));
        }
        const QString importId = ast->importId.toString();

        ImportInfo import = ImportInfo::moduleImport(uri, version, importId, ast);
        if (_doc->language() == Dialect::Qml) {
            const QString importStr = import.name() + importId;
            if (ModelManagerInterface::instance()) {
                QmlLanguageBundles langBundles = ModelManagerInterface::instance()->extendedBundles();
                QmlBundle qq2 = langBundles.bundleForLanguage(Dialect::QmlQtQuick2);
                bool isQQ2 = qq2.supportedImports().contains(importStr);
                if (isQQ2)
                    _doc->setLanguage(Dialect::QmlQtQuick2);
            }
        }

        // Make sure QtQuick import is in the list before imports that might depend on it
        if (import.name() == QLatin1String("QtQuick"))
            _imports.prepend(import);
        else
            _imports += import;
    } else if (!ast->fileName.isEmpty()) {
        _imports += ImportInfo::pathImport(_doc->path(), ast->fileName.toString(),
                                           version, ast->importId.toString(), ast);
    } else {
        _imports += ImportInfo::invalidImport(ast);
    }
    return false;
}

bool Bind::visit(UiPublicMember *ast)
{
    const Block *block = AST::cast<const Block*>(ast->statement);
    if (block) {
        // build block scope
        ObjectValue *blockScope = _valueOwner.newObject(/*prototype=*/nullptr);
        _attachedJSScopes.insert(ast, blockScope); // associated with the UiPublicMember, not with the block
        ObjectValue *parent = switchObjectValue(blockScope);
        accept(ast->statement);
        switchObjectValue(parent);
        return false;
    }
    return true;
}

bool Bind::visit(UiObjectDefinition *ast)
{
    // an UiObjectDefinition may be used to group property bindings
    // think anchors { ... }
    bool isGroupedBinding = ast->qualifiedTypeNameId
            && !ast->qualifiedTypeNameId->name.isEmpty()
            && ast->qualifiedTypeNameId->name.at(0).isLower();

    if (!isGroupedBinding) {
        ObjectValue *value = bindObject(ast->qualifiedTypeNameId, ast->initializer);
        _qmlObjects.insert(ast, value);
    } else {
        _groupedPropertyBindings.insert(ast);
        ObjectValue *oldObjectValue = switchObjectValue(nullptr);
        accept(ast->initializer);
        switchObjectValue(oldObjectValue);
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
    if (_currentObjectValue && toString(ast->qualifiedId) == QLatin1String("id")) {
        if (ExpressionStatement *e = cast<ExpressionStatement*>(ast->statement))
            if (IdentifierExpression *i = cast<IdentifierExpression*>(e->expression))
                if (!i->name.isEmpty())
                    _idEnvironment->setMember(i->name.toString(), _currentObjectValue);
    }
    const Block *block = AST::cast<const Block*>(ast->statement);
    if (block) {
        // build block scope
        ObjectValue *blockScope = _valueOwner.newObject(/*prototype=*/nullptr);
        _attachedJSScopes.insert(ast, blockScope); // associated with the UiScriptBinding, not with the block
        ObjectValue *parent = switchObjectValue(blockScope);
        accept(ast->statement);
        switchObjectValue(parent);
        return false;
    }
    return true;
}

bool Bind::visit(UiArrayBinding *)
{
    // ### FIXME: do we need to store the members into the property? Or, maybe the property type is an JS Array?

    return true;
}

bool Bind::visit(UiInlineComponent *ast)
{
    if (!_currentComponentName.isEmpty()) {
        _currentComponentName += ".";
        _diagnosticMessages->append(
            errorMessage(ast, Tr::tr("Nested inline components are not supported.")));
    }
    _currentComponentName += ast->name.toString();
    _rootObjectValue = nullptr;
    return true;
}

bool Bind::visit(AST::TemplateLiteral *ast)
{
    Node::accept(ast->expression, this);
    return true;
}

bool Bind::visit(PatternElement *ast)
{
    if (ast->bindingIdentifier.isEmpty() || !ast->isVariableDeclaration())
        return false;

    ASTVariableReference *ref = new ASTVariableReference(ast, _doc, &_valueOwner);
    if (_currentObjectValue)
        _currentObjectValue->setMember(ast->bindingIdentifier, ref);
    return true;
}

bool Bind::visit(FunctionExpression *ast)
{
    // ### FIXME: the first declaration counts
    //if (_currentObjectValue->property(ast->name->asString(), 0))
    //    return false;

    ASTFunctionValue *function = new ASTFunctionValue(ast, _doc, &_valueOwner);
    if (_currentObjectValue && !ast->name.isEmpty() && cast<FunctionDeclaration *>(ast))
        _currentObjectValue->setMember(ast->name.toString(), function);

    // build function scope
    ObjectValue *functionScope = _valueOwner.newObject(/*prototype=*/nullptr);
    _attachedJSScopes.insert(ast, functionScope);
    ObjectValue *parent = switchObjectValue(functionScope);

    // The order of the following is important. Example: A function with name "arguments"
    // overrides the arguments object, a variable doesn't.

    // 1. Function formal arguments
    for (FormalParameterList *it = ast->formals; it; it = it->next) {
        if (!it->element->bindingIdentifier.isEmpty())
            functionScope->setMember(it->element->bindingIdentifier, _valueOwner.unknownValue());
    }

    // 2. Functions defined inside the function body
    // ### TODO, currently covered by the accept(body)

    // 3. Arguments object
    ObjectValue *arguments = _valueOwner.newObject(/*prototype=*/nullptr);
    arguments->setMember(QLatin1String("callee"), function);
    arguments->setMember(QLatin1String("length"), _valueOwner.numberValue());
    functionScope->setMember(QLatin1String("arguments"), arguments);

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
