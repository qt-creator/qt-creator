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
#include "qmljsscopechain.h"
#include "qmljsscopeastpath.h"
#include "qmljstypedescriptionreader.h"
#include "qmljsvalueowner.h"
#include "qmljscontext.h"
#include "parser/qmljsast_p.h"

#include <languageutils/fakemetaobject.h>
#include <utils/qtcassert.h>

#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QMetaObject>
#include <QtCore/QMetaProperty>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QProcess>
#include <QtCore/QDebug>

#include <algorithm>

using namespace LanguageUtils;
using namespace QmlJS;
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
                               const QString &packageName, const ComponentVersion &componentVersion,
                               const ComponentVersion &importVersion, ValueOwner *valueOwner)
    : ObjectValue(valueOwner),
      _attachedType(0),
      _metaObject(metaObject),
      _moduleName(packageName),
      _componentVersion(componentVersion),
      _importVersion(importVersion)
{
    setClassName(className);
    int nEnums = metaObject->enumeratorCount();
    for (int i = 0; i < nEnums; ++i) {
        FakeMetaEnum fEnum = metaObject->enumerator(i);
        _enums[fEnum.name()] = new QmlEnumValue(this, i);
    }
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
    const CppQmlTypes &cppTypes = valueOwner()->cppQmlTypes();

    // check in the same package/version first
    const QmlObjectValue *objectValue = cppTypes.objectByQualifiedName(
                _moduleName, typeName, _importVersion);
    if (objectValue)
        return objectValue;

    // fallback to plain cpp name
    objectValue = cppTypes.objectByCppName(typeName);
    if (objectValue)
        return objectValue;

    // try qml builtin type names
    if (const Value *v = valueOwner()->defaultValueForBuiltinType(typeName)) {
        if (!v->asUndefinedValue())
            return v;
    }

    // map other C++ types
    if (typeName == QLatin1String("QByteArray")
            || typeName == QLatin1String("QString")) {
        return valueOwner()->stringValue();
    } else if (typeName == QLatin1String("QUrl")) {
        return valueOwner()->urlValue();
    } else if (typeName == QLatin1String("long")) {
        return valueOwner()->intValue();
    }  else if (typeName == QLatin1String("float")
                || typeName == QLatin1String("qreal")) {
        return valueOwner()->realValue();
    } else if (typeName == QLatin1String("QFont")) {
        return valueOwner()->qmlFontObject();
    } else if (typeName == QLatin1String("QPoint")
            || typeName == QLatin1String("QPointF")
            || typeName == QLatin1String("QVector2D")) {
        return valueOwner()->qmlPointObject();
    } else if (typeName == QLatin1String("QSize")
            || typeName == QLatin1String("QSizeF")) {
        return valueOwner()->qmlSizeObject();
    } else if (typeName == QLatin1String("QRect")
            || typeName == QLatin1String("QRectF")) {
        return valueOwner()->qmlRectObject();
    } else if (typeName == QLatin1String("QVector3D")) {
        return valueOwner()->qmlVector3DObject();
    } else if (typeName == QLatin1String("QColor")) {
        return valueOwner()->colorValue();
    } else if (typeName == QLatin1String("QDeclarativeAnchorLine")) {
        return valueOwner()->anchorLineValue();
    }

    // might be an enum
    const QmlObjectValue *base = this;
    const QStringList components = typeName.split(QLatin1String("::"));
    if (components.size() == 2) {
        base = valueOwner()->cppQmlTypes().objectByCppName(components.first());
        typeName = components.last();
    }
    if (base) {
        if (const QmlEnumValue *value = base->getEnumValue(typeName))
            return value;
    }

    return valueOwner()->undefinedValue();
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

QString QmlObjectValue::moduleName() const
{ return _moduleName; }

ComponentVersion QmlObjectValue::componentVersion() const
{ return _componentVersion; }

ComponentVersion QmlObjectValue::importVersion() const
{ return _importVersion; }

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

FakeMetaEnum QmlObjectValue::getEnum(const QString &typeName, const QmlObjectValue **foundInScope) const
{
    for (const QmlObjectValue *it = this; it; it = it->prototype()) {
        FakeMetaObject::ConstPtr iter = it->_metaObject;
        const int index = iter->enumeratorIndex(typeName);
        if (index != -1) {
            if (foundInScope)
                *foundInScope = it;
            return iter->enumerator(index);
        }
    }
    if (foundInScope)
        *foundInScope = 0;
    return FakeMetaEnum();
}

const QmlEnumValue *QmlObjectValue::getEnumValue(const QString &typeName, const QmlObjectValue **foundInScope) const
{
    for (const QmlObjectValue *it = this; it; it = it->prototype()) {
        if (const QmlEnumValue *e = it->_enums.value(typeName)) {
            if (foundInScope)
                *foundInScope = it;
            return e;
        }
    }
    if (foundInScope)
        *foundInScope = 0;
    return 0;
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

bool QmlObjectValue::isDerivedFrom(FakeMetaObject::ConstPtr base) const
{
    for (const QmlObjectValue *it = this; it; it = it->prototype()) {
        FakeMetaObject::ConstPtr iter = it->_metaObject;
        if (iter == base)
            return true;
    }
    return false;
}

QmlEnumValue::QmlEnumValue(const QmlObjectValue *owner, int enumIndex)
    : _owner(owner)
    , _enumIndex(enumIndex)
{
    owner->valueOwner()->registerValue(this);
}

QmlEnumValue::~QmlEnumValue()
{
}

QString QmlEnumValue::name() const
{
    return _owner->metaObject()->enumerator(_enumIndex).name();
}

QStringList QmlEnumValue::keys() const
{
    return _owner->metaObject()->enumerator(_enumIndex).keys();
}

const QmlObjectValue *QmlEnumValue::owner() const
{
    return _owner;
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

const Value *Reference::value(ReferenceContext *) const
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

    if (examinePrototypes && context) {
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

PrototypeIterator::PrototypeIterator(const ObjectValue *start, const ContextPtr &context)
    : m_current(0)
    , m_next(start)
    , m_context(context.data())
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
    setMember(QLatin1String("length"), valueOwner->numberValue());
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

void Function::addArgument(const Value *argument, const QString &name)
{
    if (!name.isEmpty()) {
        while (_argumentNames.size() < _arguments.size())
            _argumentNames.push_back(QString());
        _argumentNames.push_back(name);
    }
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

QString Function::argumentName(int index) const
{
    if (index < _argumentNames.size()) {
        const QString name = _argumentNames.at(index);
        if (!name.isEmpty())
            return _argumentNames.at(index);
    }
    return FunctionValue::argumentName(index);
}

const Value *Function::invoke(const Activation *) const
{
    return _returnValue;
}

////////////////////////////////////////////////////////////////////////////////
// typing environment
////////////////////////////////////////////////////////////////////////////////

CppQmlTypesLoader::BuiltinObjects CppQmlTypesLoader::defaultLibraryObjects;
CppQmlTypesLoader::BuiltinObjects CppQmlTypesLoader::defaultQtObjects;

CppQmlTypesLoader::BuiltinObjects CppQmlTypesLoader::loadQmlTypes(const QFileInfoList &qmlTypeFiles, QStringList *errors, QStringList *warnings)
{
    QHash<QString, FakeMetaObject::ConstPtr> newObjects;

    foreach (const QFileInfo &qmlTypeFile, qmlTypeFiles) {
        QString error, warning;
        QFile file(qmlTypeFile.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly)) {
            QString contents = QString::fromUtf8(file.readAll());
            file.close();

            TypeDescriptionReader reader(contents);
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

    return newObjects;
}

void CppQmlTypesLoader::parseQmlTypeDescriptions(const QByteArray &xml,
                                                 BuiltinObjects *newObjects,
                                                 QString *errorMessage,
                                                 QString *warningMessage)
{
    errorMessage->clear();
    warningMessage->clear();
    TypeDescriptionReader reader(QString::fromUtf8(xml));
    if (!reader(newObjects)) {
        if (reader.errorMessage().isEmpty()) {
            *errorMessage = QLatin1String("unknown error");
        } else {
            *errorMessage = reader.errorMessage();
        }
    }
    *warningMessage = reader.warningMessage();
}

CppQmlTypes::CppQmlTypes(ValueOwner *valueOwner)
    : _valueOwner(valueOwner)
{
}

const QLatin1String CppQmlTypes::defaultPackage("<default>");
const QLatin1String CppQmlTypes::cppPackage("<cpp>");

template <typename T>
void CppQmlTypes::load(const T &fakeMetaObjects, const QString &overridePackage)
{
    QList<QmlObjectValue *> newCppTypes;
    foreach (const FakeMetaObject::ConstPtr &fmo, fakeMetaObjects) {
        foreach (const FakeMetaObject::Export &exp, fmo->exports()) {
            QString package = exp.package;
            if (package.isEmpty())
                package = overridePackage;
            _fakeMetaObjectsByPackage[package].insert(fmo);

            // make versionless cpp types directly
            // needed for access to property types that are not exported, like QDeclarativeAnchors
            if (exp.package == cppPackage) {
                QTC_ASSERT(exp.version == ComponentVersion(), continue);
                QTC_ASSERT(exp.type == fmo->className(), continue);
                QmlObjectValue *cppValue = new QmlObjectValue(
                            fmo, fmo->className(), cppPackage, ComponentVersion(), ComponentVersion(), _valueOwner);
                _objectsByQualifiedName[exp.packageNameVersion] = cppValue;
                newCppTypes += cppValue;
            }
        }
    }

    // set prototypes of cpp types
    foreach (QmlObjectValue *object, newCppTypes) {
        const QString &protoCppName = object->metaObject()->superclassName();
        const QmlObjectValue *proto = objectByCppName(protoCppName);
        if (proto)
            object->setPrototype(proto);
    }
}
// explicitly instantiate load for list and hash
template void CppQmlTypes::load< QList<FakeMetaObject::ConstPtr> >(const QList<FakeMetaObject::ConstPtr> &, const QString &);
template void CppQmlTypes::load< QHash<QString, FakeMetaObject::ConstPtr> >(const QHash<QString, FakeMetaObject::ConstPtr> &, const QString &);

QList<const QmlObjectValue *> CppQmlTypes::createObjectsForImport(const QString &package, ComponentVersion version)
{
    QList<const QmlObjectValue *> exportedObjects;

    // make new exported objects
    foreach (const FakeMetaObject::ConstPtr &fmo, _fakeMetaObjectsByPackage.value(package)) {
        // find the highest-version export
        FakeMetaObject::Export bestExport;
        foreach (const FakeMetaObject::Export &exp, fmo->exports()) {
            if (exp.package != package || (version.isValid() && exp.version > version))
                continue;
            if (!bestExport.isValid() || exp.version > bestExport.version)
                bestExport = exp;
        }
        if (!bestExport.isValid())
            continue;

        // if it already exists, skip
        const QString key = qualifiedName(package, fmo->className(), version);
        if (_objectsByQualifiedName.contains(key))
            continue;

        QmlObjectValue *newObject = new QmlObjectValue(
                    fmo, bestExport.type, package, bestExport.version, version, _valueOwner);

        // use package.cppname importversion as key
        _objectsByQualifiedName.insert(key, newObject);
        exportedObjects += newObject;
    }

    // set their prototypes, creating them if necessary
    foreach (const QmlObjectValue *cobject, exportedObjects) {
        QmlObjectValue *object = const_cast<QmlObjectValue *>(cobject);
        while (!object->prototype()) {
            const QString &protoCppName = object->metaObject()->superclassName();
            if (protoCppName.isEmpty())
                break;

            // if the prototype already exists, done
            const QString key = qualifiedName(object->moduleName(), protoCppName, version);
            if (const QmlObjectValue *proto = _objectsByQualifiedName.value(key)) {
                object->setPrototype(proto);
                break;
            }

            // get the fmo via the cpp name
            const QmlObjectValue *cppProto = objectByCppName(protoCppName);
            if (!cppProto)
                break;
            FakeMetaObject::ConstPtr protoFmo = cppProto->metaObject();

            // make a new object
            QmlObjectValue *proto = new QmlObjectValue(
                        protoFmo, protoCppName, object->moduleName(), ComponentVersion(),
                        object->importVersion(), _valueOwner);
            _objectsByQualifiedName.insert(key, proto);
            object->setPrototype(proto);

            // maybe set prototype of prototype
            object = proto;
        }
    }

    return exportedObjects;
}

bool CppQmlTypes::hasModule(const QString &module) const
{
    return _fakeMetaObjectsByPackage.contains(module);
}

QString CppQmlTypes::qualifiedName(const QString &module, const QString &type, ComponentVersion version)
{
    return QString("%1/%2 %3").arg(
                module, type,
                version.toString());

}

const QmlObjectValue *CppQmlTypes::objectByQualifiedName(const QString &name) const
{
    return _objectsByQualifiedName.value(name);
}

const QmlObjectValue *CppQmlTypes::objectByQualifiedName(const QString &package, const QString &type,
                                                         ComponentVersion version) const
{
    return objectByQualifiedName(qualifiedName(package, type, version));
}

const QmlObjectValue *CppQmlTypes::objectByCppName(const QString &cppName) const
{
    return objectByQualifiedName(qualifiedName(cppPackage, cppName, ComponentVersion()));
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
    if (const FunctionValue *valueOfMember = value_cast<const FunctionValue *>(object->lookupMember("valueOf", ContextPtr()))) {
        _result = value_cast<const NumberValue *>(valueOfMember->call(object)); // ### invoke convert-to-number?
    }
}

void ConvertToNumber::visit(const FunctionValue *object)
{
    if (const FunctionValue *valueOfMember = value_cast<const FunctionValue *>(object->lookupMember("valueOf", ContextPtr()))) {
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
    if (const FunctionValue *toStringMember = value_cast<const FunctionValue *>(object->lookupMember("toString", ContextPtr()))) {
        _result = value_cast<const StringValue *>(toStringMember->call(object)); // ### invoke convert-to-string?
    }
}

void ConvertToString::visit(const FunctionValue *object)
{
    if (const FunctionValue *toStringMember = value_cast<const FunctionValue *>(object->lookupMember("toString", ContextPtr()))) {
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
                               const Document *doc,
                               ValueOwner *valueOwner)
    : ObjectValue(valueOwner), _typeName(typeName), _initializer(initializer), _doc(doc), _defaultPropertyRef(0)
{
    if (_initializer) {
        for (UiObjectMemberList *it = _initializer->members; it; it = it->next) {
            UiObjectMember *member = it->member;
            if (UiPublicMember *def = cast<UiPublicMember *>(member)) {
                if (def->type == UiPublicMember::Property && !def->name.isEmpty() && !def->memberType.isEmpty()) {
                    ASTPropertyReference *ref = new ASTPropertyReference(def, _doc, valueOwner);
                    _properties.append(ref);
                    if (def->defaultToken.isValid())
                        _defaultPropertyRef = ref;
                } else if (def->type == UiPublicMember::Signal && !def->name.isEmpty()) {
                    ASTSignal *ref = new ASTSignal(def, _doc, valueOwner);
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
        processor->processProperty(ref->ast()->name.toString(), ref);
        // ### Should get a different value?
        processor->processGeneratedSlot(ref->onChangedSlotName(), ref);
    }
    foreach (ASTSignal *ref, _signals) {
        processor->processSignal(ref->ast()->name.toString(), ref);
        // ### Should get a different value?
        processor->processGeneratedSlot(ref->slotName(), ref);
    }

    ObjectValue::processMembers(processor);
}

QString ASTObjectValue::defaultPropertyName() const
{
    if (_defaultPropertyRef) {
        UiPublicMember *prop = _defaultPropertyRef->ast();
        if (prop)
            return prop->name.toString();
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

const Document *ASTObjectValue::document() const
{
    return _doc;
}

ASTVariableReference::ASTVariableReference(VariableDeclaration *ast, const Document *doc, ValueOwner *valueOwner)
    : Reference(valueOwner)
    , _ast(ast)
    , _doc(doc)
{
}

ASTVariableReference::~ASTVariableReference()
{
}

const Value *ASTVariableReference::value(ReferenceContext *referenceContext) const
{
    if (!_ast->expression)
        return valueOwner()->undefinedValue();

    Document::Ptr doc = _doc->ptr();
    ScopeChain scopeChain(doc, referenceContext->context());
    ScopeBuilder builder(&scopeChain);
    builder.push(ScopeAstPath(doc)(_ast->expression->firstSourceLocation().begin()));

    Evaluate evaluator(&scopeChain, referenceContext);
    return evaluator(_ast->expression);
}

bool ASTVariableReference::getSourceLocation(QString *fileName, int *line, int *column) const
{
    *fileName = _doc->fileName();
    *line = _ast->identifierToken.startLine;
    *column = _ast->identifierToken.startColumn;
    return true;
}

ASTFunctionValue::ASTFunctionValue(FunctionExpression *ast, const Document *doc, ValueOwner *valueOwner)
    : FunctionValue(valueOwner), _ast(ast), _doc(doc)
{
    setPrototype(valueOwner->functionPrototype());

    for (FormalParameterList *it = ast->formals; it; it = it->next)
        _argumentNames.append(it->name.toString());
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
        const QString &name = _argumentNames.at(index);
        if (!name.isEmpty())
            return name;
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

QmlPrototypeReference::QmlPrototypeReference(UiQualifiedId *qmlTypeName, const Document *doc,
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

const Value *QmlPrototypeReference::value(ReferenceContext *referenceContext) const
{
    return referenceContext->context()->lookupType(_doc, _qmlTypeName);
}

ASTPropertyReference::ASTPropertyReference(UiPublicMember *ast, const Document *doc, ValueOwner *valueOwner)
    : Reference(valueOwner), _ast(ast), _doc(doc)
{
    const QString &propertyName = ast->name.toString();
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

const Value *ASTPropertyReference::value(ReferenceContext *referenceContext) const
{
    if (_ast->statement
            && (_ast->memberType.isEmpty() || _ast->memberType == QLatin1String("variant")
                || _ast->memberType == QLatin1String("alias"))) {

        // Adjust the context for the current location - expensive!
        // ### Improve efficiency by caching the 'use chain' constructed in ScopeBuilder.

        Document::Ptr doc = _doc->ptr();
        ScopeChain scopeChain(doc, referenceContext->context());
        ScopeBuilder builder(&scopeChain);

        int offset = _ast->statement->firstSourceLocation().begin();
        builder.push(ScopeAstPath(doc)(offset));

        Evaluate evaluator(&scopeChain, referenceContext);
        return evaluator(_ast->statement);
    }

    if (!_ast->memberType.isEmpty())
        return valueOwner()->defaultValueForBuiltinType(_ast->memberType.toString());

    return valueOwner()->undefinedValue();
}

ASTSignal::ASTSignal(UiPublicMember *ast, const Document *doc, ValueOwner *valueOwner)
    : FunctionValue(valueOwner), _ast(ast), _doc(doc)
{
    const QString &signalName = ast->name.toString();
    _slotName = QLatin1String("on");
    _slotName += signalName.at(0).toUpper();
    _slotName += signalName.midRef(1);
}

ASTSignal::~ASTSignal()
{
}

int ASTSignal::argumentCount() const
{
    int count = 0;
    for (UiParameterList *it = _ast->parameters; it; it = it->next)
        ++count;
    return count;
}

const Value *ASTSignal::argument(int index) const
{
    UiParameterList *param = _ast->parameters;
    for (int i = 0; param && i < index; ++i)
        param = param->next;
    if (!param || param->type.isEmpty())
        return valueOwner()->undefinedValue();
    return valueOwner()->defaultValueForBuiltinType(param->type.toString());
}

QString ASTSignal::argumentName(int index) const
{
    UiParameterList *param = _ast->parameters;
    for (int i = 0; param && i < index; ++i)
        param = param->next;
    if (!param || param->name.isEmpty())
        return FunctionValue::argumentName(index);
    return param->name.toString();
}

bool ASTSignal::getSourceLocation(QString *fileName, int *line, int *column) const
{
    *fileName = _doc->fileName();
    *line = _ast->identifierToken.startLine;
    *column = _ast->identifierToken.startColumn;
    return true;
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
    if (_ast)
        return _ast->importId.toString();
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

QString Imports::nameForImportedObject(const ObjectValue *value, const Context *context) const
{
    QListIterator<Import> it(_imports);
    it.toBack();
    while (it.hasPrevious()) {
        const Import &i = it.previous();
        const ObjectValue *import = i.object;
        const ImportInfo &info = i.info;

        if (info.type() == ImportInfo::FileImport) {
            if (import == value)
                return import->className();
        } else {
            const Value *v = import->lookupMember(value->className(), context);
            if (v == value) {
                QString result = value->className();
                if (!info.id().isEmpty()) {
                    result.prepend(QLatin1Char('.'));
                    result.prepend(info.id());
                }
                return result;
            }
        }
    }
    return QString();
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
