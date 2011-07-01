/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qmljsinterpreter.h"
#include "qmljsevaluate.h"
#include "qmljslink.h"
#include "qmljsbind.h"
#include "qmljsscopebuilder.h"
#include "qmljstypedescriptionreader.h"
#include "qmljsscopeastpath.h"
#include "qmljsvalueowner.h"
#include "parser/qmljsast_p.h"

#include <languageutils/fakemetaobject.h>

#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QMetaObject>
#include <QtCore/QMetaProperty>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QProcess>
#include <QtCore/QDebug>

using namespace LanguageUtils;
using namespace QmlJS::Interpreter;
using namespace QmlJS::AST;

namespace {

class LookupMember: public MemberProcessor
{
    QString _name;
    const Value *_value;

    bool process(const QString &name, const Value *value)
    {
        if (_value)
            return false;

        if (name == _name) {
            _value = value;
            return false;
        }

        return true;
    }

public:
    LookupMember(const QString &name)
        : _name(name), _value(0) {}

    const Value *value() const { return _value; }

    virtual bool processProperty(const QString &name, const Value *value)
    {
        return process(name, value);
    }

    virtual bool processEnumerator(const QString &name, const Value *value)
    {
        return process(name, value);
    }

    virtual bool processSignal(const QString &name, const Value *value)
    {
        return process(name, value);
    }

    virtual bool processSlot(const QString &name, const Value *value)
    {
        return process(name, value);
    }

    virtual bool processGeneratedSlot(const QString &name, const Value *value)
    {
        return process(name, value);
    }
};

class MetaFunction: public FunctionValue
{
    FakeMetaMethod _method;

public:
    MetaFunction(const FakeMetaMethod &method, ValueOwner *valueOwner)
        : FunctionValue(valueOwner), _method(method)
    {
    }

    virtual const Value *returnValue() const
    {
        return valueOwner()->undefinedValue();
    }

    virtual int argumentCount() const
    {
        return _method.parameterNames().size();
    }

    virtual const Value *argument(int) const
    {
        return valueOwner()->undefinedValue();
    }

    virtual QString argumentName(int index) const
    {
        if (index < _method.parameterNames().size())
            return _method.parameterNames().at(index);

        return FunctionValue::argumentName(index);
    }

    virtual bool isVariadic() const
    {
        return false;
    }

    virtual const Value *invoke(const Activation *) const
    {
        return valueOwner()->undefinedValue();
    }
};

} // end of anonymous namespace

QmlObjectValue::QmlObjectValue(FakeMetaObject::ConstPtr metaObject, const QString &className,
                               const QString &packageName, const ComponentVersion version, ValueOwner *valueOwner)
    : ObjectValue(valueOwner),
      _attachedType(0),
      _metaObject(metaObject),
      _packageName(packageName),
      _componentVersion(version)
{
    setClassName(className);
}

QmlObjectValue::~QmlObjectValue()
{}

const Value *QmlObjectValue::findOrCreateSignature(int index, const FakeMetaMethod &method, QString *methodName) const
{
    *methodName = method.methodName();
    const Value *value = _metaSignature.value(index);
    if (! value) {
        value = new MetaFunction(method, valueOwner());
        _metaSignature.insert(index, value);
    }
    return value;
}

void QmlObjectValue::processMembers(MemberProcessor *processor) const
{
    // process the meta enums
    for (int index = _metaObject->enumeratorOffset(); index < _metaObject->enumeratorCount(); ++index) {
        FakeMetaEnum e = _metaObject->enumerator(index);

        for (int i = 0; i < e.keyCount(); ++i) {
            processor->processEnumerator(e.key(i), valueOwner()->numberValue());
        }
    }

    // all explicitly defined signal names
    QSet<QString> explicitSignals;

    // process the meta methods
    for (int index = 0; index < _metaObject->methodCount(); ++index) {
        const FakeMetaMethod method = _metaObject->method(index);
        if (_componentVersion.isValid() && _componentVersion.minorVersion() < method.revision())
            continue;

        QString methodName;
        const Value *signature = findOrCreateSignature(index, method, &methodName);

        if (method.methodType() == FakeMetaMethod::Slot && method.access() == FakeMetaMethod::Public) {
            processor->processSlot(methodName, signature);

        } else if (method.methodType() == FakeMetaMethod::Signal && method.access() != FakeMetaMethod::Private) {
            // process the signal
            processor->processSignal(methodName, signature);
            explicitSignals.insert(methodName);

            QString slotName = QLatin1String("on");
            slotName += methodName.at(0).toUpper();
            slotName += methodName.midRef(1);

            // process the generated slot
            processor->processGeneratedSlot(slotName, signature);
        }
    }

    // process the meta properties
    for (int index = 0; index < _metaObject->propertyCount(); ++index) {
        const FakeMetaProperty prop = _metaObject->property(index);
        if (_componentVersion.isValid() && _componentVersion.minorVersion() < prop.revision())
            continue;

        const QString propertyName = prop.name();
        processor->processProperty(propertyName, propertyValue(prop));

        // every property always has a onXyzChanged slot, even if the NOTIFY
        // signal has a different name
        QString signalName = propertyName;
        signalName += QLatin1String("Changed");
        if (!explicitSignals.contains(signalName)) {
            QString slotName = QLatin1String("on");
            slotName += signalName.at(0).toUpper();
            slotName += signalName.midRef(1);

            // process the generated slot
            processor->processGeneratedSlot(slotName, valueOwner()->undefinedValue());
        }
    }

    if (_attachedType)
        _attachedType->processMembers(processor);

    ObjectValue::processMembers(processor);
}

const Value *QmlObjectValue::propertyValue(const FakeMetaProperty &prop) const
{
    QString typeName = prop.typeName();

    // ### Verify type resolving.
    QmlObjectValue *objectValue = valueOwner()->cppQmlTypes().typeByCppName(typeName);
    if (objectValue) {
        FakeMetaObject::Export exp = objectValue->metaObject()->exportInPackage(packageName());
        if (exp.isValid())
            objectValue = valueOwner()->cppQmlTypes().typeByQualifiedName(exp.packageNameVersion);
        return objectValue;
    }

    const Value *value = valueOwner()->undefinedValue();
    if (typeName == QLatin1String("QByteArray")
            || typeName == QLatin1String("string")
            || typeName == QLatin1String("QString")) {
        value = valueOwner()->stringValue();
    } else if (typeName == QLatin1String("QUrl")) {
        value = valueOwner()->urlValue();
    } else if (typeName == QLatin1String("bool")) {
        value = valueOwner()->booleanValue();
    } else if (typeName == QLatin1String("int")
               || typeName == QLatin1String("long")) {
        value = valueOwner()->intValue();
    }  else if (typeName == QLatin1String("float")
                || typeName == QLatin1String("double")
                || typeName == QLatin1String("qreal")) {
        // ### Review: more types here?
        value = valueOwner()->realValue();
    } else if (typeName == QLatin1String("QFont")) {
        value = valueOwner()->qmlFontObject();
    } else if (typeName == QLatin1String("QPoint")
            || typeName == QLatin1String("QPointF")
            || typeName == QLatin1String("QVector2D")) {
        value = valueOwner()->qmlPointObject();
    } else if (typeName == QLatin1String("QSize")
            || typeName == QLatin1String("QSizeF")) {
        value = valueOwner()->qmlSizeObject();
    } else if (typeName == QLatin1String("QRect")
            || typeName == QLatin1String("QRectF")) {
        value = valueOwner()->qmlRectObject();
    } else if (typeName == QLatin1String("QVector3D")) {
        value = valueOwner()->qmlVector3DObject();
    } else if (typeName == QLatin1String("QColor")) {
        value = valueOwner()->colorValue();
    } else if (typeName == QLatin1String("QDeclarativeAnchorLine")) {
        value = valueOwner()->anchorLineValue();
    }

    // might be an enum
    const QmlObjectValue *base = this;
    const QStringList components = typeName.split(QLatin1String("::"));
    if (components.size() == 2) {
        base = valueOwner()->cppQmlTypes().typeByCppName(components.first());
        typeName = components.last();
    }
    if (base) {
        const FakeMetaEnum &metaEnum = base->getEnum(typeName);
        if (metaEnum.isValid())
            value = new QmlEnumValue(metaEnum, valueOwner());
    }

    return value;
}

const QmlObjectValue *QmlObjectValue::prototype() const
{
    Q_ASSERT(!_prototype || dynamic_cast<const QmlObjectValue *>(_prototype));
    return static_cast<const QmlObjectValue *>(_prototype);
}

const QmlObjectValue *QmlObjectValue::attachedType() const
{
    return _attachedType;
}

void QmlObjectValue::setAttachedType(QmlObjectValue *value)
{
    _attachedType = value;
}

FakeMetaObject::ConstPtr QmlObjectValue::metaObject() const
{
    return _metaObject;
}

QString QmlObjectValue::packageName() const
{ return _packageName; }

ComponentVersion QmlObjectValue::version() const
{ return _componentVersion; }

QString QmlObjectValue::defaultPropertyName() const
{ return _metaObject->defaultPropertyName(); }

QString QmlObjectValue::propertyType(const QString &propertyName) const
{
    for (const QmlObjectValue *it = this; it; it = it->prototype()) {
        FakeMetaObject::ConstPtr iter = it->_metaObject;
        int propIdx = iter->propertyIndex(propertyName);
        if (propIdx != -1) {
            return iter->property(propIdx).typeName();
        }
    }
    return QString();
}

bool QmlObjectValue::isListProperty(const QString &propertyName) const
{
    for (const QmlObjectValue *it = this; it; it = it->prototype()) {
        FakeMetaObject::ConstPtr iter = it->_metaObject;
        int propIdx = iter->propertyIndex(propertyName);
        if (propIdx != -1) {
            return iter->property(propIdx).isList();
        }
    }
    return false;
}

bool QmlObjectValue::isEnum(const QString &typeName) const
{
    return _metaObject->enumeratorIndex(typeName) != -1;
}

FakeMetaEnum QmlObjectValue::getEnum(const QString &typeName) const
{
    const int index = _metaObject->enumeratorIndex(typeName);
    if (index == -1)
        return FakeMetaEnum();

    return _metaObject->enumerator(index);
}

bool QmlObjectValue::isWritable(const QString &propertyName) const
{
    for (const QmlObjectValue *it = this; it; it = it->prototype()) {
        FakeMetaObject::ConstPtr iter = it->_metaObject;
        int propIdx = iter->propertyIndex(propertyName);
        if (propIdx != -1) {
            return iter->property(propIdx).isWritable();
        }
    }
    return false;
}

bool QmlObjectValue::isPointer(const QString &propertyName) const
{
    for (const QmlObjectValue *it = this; it; it = it->prototype()) {
        FakeMetaObject::ConstPtr iter = it->_metaObject;
        int propIdx = iter->propertyIndex(propertyName);
        if (propIdx != -1) {
            return iter->property(propIdx).isPointer();
        }
    }
    return false;
}

bool QmlObjectValue::hasLocalProperty(const QString &typeName) const
{
    int idx = _metaObject->propertyIndex(typeName);
    if (idx == -1)
        return false;
    return true;
}

bool QmlObjectValue::hasProperty(const QString &propertyName) const
{
    for (const QmlObjectValue *it = this; it; it = it->prototype()) {
        FakeMetaObject::ConstPtr iter = it->_metaObject;
        int propIdx = iter->propertyIndex(propertyName);
        if (propIdx != -1) {
            return true;
        }
    }
    return false;
}

bool QmlObjectValue::enumContainsKey(const QString &enumName, const QString &enumKeyName) const
{
    int idx = _metaObject->enumeratorIndex(enumName);
    if (idx == -1)
        return false;
    const FakeMetaEnum &fme = _metaObject->enumerator(idx);
    for (int i = 0; i < fme.keyCount(); ++i) {
        if (fme.key(i) == enumKeyName)
            return true;
    }
    return false;
}

QStringList QmlObjectValue::keysForEnum(const QString &enumName) const
{
    int idx = _metaObject->enumeratorIndex(enumName);
    if (idx == -1)
        return QStringList();
    const FakeMetaEnum &fme = _metaObject->enumerator(idx);
    return fme.keys();
}

// Returns true if this object is in a package or if there is an object that
// has this one in its prototype chain and is itself in a package.
bool QmlObjectValue::hasChildInPackage() const
{
    if (!packageName().isEmpty()
            && packageName() != CppQmlTypes::cppPackage)
        return true;
    QHashIterator<QString, QmlObjectValue *> it(valueOwner()->cppQmlTypes().types());
    while (it.hasNext()) {
        it.next();
        FakeMetaObject::ConstPtr otherMeta = it.value()->_metaObject;
        // if it has only a cpp-package export, it is not really exported
        if (otherMeta->exports().size() <= 1)
            continue;
        for (const QmlObjectValue *other = it.value(); other; other = other->prototype()) {
            if (other->metaObject() == _metaObject) // this object is a parent of other
                return true;
        }
    }
    return false;
}

bool QmlObjectValue::isDerivedFrom(FakeMetaObject::ConstPtr base) const
{
    for (const QmlObjectValue *it = this; it; it = it->prototype()) {
        FakeMetaObject::ConstPtr iter = it->_metaObject;
        if (iter == base)
            return true;
    }
    return false;
}

QmlEnumValue::QmlEnumValue(const FakeMetaEnum &metaEnum, ValueOwner *valueOwner)
    : NumberValue(),
      _metaEnum(new FakeMetaEnum(metaEnum))
{
    valueOwner->registerValue(this);
}

QmlEnumValue::~QmlEnumValue()
{
    delete _metaEnum;
}

QString QmlEnumValue::name() const
{
    return _metaEnum->name();
}

QStringList QmlEnumValue::keys() const
{
    return _metaEnum->keys();
}

////////////////////////////////////////////////////////////////////////////////
// ValueVisitor
////////////////////////////////////////////////////////////////////////////////
ValueVisitor::ValueVisitor()
{
}

ValueVisitor::~ValueVisitor()
{
}

void ValueVisitor::visit(const NullValue *)
{
}

void ValueVisitor::visit(const UndefinedValue *)
{
}

void ValueVisitor::visit(const NumberValue *)
{
}

void ValueVisitor::visit(const BooleanValue *)
{
}

void ValueVisitor::visit(const StringValue *)
{
}

void ValueVisitor::visit(const ObjectValue *)
{
}

void ValueVisitor::visit(const FunctionValue *)
{
}

void ValueVisitor::visit(const Reference *)
{
}

void ValueVisitor::visit(const ColorValue *)
{
}

void ValueVisitor::visit(const AnchorLineValue *)
{
}

////////////////////////////////////////////////////////////////////////////////
// Value
////////////////////////////////////////////////////////////////////////////////
Value::Value()
{
}

Value::~Value()
{
}

bool Value::getSourceLocation(QString *, int *, int *) const
{
    return false;
}

const NullValue *Value::asNullValue() const
{
    return 0;
}

const UndefinedValue *Value::asUndefinedValue() const
{
    return 0;
}

const NumberValue *Value::asNumberValue() const
{
    return 0;
}

const IntValue *Value::asIntValue() const
{
    return 0;
}

const RealValue *Value::asRealValue() const
{
    return 0;
}

const BooleanValue *Value::asBooleanValue() const
{
    return 0;
}

const StringValue *Value::asStringValue() const
{
    return 0;
}

const UrlValue *Value::asUrlValue() const
{
    return 0;
}

const ObjectValue *Value::asObjectValue() const
{
    return 0;
}

const FunctionValue *Value::asFunctionValue() const
{
    return 0;
}

const Reference *Value::asReference() const
{
    return 0;
}

const ColorValue *Value::asColorValue() const
{
    return 0;
}

const AnchorLineValue *Value::asAnchorLineValue() const
{
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Values
////////////////////////////////////////////////////////////////////////////////
const NullValue *NullValue::asNullValue() const
{
    return this;
}

void NullValue::accept(ValueVisitor *visitor) const
{
    visitor->visit(this);
}

const UndefinedValue *UndefinedValue::asUndefinedValue() const
{
    return this;
}

void UndefinedValue::accept(ValueVisitor *visitor) const
{
    visitor->visit(this);
}

const NumberValue *NumberValue::asNumberValue() const
{
    return this;
}

const RealValue *RealValue::asRealValue() const
{
    return this;
}

const IntValue *IntValue::asIntValue() const
{
    return this;
}

void NumberValue::accept(ValueVisitor *visitor) const
{
    visitor->visit(this);
}

const BooleanValue *BooleanValue::asBooleanValue() const
{
    return this;
}

void BooleanValue::accept(ValueVisitor *visitor) const
{
    visitor->visit(this);
}

const StringValue *StringValue::asStringValue() const
{
    return this;
}

const UrlValue *UrlValue::asUrlValue() const
{
    return this;
}

void StringValue::accept(ValueVisitor *visitor) const
{
    visitor->visit(this);
}


ScopeChain::ScopeChain()
    : globalScope(0)
    , qmlTypes(0)
    , jsImports(0)
{
}

ScopeChain::QmlComponentChain::QmlComponentChain()
{
}

ScopeChain::QmlComponentChain::~QmlComponentChain()
{
    qDeleteAll(instantiatingComponents);
}

void ScopeChain::QmlComponentChain::clear()
{
    qDeleteAll(instantiatingComponents);
    instantiatingComponents.clear();
    document = Document::Ptr(0);
}

void ScopeChain::QmlComponentChain::collect(QList<const ObjectValue *> *list) const
{
    foreach (const QmlComponentChain *parent, instantiatingComponents)
        parent->collect(list);

    if (!document)
        return;

    if (ObjectValue *root = document->bind()->rootObjectValue())
        list->append(root);
    if (ObjectValue *ids = document->bind()->idEnvironment())
        list->append(ids);
}

void ScopeChain::update()
{
    _all.clear();

    _all += globalScope;

    // the root scope in js files doesn't see instantiating components
    if (jsScopes.count() != 1 || !qmlScopeObjects.isEmpty()) {
        if (qmlComponentScope) {
            foreach (const QmlComponentChain *parent, qmlComponentScope->instantiatingComponents)
                parent->collect(&_all);
        }
    }

    ObjectValue *root = 0;
    ObjectValue *ids = 0;
    if (qmlComponentScope && qmlComponentScope->document) {
        root = qmlComponentScope->document->bind()->rootObjectValue();
        ids = qmlComponentScope->document->bind()->idEnvironment();
    }

    if (root && !qmlScopeObjects.contains(root))
        _all += root;
    _all += qmlScopeObjects;
    if (ids)
        _all += ids;
    if (qmlTypes)
        _all += qmlTypes;
    if (jsImports)
        _all += jsImports;
    _all += jsScopes;
}

QList<const ObjectValue *> ScopeChain::all() const
{
    return _all;
}


Context::Context(const QmlJS::Snapshot &snapshot)
    : _snapshot(snapshot),
      _valueOwner(new ValueOwner),
      _qmlScopeObjectIndex(-1),
      _qmlScopeObjectSet(false)
{
}

Context::~Context()
{
}

// the values is only guaranteed to live as long as the context
ValueOwner *Context::valueOwner() const
{
    return _valueOwner.data();
}

QmlJS::Snapshot Context::snapshot() const
{
    return _snapshot;
}

const ScopeChain &Context::scopeChain() const
{
    return _scopeChain;
}

ScopeChain &Context::scopeChain()
{
    return _scopeChain;
}

const Imports *Context::imports(const QmlJS::Document *doc) const
{
    if (!doc)
        return 0;
    return _imports.value(doc).data();
}

void Context::setImports(const QmlJS::Document *doc, const Imports *imports)
{
    if (!doc)
        return;
    _imports[doc] = QSharedPointer<const Imports>(imports);
}

const Value *Context::lookup(const QString &name, const ObjectValue **foundInScope) const
{
    QList<const ObjectValue *> scopes = _scopeChain.all();
    for (int index = scopes.size() - 1; index != -1; --index) {
        const ObjectValue *scope = scopes.at(index);

        if (const Value *member = scope->lookupMember(name, this)) {
            if (foundInScope)
                *foundInScope = scope;
            return member;
        }
    }

    if (foundInScope)
        *foundInScope = 0;
    return _valueOwner->undefinedValue();
}

const ObjectValue *Context::lookupType(const QmlJS::Document *doc, UiQualifiedId *qmlTypeName,
                                       UiQualifiedId *qmlTypeNameEnd) const
{
    const Imports *importsObj = imports(doc);
    if (!importsObj)
        return 0;
    const ObjectValue *objectValue = importsObj->typeScope();
    if (!objectValue)
        return 0;

    for (UiQualifiedId *iter = qmlTypeName; objectValue && iter && iter != qmlTypeNameEnd;
         iter = iter->next) {
        if (! iter->name)
            return 0;

        const Value *value = objectValue->lookupMember(iter->name->asString(), this);
        if (!value)
            return 0;

        objectValue = value->asObjectValue();
    }

    return objectValue;
}

const ObjectValue *Context::lookupType(const QmlJS::Document *doc, const QStringList &qmlTypeName) const
{
    const Imports *importsObj = imports(doc);
    if (!importsObj)
        return 0;
    const ObjectValue *objectValue = importsObj->typeScope();
    if (!objectValue)
        return 0;

    foreach (const QString &name, qmlTypeName) {
        if (!objectValue)
            return 0;

        const Value *value = objectValue->lookupMember(name, this);
        if (!value)
            return 0;

        objectValue = value->asObjectValue();
    }

    return objectValue;
}

const Value *Context::lookupReference(const Value *value) const
{
    const Reference *reference = value_cast<const Reference *>(value);
    if (!reference)
        return value;

    if (_referenceStack.contains(reference))
        return 0;

    _referenceStack.append(reference);
    const Value *v = reference->value(this);
    _referenceStack.removeLast();

    return v;
}

QString Context::defaultPropertyName(const ObjectValue *object) const
{
    PrototypeIterator iter(object, this);
    while (iter.hasNext()) {
        const ObjectValue *o = iter.next();
        if (const ASTObjectValue *astObjValue = dynamic_cast<const ASTObjectValue *>(o)) {
            QString defaultProperty = astObjValue->defaultPropertyName();
            if (!defaultProperty.isEmpty())
                return defaultProperty;
        } else if (const QmlObjectValue *qmlValue = dynamic_cast<const QmlObjectValue *>(o)) {
            return qmlValue->defaultPropertyName();
        }
    }
    return QString();
}

Reference::Reference(ValueOwner *valueOwner)
    : _valueOwner(valueOwner)
{
    _valueOwner->registerValue(this);
}

Reference::~Reference()
{
}

ValueOwner *Reference::valueOwner() const
{
    return _valueOwner;
}

const Reference *Reference::asReference() const
{
    return this;
}

void Reference::accept(ValueVisitor *visitor) const
{
    visitor->visit(this);
}

const Value *Reference::value(const Context *) const
{
    return _valueOwner->undefinedValue();
}

void ColorValue::accept(ValueVisitor *visitor) const
{
    visitor->visit(this);
}

const ColorValue *ColorValue::asColorValue() const
{
    return this;
}

void AnchorLineValue::accept(ValueVisitor *visitor) const
{
    visitor->visit(this);
}

const AnchorLineValue *AnchorLineValue::asAnchorLineValue() const
{
    return this;
}

MemberProcessor::MemberProcessor()
{
}

MemberProcessor::~MemberProcessor()
{
}

bool MemberProcessor::processProperty(const QString &, const Value *)
{
    return true;
}

bool MemberProcessor::processEnumerator(const QString &, const Value *)
{
    return true;
}

bool MemberProcessor::processSignal(const QString &, const Value *)
{
    return true;
}

bool MemberProcessor::processSlot(const QString &, const Value *)
{
    return true;
}

bool MemberProcessor::processGeneratedSlot(const QString &, const Value *)
{
    return true;
}

ObjectValue::ObjectValue(ValueOwner *valueOwner)
    : _valueOwner(valueOwner),
      _prototype(0)
{
    valueOwner->registerValue(this);
}

ObjectValue::~ObjectValue()
{
}

ValueOwner *ObjectValue::valueOwner() const
{
    return _valueOwner;
}

QString ObjectValue::className() const
{
    return _className;
}

void ObjectValue::setClassName(const QString &className)
{
    _className = className;
}

const Value *ObjectValue::prototype() const
{
    return _prototype;
}

const ObjectValue *ObjectValue::prototype(const Context *context) const
{
    const ObjectValue *prototypeObject = value_cast<const ObjectValue *>(_prototype);
    if (! prototypeObject) {
        if (const Reference *prototypeReference = value_cast<const Reference *>(_prototype)) {
            prototypeObject = value_cast<const ObjectValue *>(context->lookupReference(prototypeReference));
        }
    }
    return prototypeObject;
}

void ObjectValue::setPrototype(const Value *prototype)
{
    _prototype = prototype;
}

void ObjectValue::setMember(const QString &name, const Value *value)
{
    _members[name] = value;
}

void ObjectValue::removeMember(const QString &name)
{
    _members.remove(name);
}

const ObjectValue *ObjectValue::asObjectValue() const
{
    return this;
}

void ObjectValue::accept(ValueVisitor *visitor) const
{
    visitor->visit(this);
}

bool ObjectValue::checkPrototype(const ObjectValue *, QSet<const ObjectValue *> *) const
{
#if 0
    const int previousSize = processed->size();
    processed->insert(this);

    if (previousSize != processed->size()) {
        if (this == proto)
            return false;

        if (prototype() && ! prototype()->checkPrototype(proto, processed))
            return false;

        return true;
    }
#endif
    return false;
}

void ObjectValue::processMembers(MemberProcessor *processor) const
{
    QHashIterator<QString, const Value *> it(_members);

    while (it.hasNext()) {
        it.next();

        if (! processor->processProperty(it.key(), it.value()))
            break;
    }
}

const Value *ObjectValue::lookupMember(const QString &name, const Context *context,
                                       const ObjectValue **foundInObject,
                                       bool examinePrototypes) const
{
    if (const Value *m = _members.value(name)) {
        if (foundInObject)
            *foundInObject = this;
        return m;
    } else {
        LookupMember slowLookup(name);
        processMembers(&slowLookup);
        if (slowLookup.value()) {
            if (foundInObject)
                *foundInObject = this;
            return slowLookup.value();
        }
    }

    if (examinePrototypes) {
        PrototypeIterator iter(this, context);
        iter.next(); // skip this
        while (iter.hasNext()) {
            const ObjectValue *prototypeObject = iter.next();
            if (const Value *m = prototypeObject->lookupMember(name, context, foundInObject, false))
                return m;
        }
    }

    if (foundInObject)
        *foundInObject = 0;
    return 0;
}

PrototypeIterator::PrototypeIterator(const ObjectValue *start, const Context *context)
    : m_current(0)
    , m_next(start)
    , m_context(context)
    , m_error(NoError)
{
    if (start)
        m_prototypes.reserve(10);
}

bool PrototypeIterator::hasNext()
{
    if (m_next)
        return true;
    if (!m_current)
        return false;
    const Value *proto = m_current->prototype();
    if (!proto)
        return false;

    m_next = value_cast<const ObjectValue *>(proto);
    if (! m_next)
        m_next = value_cast<const ObjectValue *>(m_context->lookupReference(proto));
    if (!m_next) {
        m_error = ReferenceResolutionError;
        return false;
    }
    if (m_prototypes.contains(m_next)) {
        m_error = CycleError;
        m_next = 0;
        return false;
    }
    return true;
}

const ObjectValue *PrototypeIterator::next()
{
    if (hasNext()) {
        m_current = m_next;
        m_prototypes += m_next;
        m_next = 0;
        return m_current;
    }
    return 0;
}

const ObjectValue *PrototypeIterator::peekNext()
{
    if (hasNext()) {
        return m_next;
    }
    return 0;
}

PrototypeIterator::Error PrototypeIterator::error() const
{
    return m_error;
}

QList<const ObjectValue *> PrototypeIterator::all()
{
    while (hasNext())
        next();
    return m_prototypes;
}

Activation::Activation(Context *parentContext)
    : _thisObject(0),
      _calledAsFunction(true),
      _parentContext(parentContext)
{
}

Activation::~Activation()
{
}

Context *Activation::parentContext() const
{
    return _parentContext;
}

Context *Activation::context() const
{
    // ### FIXME: Real context for activations.
    return 0;
}

bool Activation::calledAsConstructor() const
{
    return ! _calledAsFunction;
}

void Activation::setCalledAsConstructor(bool calledAsConstructor)
{
    _calledAsFunction = ! calledAsConstructor;
}

bool Activation::calledAsFunction() const
{
    return _calledAsFunction;
}

void Activation::setCalledAsFunction(bool calledAsFunction)
{
    _calledAsFunction = calledAsFunction;
}

ObjectValue *Activation::thisObject() const
{
    return _thisObject;
}

void Activation::setThisObject(ObjectValue *thisObject)
{
    _thisObject = thisObject;
}

ValueList Activation::arguments() const
{
    return _arguments;
}

void Activation::setArguments(const ValueList &arguments)
{
    _arguments = arguments;
}

FunctionValue::FunctionValue(ValueOwner *valueOwner)
    : ObjectValue(valueOwner)
{
}

FunctionValue::~FunctionValue()
{
}

const Value *FunctionValue::construct(const ValueList &actuals) const
{
    Activation activation;
    activation.setCalledAsConstructor(true);
    activation.setThisObject(valueOwner()->newObject());
    activation.setArguments(actuals);
    return invoke(&activation);
}

const Value *FunctionValue::call(const ValueList &actuals) const
{
    Activation activation;
    activation.setCalledAsFunction(true);
    activation.setThisObject(valueOwner()->globalObject()); // ### FIXME: it should be `null'
    activation.setArguments(actuals);
    return invoke(&activation);
}

const Value *FunctionValue::call(const ObjectValue *thisObject, const ValueList &actuals) const
{
    Activation activation;
    activation.setCalledAsFunction(true);
    activation.setThisObject(const_cast<ObjectValue *>(thisObject)); // ### FIXME: remove the const_cast
    activation.setArguments(actuals);
    return invoke(&activation);
}

const Value *FunctionValue::returnValue() const
{
    return valueOwner()->undefinedValue();
}

int FunctionValue::argumentCount() const
{
    return 0;
}

const Value *FunctionValue::argument(int) const
{
    return valueOwner()->undefinedValue();
}

QString FunctionValue::argumentName(int index) const
{
    return QString::fromLatin1("arg%1").arg(index + 1);
}

bool FunctionValue::isVariadic() const
{
    return true;
}

const Value *FunctionValue::invoke(const Activation *activation) const
{
    return activation->thisObject(); // ### FIXME: it should return undefined
}

const FunctionValue *FunctionValue::asFunctionValue() const
{
    return this;
}

void FunctionValue::accept(ValueVisitor *visitor) const
{
    visitor->visit(this);
}

Function::Function(ValueOwner *valueOwner)
    : FunctionValue(valueOwner), _returnValue(0)
{
    setClassName("Function");
}

Function::~Function()
{
}

void Function::addArgument(const Value *argument)
{
    _arguments.push_back(argument);
}

const Value *Function::returnValue() const
{
    return _returnValue;
}

void Function::setReturnValue(const Value *returnValue)
{
    _returnValue = returnValue;
}

int Function::argumentCount() const
{
    return _arguments.size();
}

const Value *Function::argument(int index) const
{
    return _arguments.at(index);
}

const Value *Function::lookupMember(const QString &name, const Context *context,
                                    const ObjectValue **foundInScope, bool examinePrototypes) const
{
    if (name == "length") {
        if (foundInScope)
            *foundInScope = this;
        return valueOwner()->numberValue();
    }

    return FunctionValue::lookupMember(name, context, foundInScope, examinePrototypes);
}

const Value *Function::invoke(const Activation *activation) const
{
    return activation->thisObject(); // ### FIXME it should return undefined
}

////////////////////////////////////////////////////////////////////////////////
// typing environment
////////////////////////////////////////////////////////////////////////////////

QHash<QString, FakeMetaObject::ConstPtr> CppQmlTypesLoader::builtinObjects;
QHash<QString, QList<LanguageUtils::ComponentVersion> > CppQmlTypesLoader::builtinPackages;

void CppQmlTypesLoader::loadQmlTypes(const QFileInfoList &qmlTypeFiles, QStringList *errors, QStringList *warnings)
{
    QHash<QString, FakeMetaObject::ConstPtr> newObjects;

    foreach (const QFileInfo &qmlTypeFile, qmlTypeFiles) {
        QString error, warning;
        QFile file(qmlTypeFile.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly)) {
            QString contents = QString::fromUtf8(file.readAll());
            file.close();

            QmlJS::TypeDescriptionReader reader(contents);
            if (!reader(&newObjects))
                error = reader.errorMessage();
            warning = reader.warningMessage();
        } else {
            error = file.errorString();
        }
        if (!error.isEmpty()) {
            errors->append(TypeDescriptionReader::tr(
                               "Errors while loading qmltypes from %1:\n%2").arg(
                               qmlTypeFile.absoluteFilePath(), error));
        }
        if (!warning.isEmpty()) {
            warnings->append(TypeDescriptionReader::tr(
                                 "Warnings while loading qmltypes from %1:\n%2").arg(
                                 qmlTypeFile.absoluteFilePath(), error));
        }
    }

    builtinObjects.unite(newObjects);

    QHash<QString, LanguageUtils::FakeMetaObject::ConstPtr>::const_iterator iter
            = builtinObjects.constBegin();
    for (; iter != builtinObjects.constEnd(); iter++) {
        foreach (const FakeMetaObject::Export &exp, iter.value().data()->exports()) {
            QList<LanguageUtils::ComponentVersion> versions = builtinPackages.value(exp.package);
            if (!versions.contains(exp.version)) {
                versions.append(exp.version);
                builtinPackages.insert(exp.package, versions);
            }
        }
    }
}

void CppQmlTypesLoader::parseQmlTypeDescriptions(const QByteArray &xml,
                                                 QHash<QString, FakeMetaObject::ConstPtr> *newObjects,
                                                 QString *errorMessage,
                                                 QString *warningMessage)
{
    errorMessage->clear();
    warningMessage->clear();
    QmlJS::TypeDescriptionReader reader(QString::fromUtf8(xml));
    if (!reader(newObjects)) {
        if (reader.errorMessage().isEmpty()) {
            *errorMessage = QLatin1String("unknown error");
        } else {
            *errorMessage = reader.errorMessage();
        }
    }
    *warningMessage = reader.warningMessage();
}

const QLatin1String CppQmlTypes::defaultPackage("<default>");
const QLatin1String CppQmlTypes::cppPackage("<cpp>");

template <typename T>
QList<QmlObjectValue *> CppQmlTypes::load(ValueOwner *valueOwner, const T &objects)
{
    // load
    QList<QmlObjectValue *> loadedObjects;
    QList<QmlObjectValue *> newObjects;
    foreach (FakeMetaObject::ConstPtr metaObject, objects) {
        foreach (const FakeMetaObject::Export &exp, metaObject->exports()) {
            bool wasCreated;
            QmlObjectValue *loadedObject = getOrCreate(valueOwner, metaObject, exp, &wasCreated);
            loadedObjects += loadedObject;
            if (wasCreated)
                newObjects += loadedObject;
        }
    }

    // set prototypes
    foreach (QmlObjectValue *object, newObjects) {
        setPrototypes(object);
    }

    return loadedObjects;
}
// explicitly instantiate load for list and hash
template QList<QmlObjectValue *> CppQmlTypes::load< QList<FakeMetaObject::ConstPtr> >(ValueOwner *, const QList<FakeMetaObject::ConstPtr> &);
template QList<QmlObjectValue *> CppQmlTypes::load< QHash<QString, FakeMetaObject::ConstPtr> >(ValueOwner *, const QHash<QString, FakeMetaObject::ConstPtr> &);

QList<QmlObjectValue *> CppQmlTypes::typesForImport(const QString &packageName, ComponentVersion version) const
{
    QMap<QString, QmlObjectValue *> objectValuesByName;

    foreach (QmlObjectValue *qmlObjectValue, _typesByPackage.value(packageName)) {
        if (qmlObjectValue->version() <= version) {
            // we got a candidate.
            const QString typeName = qmlObjectValue->className();
            QmlObjectValue *previousCandidate = objectValuesByName.value(typeName, 0);
            if (previousCandidate) {
                // check if our new candidate is newer than the one we found previously
                if (previousCandidate->version() < qmlObjectValue->version()) {
                    // the new candidate has a higher version no. than the one we found previously, so replace it
                    objectValuesByName.insert(typeName, qmlObjectValue);
                }
            } else {
                objectValuesByName.insert(typeName, qmlObjectValue);
            }
        }
    }

    return objectValuesByName.values();
}

QmlObjectValue *CppQmlTypes::typeByCppName(const QString &cppName) const
{
    return typeByQualifiedName(cppPackage, cppName, ComponentVersion());
}

bool CppQmlTypes::hasPackage(const QString &package) const
{
    return _typesByPackage.contains(package);
}

QString CppQmlTypes::qualifiedName(const QString &package, const QString &type, ComponentVersion version)
{
    return QString("%1/%2 %3").arg(
                package, type,
                version.toString());

}

QmlObjectValue *CppQmlTypes::typeByQualifiedName(const QString &name) const
{
    return _typesByFullyQualifiedName.value(name);
}

QmlObjectValue *CppQmlTypes::typeByQualifiedName(const QString &package, const QString &type, ComponentVersion version) const
{
    return typeByQualifiedName(qualifiedName(package, type, version));
}

QmlObjectValue *CppQmlTypes::getOrCreate(
    ValueOwner *valueOwner,
    FakeMetaObject::ConstPtr metaObject,
    const LanguageUtils::FakeMetaObject::Export &exp,
    bool *wasCreated)
{
    // make sure we're not loading duplicate objects
    if (QmlObjectValue *existing = _typesByFullyQualifiedName.value(exp.packageNameVersion)) {
        if (wasCreated)
            *wasCreated = false;
        return existing;
    }

    QmlObjectValue *objectValue = new QmlObjectValue(
                metaObject, exp.type, exp.package, exp.version, valueOwner);
    _typesByPackage[exp.package].append(objectValue);
    _typesByFullyQualifiedName[exp.packageNameVersion] = objectValue;

    if (wasCreated)
        *wasCreated = true;
    return objectValue;
}

void CppQmlTypes::setPrototypes(QmlObjectValue *object)
{
    if (!object)
        return;

    FakeMetaObject::ConstPtr fmo = object->metaObject();

    // resolve attached type
    // don't do it if the attached type name is the object itself (happens for QDeclarativeKeysAttached)
    if (!fmo->attachedTypeName().isEmpty()
            && fmo->className() != fmo->attachedTypeName()) {
        QmlObjectValue *attachedObject = typeByCppName(fmo->attachedTypeName());
        if (attachedObject && attachedObject != object)
            object->setAttachedType(attachedObject);
    }

    const QString targetPackage = object->packageName();

    // set prototypes for whole chain, creating new QmlObjectValues if necessary
    // for instance, if an type isn't exported in the package of the super type
    // Example: QObject (Qt, QtQuick) -> Positioner (not exported) -> Column (Qt, QtQuick)
    // needs to create Positioner (Qt) and Positioner (QtQuick)
    QmlObjectValue *v = object;
    while (!v->prototype() && !fmo->superclassName().isEmpty()) {
        QmlObjectValue *superValue = getOrCreateForPackage(targetPackage, fmo->superclassName());
        if (!superValue)
            return;
        v->setPrototype(superValue);
        v = superValue;
        fmo = v->metaObject();
    }
}

QmlObjectValue *CppQmlTypes::getOrCreateForPackage(const QString &package, const QString &cppName)
{
    // first get the cpp object value
    QmlObjectValue *cppObject = typeByCppName(cppName);
    if (!cppObject) {
        // ### disabled for now, should be communicated to the user somehow.
        //qWarning() << "QML type system: could not find '" << cppName << "'";
        return 0;
    }

    FakeMetaObject::ConstPtr metaObject = cppObject->metaObject();
    FakeMetaObject::Export exp = metaObject->exportInPackage(package);
    QmlObjectValue *object = 0;
    if (exp.isValid()) {
        object = getOrCreate(cppObject->valueOwner(), metaObject, exp);
    } else {
        // make a convenience object that does not get added to _typesByPackage
        const QString qname = qualifiedName(package, cppName, ComponentVersion());
        object = typeByQualifiedName(qname);
        if (!object) {
            object = new QmlObjectValue(
                        metaObject, cppName, package, ComponentVersion(), cppObject->valueOwner());
            _typesByFullyQualifiedName[qname] = object;
        }
    }

    return object;
}

ConvertToNumber::ConvertToNumber(ValueOwner *valueOwner)
    : _valueOwner(valueOwner), _result(0)
{
}

const Value *ConvertToNumber::operator()(const Value *value)
{
    const Value *previousValue = switchResult(0);

    if (value)
        value->accept(this);

    return switchResult(previousValue);
}

const Value *ConvertToNumber::switchResult(const Value *value)
{
    const Value *previousResult = _result;
    _result = value;
    return previousResult;
}

void ConvertToNumber::visit(const NullValue *)
{
    _result = _valueOwner->numberValue();
}

void ConvertToNumber::visit(const UndefinedValue *)
{
    _result = _valueOwner->numberValue();
}

void ConvertToNumber::visit(const NumberValue *value)
{
    _result = value;
}

void ConvertToNumber::visit(const BooleanValue *)
{
    _result = _valueOwner->numberValue();
}

void ConvertToNumber::visit(const StringValue *)
{
    _result = _valueOwner->numberValue();
}

void ConvertToNumber::visit(const ObjectValue *object)
{
    if (const FunctionValue *valueOfMember = value_cast<const FunctionValue *>(object->lookupMember("valueOf", 0))) {
        _result = value_cast<const NumberValue *>(valueOfMember->call(object)); // ### invoke convert-to-number?
    }
}

void ConvertToNumber::visit(const FunctionValue *object)
{
    if (const FunctionValue *valueOfMember = value_cast<const FunctionValue *>(object->lookupMember("valueOf", 0))) {
        _result = value_cast<const NumberValue *>(valueOfMember->call(object)); // ### invoke convert-to-number?
    }
}

ConvertToString::ConvertToString(ValueOwner *valueOwner)
    : _valueOwner(valueOwner), _result(0)
{
}

const Value *ConvertToString::operator()(const Value *value)
{
    const Value *previousValue = switchResult(0);

    if (value)
        value->accept(this);

    return switchResult(previousValue);
}

const Value *ConvertToString::switchResult(const Value *value)
{
    const Value *previousResult = _result;
    _result = value;
    return previousResult;
}

void ConvertToString::visit(const NullValue *)
{
    _result = _valueOwner->stringValue();
}

void ConvertToString::visit(const UndefinedValue *)
{
    _result = _valueOwner->stringValue();
}

void ConvertToString::visit(const NumberValue *)
{
    _result = _valueOwner->stringValue();
}

void ConvertToString::visit(const BooleanValue *)
{
    _result = _valueOwner->stringValue();
}

void ConvertToString::visit(const StringValue *value)
{
    _result = value;
}

void ConvertToString::visit(const ObjectValue *object)
{
    if (const FunctionValue *toStringMember = value_cast<const FunctionValue *>(object->lookupMember("toString", 0))) {
        _result = value_cast<const StringValue *>(toStringMember->call(object)); // ### invoke convert-to-string?
    }
}

void ConvertToString::visit(const FunctionValue *object)
{
    if (const FunctionValue *toStringMember = value_cast<const FunctionValue *>(object->lookupMember("toString", 0))) {
        _result = value_cast<const StringValue *>(toStringMember->call(object)); // ### invoke convert-to-string?
    }
}

ConvertToObject::ConvertToObject(ValueOwner *valueOwner)
    : _valueOwner(valueOwner), _result(0)
{
}

const Value *ConvertToObject::operator()(const Value *value)
{
    const Value *previousValue = switchResult(0);

    if (value)
        value->accept(this);

    return switchResult(previousValue);
}

const Value *ConvertToObject::switchResult(const Value *value)
{
    const Value *previousResult = _result;
    _result = value;
    return previousResult;
}

void ConvertToObject::visit(const NullValue *value)
{
    _result = value;
}

void ConvertToObject::visit(const UndefinedValue *)
{
    _result = _valueOwner->nullValue();
}

void ConvertToObject::visit(const NumberValue *value)
{
    ValueList actuals;
    actuals.append(value);
    _result = _valueOwner->numberCtor()->construct(actuals);
}

void ConvertToObject::visit(const BooleanValue *value)
{
    ValueList actuals;
    actuals.append(value);
    _result = _valueOwner->booleanCtor()->construct(actuals);
}

void ConvertToObject::visit(const StringValue *value)
{
    ValueList actuals;
    actuals.append(value);
    _result = _valueOwner->stringCtor()->construct(actuals);
}

void ConvertToObject::visit(const ObjectValue *object)
{
    _result = object;
}

void ConvertToObject::visit(const FunctionValue *object)
{
    _result = object;
}

QString TypeId::operator()(const Value *value)
{
    _result = QLatin1String("unknown");

    if (value)
        value->accept(this);

    return _result;
}

void TypeId::visit(const NullValue *)
{
    _result = QLatin1String("null");
}

void TypeId::visit(const UndefinedValue *)
{
    _result = QLatin1String("undefined");
}

void TypeId::visit(const NumberValue *)
{
    _result = QLatin1String("number");
}

void TypeId::visit(const BooleanValue *)
{
    _result = QLatin1String("boolean");
}

void TypeId::visit(const StringValue *)
{
    _result = QLatin1String("string");
}

void TypeId::visit(const ObjectValue *object)
{
    _result = object->className();

    if (_result.isEmpty())
        _result = QLatin1String("object");
}

void TypeId::visit(const FunctionValue *object)
{
    _result = object->className();

    if (_result.isEmpty())
        _result = QLatin1String("Function");
}

void TypeId::visit(const ColorValue *)
{
    _result = QLatin1String("string");
}

void TypeId::visit(const AnchorLineValue *)
{
    _result = QLatin1String("AnchorLine");
}

ASTObjectValue::ASTObjectValue(UiQualifiedId *typeName,
                               UiObjectInitializer *initializer,
                               const QmlJS::Document *doc,
                               ValueOwner *valueOwner)
    : ObjectValue(valueOwner), _typeName(typeName), _initializer(initializer), _doc(doc), _defaultPropertyRef(0)
{
    if (_initializer) {
        for (UiObjectMemberList *it = _initializer->members; it; it = it->next) {
            UiObjectMember *member = it->member;
            if (UiPublicMember *def = cast<UiPublicMember *>(member)) {
                if (def->type == UiPublicMember::Property && def->name && def->memberType) {
                    ASTPropertyReference *ref = new ASTPropertyReference(def, _doc, valueOwner);
                    _properties.append(ref);
                    if (def->defaultToken.isValid())
                        _defaultPropertyRef = ref;
                } else if (def->type == UiPublicMember::Signal && def->name) {
                    ASTSignalReference *ref = new ASTSignalReference(def, _doc, valueOwner);
                    _signals.append(ref);
                }
            }
        }
    }
}

ASTObjectValue::~ASTObjectValue()
{
}

bool ASTObjectValue::getSourceLocation(QString *fileName, int *line, int *column) const
{
    *fileName = _doc->fileName();
    *line = _typeName->identifierToken.startLine;
    *column = _typeName->identifierToken.startColumn;
    return true;
}

void ASTObjectValue::processMembers(MemberProcessor *processor) const
{
    foreach (ASTPropertyReference *ref, _properties) {
        processor->processProperty(ref->ast()->name->asString(), ref);
        // ### Should get a different value?
        processor->processGeneratedSlot(ref->onChangedSlotName(), ref);
    }
    foreach (ASTSignalReference *ref, _signals) {
        processor->processSignal(ref->ast()->name->asString(), ref);
        // ### Should get a different value?
        processor->processGeneratedSlot(ref->slotName(), ref);
    }

    ObjectValue::processMembers(processor);
}

QString ASTObjectValue::defaultPropertyName() const
{
    if (_defaultPropertyRef) {
        UiPublicMember *prop = _defaultPropertyRef->ast();
        if (prop && prop->name)
            return prop->name->asString();
    }
    return QString();
}

UiObjectInitializer *ASTObjectValue::initializer() const
{
    return _initializer;
}

UiQualifiedId *ASTObjectValue::typeName() const
{
    return _typeName;
}

const QmlJS::Document *ASTObjectValue::document() const
{
    return _doc;
}

ASTVariableReference::ASTVariableReference(VariableDeclaration *ast, ValueOwner *valueOwner)
    : Reference(valueOwner), _ast(ast)
{
}

ASTVariableReference::~ASTVariableReference()
{
}

const Value *ASTVariableReference::value(const Context *context) const
{
    Evaluate check(context);
    return check(_ast->expression);
}

ASTFunctionValue::ASTFunctionValue(FunctionExpression *ast, const QmlJS::Document *doc, ValueOwner *valueOwner)
    : FunctionValue(valueOwner), _ast(ast), _doc(doc)
{
    setPrototype(valueOwner->functionPrototype());

    for (FormalParameterList *it = ast->formals; it; it = it->next)
        _argumentNames.append(it->name);
}

ASTFunctionValue::~ASTFunctionValue()
{
}

FunctionExpression *ASTFunctionValue::ast() const
{
    return _ast;
}

const Value *ASTFunctionValue::returnValue() const
{
    return valueOwner()->undefinedValue();
}

int ASTFunctionValue::argumentCount() const
{
    return _argumentNames.size();
}

const Value *ASTFunctionValue::argument(int) const
{
    return valueOwner()->undefinedValue();
}

QString ASTFunctionValue::argumentName(int index) const
{
    if (index < _argumentNames.size()) {
        if (NameId *nameId = _argumentNames.at(index))
            return nameId->asString();
    }

    return FunctionValue::argumentName(index);
}

bool ASTFunctionValue::isVariadic() const
{
    return true;
}

bool ASTFunctionValue::getSourceLocation(QString *fileName, int *line, int *column) const
{
    *fileName = _doc->fileName();
    *line = _ast->identifierToken.startLine;
    *column = _ast->identifierToken.startColumn;
    return true;
}

QmlPrototypeReference::QmlPrototypeReference(UiQualifiedId *qmlTypeName, const QmlJS::Document *doc,
                                             ValueOwner *valueOwner)
    : Reference(valueOwner),
      _qmlTypeName(qmlTypeName),
      _doc(doc)
{
}

QmlPrototypeReference::~QmlPrototypeReference()
{
}

UiQualifiedId *QmlPrototypeReference::qmlTypeName() const
{
    return _qmlTypeName;
}

const Value *QmlPrototypeReference::value(const Context *context) const
{
    return context->lookupType(_doc, _qmlTypeName);
}

ASTPropertyReference::ASTPropertyReference(UiPublicMember *ast, const QmlJS::Document *doc, ValueOwner *valueOwner)
    : Reference(valueOwner), _ast(ast), _doc(doc)
{
    const QString propertyName = ast->name->asString();
    _onChangedSlotName = QLatin1String("on");
    _onChangedSlotName += propertyName.at(0).toUpper();
    _onChangedSlotName += propertyName.midRef(1);
    _onChangedSlotName += QLatin1String("Changed");
}

ASTPropertyReference::~ASTPropertyReference()
{
}

bool ASTPropertyReference::getSourceLocation(QString *fileName, int *line, int *column) const
{
    *fileName = _doc->fileName();
    *line = _ast->identifierToken.startLine;
    *column = _ast->identifierToken.startColumn;
    return true;
}

const Value *ASTPropertyReference::value(const Context *context) const
{
    if (_ast->statement
            && (!_ast->memberType || _ast->memberType->asString() == QLatin1String("variant")
                || _ast->memberType->asString() == QLatin1String("alias"))) {

        // Adjust the context for the current location - expensive!
        // ### Improve efficiency by caching the 'use chain' constructed in ScopeBuilder.

        QmlJS::Document::Ptr doc = _doc->ptr();
        Context localContext(*context);
        QmlJS::ScopeBuilder builder(&localContext, doc);
        builder.initializeRootScope();

        int offset = _ast->statement->firstSourceLocation().begin();
        builder.push(ScopeAstPath(doc)(offset));

        Evaluate check(&localContext);
        return check(_ast->statement);
    }

    if (_ast->memberType)
        return valueOwner()->defaultValueForBuiltinType(_ast->memberType->asString());

    return valueOwner()->undefinedValue();
}

ASTSignalReference::ASTSignalReference(UiPublicMember *ast, const QmlJS::Document *doc, ValueOwner *valueOwner)
    : Reference(valueOwner), _ast(ast), _doc(doc)
{
    const QString signalName = ast->name->asString();
    _slotName = QLatin1String("on");
    _slotName += signalName.at(0).toUpper();
    _slotName += signalName.midRef(1);
}

ASTSignalReference::~ASTSignalReference()
{
}

bool ASTSignalReference::getSourceLocation(QString *fileName, int *line, int *column) const
{
    *fileName = _doc->fileName();
    *line = _ast->identifierToken.startLine;
    *column = _ast->identifierToken.startColumn;
    return true;
}

const Value *ASTSignalReference::value(const Context *) const
{
    return valueOwner()->undefinedValue();
}

ImportInfo::ImportInfo()
    : _type(InvalidImport)
    , _ast(0)
{
}

ImportInfo::ImportInfo(Type type, const QString &name,
                       ComponentVersion version, UiImport *ast)
    : _type(type)
    , _name(name)
    , _version(version)
    , _ast(ast)
{
}

bool ImportInfo::isValid() const
{
    return _type != InvalidImport;
}

ImportInfo::Type ImportInfo::type() const
{
    return _type;
}

QString ImportInfo::name() const
{
    return _name;
}

QString ImportInfo::id() const
{
    if (_ast && _ast->importId)
        return _ast->importId->asString();
    return QString();
}

ComponentVersion ImportInfo::version() const
{
    return _version;
}

UiImport *ImportInfo::ast() const
{
    return _ast;
}

Import::Import()
    : object(0)
{}

TypeScope::TypeScope(const Imports *imports, ValueOwner *valueOwner)
    : ObjectValue(valueOwner)
    , _imports(imports)
{
}

const Value *TypeScope::lookupMember(const QString &name, const Context *context,
                                           const ObjectValue **foundInObject, bool) const
{
    QListIterator<Import> it(_imports->all());
    it.toBack();
    while (it.hasPrevious()) {
        const Import &i = it.previous();
        const ObjectValue *import = i.object;
        const ImportInfo &info = i.info;

        // JS import has no types
        if (info.type() == ImportInfo::FileImport)
            continue;

        if (!info.id().isEmpty()) {
            if (info.id() == name) {
                if (foundInObject)
                    *foundInObject = this;
                return import;
            }
            continue;
        }

        if (const Value *v = import->lookupMember(name, context, foundInObject))
            return v;
    }
    if (foundInObject)
        *foundInObject = 0;
    return 0;
}

void TypeScope::processMembers(MemberProcessor *processor) const
{
    QListIterator<Import> it(_imports->all());
    it.toBack();
    while (it.hasPrevious()) {
        const Import &i = it.previous();
        const ObjectValue *import = i.object;
        const ImportInfo &info = i.info;

        // JS import has no types
        if (info.type() == ImportInfo::FileImport)
            continue;

        if (!info.id().isEmpty()) {
            processor->processProperty(info.id(), import);
        } else {
            import->processMembers(processor);
        }
    }
}

JSImportScope::JSImportScope(const Imports *imports, ValueOwner *valueOwner)
    : ObjectValue(valueOwner)
    , _imports(imports)
{
}

const Value *JSImportScope::lookupMember(const QString &name, const Context *,
                                         const ObjectValue **foundInObject, bool) const
{
    QListIterator<Import> it(_imports->all());
    it.toBack();
    while (it.hasPrevious()) {
        const Import &i = it.previous();
        const ObjectValue *import = i.object;
        const ImportInfo &info = i.info;

        // JS imports are always: import "somefile.js" as Foo
        if (info.type() != ImportInfo::FileImport)
            continue;

        if (info.id() == name) {
            if (foundInObject)
                *foundInObject = this;
            return import;
        }
    }
    if (foundInObject)
        *foundInObject = 0;
    return 0;
}

void JSImportScope::processMembers(MemberProcessor *processor) const
{
    QListIterator<Import> it(_imports->all());
    it.toBack();
    while (it.hasPrevious()) {
        const Import &i = it.previous();
        const ObjectValue *import = i.object;
        const ImportInfo &info = i.info;

        if (info.type() == ImportInfo::FileImport)
            processor->processProperty(info.id(), import);
    }
}

Imports::Imports(ValueOwner *valueOwner)
    : _typeScope(new TypeScope(this, valueOwner))
    , _jsImportScope(new JSImportScope(this, valueOwner))
{}

void Imports::append(const Import &import)
{
    // when doing lookup, imports with 'as' clause are looked at first
    if (!import.info.id().isEmpty()) {
        _imports.append(import);
    } else {
        // find first as-import and prepend
        for (int i = 0; i < _imports.size(); ++i) {
            if (!_imports.at(i).info.id().isEmpty()) {
                _imports.insert(i, import);
                return;
            }
        }
        // not found, append
        _imports.append(import);
    }
}

ImportInfo Imports::info(const QString &name, const Context *context) const
{
    QString firstId = name;
    int dotIdx = firstId.indexOf(QLatin1Char('.'));
    if (dotIdx != -1)
        firstId = firstId.left(dotIdx);

    QListIterator<Import> it(_imports);
    it.toBack();
    while (it.hasPrevious()) {
        const Import &i = it.previous();
        const ObjectValue *import = i.object;
        const ImportInfo &info = i.info;

        if (!info.id().isEmpty()) {
            if (info.id() == firstId)
                return info;
            continue;
        }

        if (info.type() == ImportInfo::FileImport) {
            if (import->className() == firstId)
                return info;
        } else {
            if (import->lookupMember(firstId, context))
                return info;
        }
    }
    return ImportInfo();
}

QList<Import> Imports::all() const
{
    return _imports;
}

const TypeScope *Imports::typeScope() const
{
    return _typeScope;
}

const JSImportScope *Imports::jsImportScope() const
{
    return _jsImportScope;
}

#ifdef QT_DEBUG

class MemberDumper: public MemberProcessor
{
public:
    MemberDumper() {}

    virtual bool processProperty(const QString &name, const Value *)
    {
        qDebug() << "property: " << name;
        return true;
    }

    virtual bool processEnumerator(const QString &name, const Value *)
    {
        qDebug() << "enumerator: " << name;
        return true;
    }

    virtual bool processSignal(const QString &name, const Value *)
    {
        qDebug() << "signal: " << name;
        return true;
    }

    virtual bool processSlot(const QString &name, const Value *)
    {
        qDebug() << "slot: " << name;
        return true;
    }

    virtual bool processGeneratedSlot(const QString &name, const Value *)
    {
        qDebug() << "generated slot: " << name;
        return true;
    }
};

void Imports::dump() const
{
    qDebug() << "Imports contents, in search order:";
    QListIterator<Import> it(_imports);
    it.toBack();
    while (it.hasPrevious()) {
        const Import &i = it.previous();
        const ObjectValue *import = i.object;
        const ImportInfo &info = i.info;

        qDebug() << "  " << info.name() << " " << info.version().toString() << " as " << info.id() << " : " << import;
        MemberDumper dumper;
        import->processMembers(&dumper);
    }
}

#endif
