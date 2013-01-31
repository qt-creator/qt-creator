/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

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

#include <QFile>
#include <QDir>
#include <QString>
#include <QStringList>
#include <QMetaObject>
#include <QMetaProperty>
#include <QXmlStreamReader>
#include <QProcess>
#include <QDebug>

#include <algorithm>

using namespace LanguageUtils;
using namespace QmlJS;
using namespace QmlJS::AST;

/*!
    \class QmlJS::Value
    \brief Abstract base class for the result of a JS expression.
    \sa Evaluate ValueOwner ValueVisitor

    A Value represents a category of JavaScript values, such as number
    (NumberValue), string (StringValue) or functions with a
    specific signature (FunctionValue). It can also represent internal
    categories such as "a QML component instantiation defined in a file"
    (ASTObjectValue), "a QML component defined in C++"
    (CppComponentValue) or "no specific information is available"
    (UnknownValue).

    The Value class itself provides accept() for admitting
    \l{ValueVisitor}s and a do-nothing getSourceLocation().

    Value instances should be cast to a derived type either through the
    asXXX() helper functions such as asNumberValue() or via the
    value_cast() template function.

    Values are the result of many operations in the QmlJS code model:
    \list
    \o \l{Evaluate}
    \o Context::lookupType() and Context::lookupReference()
    \o ScopeChain::lookup()
    \o ObjectValue::lookupMember()
    \endlist
*/

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

    virtual int namedArgumentCount() const
    {
        return _method.parameterNames().size();
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
};

} // end of anonymous namespace

CppComponentValue::CppComponentValue(FakeMetaObject::ConstPtr metaObject, const QString &className,
                               const QString &packageName, const ComponentVersion &componentVersion,
                               const ComponentVersion &importVersion, int metaObjectRevision,
                               ValueOwner *valueOwner)
    : ObjectValue(valueOwner),
      _metaObject(metaObject),
      _moduleName(packageName),
      _componentVersion(componentVersion),
      _importVersion(importVersion),
      _metaObjectRevision(metaObjectRevision)
{
    setClassName(className);
    int nEnums = metaObject->enumeratorCount();
    for (int i = 0; i < nEnums; ++i) {
        FakeMetaEnum fEnum = metaObject->enumerator(i);
        _enums[fEnum.name()] = new QmlEnumValue(this, i);
    }
}

CppComponentValue::~CppComponentValue()
{
#if QT_VERSION >= 0x050000
    delete _metaSignatures.load();
    delete _signalScopes.load();
#else
    delete _metaSignatures;
    delete _signalScopes;
#endif
}

static QString generatedSlotName(const QString &base)
{
    QString slotName = QLatin1String("on");
    slotName += base.at(0).toUpper();
    slotName += base.midRef(1);
    return slotName;
}

const CppComponentValue *CppComponentValue::asCppComponentValue() const
{
    return this;
}

void CppComponentValue::processMembers(MemberProcessor *processor) const
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

    // make MetaFunction instances lazily when first needed
#if QT_VERSION >= 0x050000
    QList<const Value *> *signatures = _metaSignatures.load();
#else
    QList<const Value *> *signatures = _metaSignatures;
#endif
    if (!signatures) {
        signatures = new QList<const Value *>;
        signatures->reserve(_metaObject->methodCount());
        for (int index = 0; index < _metaObject->methodCount(); ++index)
            signatures->append(new MetaFunction(_metaObject->method(index), valueOwner()));
        if (!_metaSignatures.testAndSetOrdered(0, signatures)) {
            delete signatures;
#if QT_VERSION >= 0x050000
            signatures = _metaSignatures.load();
#else
            signatures = _metaSignatures;
#endif
        }
    }

    // process the meta methods
    for (int index = 0; index < _metaObject->methodCount(); ++index) {
        const FakeMetaMethod method = _metaObject->method(index);
        if (_metaObjectRevision < method.revision())
            continue;

        const QString &methodName = _metaObject->method(index).methodName();
        const Value *signature = signatures->at(index);

        if (method.methodType() == FakeMetaMethod::Slot && method.access() == FakeMetaMethod::Public) {
            processor->processSlot(methodName, signature);

        } else if (method.methodType() == FakeMetaMethod::Signal && method.access() != FakeMetaMethod::Private) {
            // process the signal
            processor->processSignal(methodName, signature);
            explicitSignals.insert(methodName);

            // process the generated slot
            const QString &slotName = generatedSlotName(methodName);
            processor->processGeneratedSlot(slotName, signature);
        }
    }

    // process the meta properties
    for (int index = 0; index < _metaObject->propertyCount(); ++index) {
        const FakeMetaProperty prop = _metaObject->property(index);
        if (_metaObjectRevision < prop.revision())
            continue;

        const QString propertyName = prop.name();
        processor->processProperty(propertyName, valueForCppName(prop.typeName()));

        // every property always has a onXyzChanged slot, even if the NOTIFY
        // signal has a different name
        QString signalName = propertyName;
        signalName += QLatin1String("Changed");
        if (!explicitSignals.contains(signalName)) {
            // process the generated slot
            const QString &slotName = generatedSlotName(signalName);
            processor->processGeneratedSlot(slotName, valueOwner()->unknownValue());
        }
    }

    // look into attached types
    const QString &attachedTypeName = _metaObject->attachedTypeName();
    if (!attachedTypeName.isEmpty()) {
        const CppComponentValue *attachedType = valueOwner()->cppQmlTypes().objectByCppName(attachedTypeName);
        if (attachedType && attachedType != this) // ### only weak protection against infinite loops
            attachedType->processMembers(processor);
    }

    ObjectValue::processMembers(processor);
}

const Value *CppComponentValue::valueForCppName(const QString &typeName) const
{
    const CppQmlTypes &cppTypes = valueOwner()->cppQmlTypes();

    // check in the same package/version first
    const CppComponentValue *objectValue = cppTypes.objectByQualifiedName(
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
    const CppComponentValue *base = this;
    const QStringList components = typeName.split(QLatin1String("::"));
    if (components.size() == 2)
        base = valueOwner()->cppQmlTypes().objectByCppName(components.first());
    if (base) {
        if (const QmlEnumValue *value = base->getEnumValue(components.last()))
            return value;
    }

    // may still be a cpp based value
    return valueOwner()->unknownValue();
}

const CppComponentValue *CppComponentValue::prototype() const
{
    Q_ASSERT(!_prototype || value_cast<CppComponentValue>(_prototype));
    return static_cast<const CppComponentValue *>(_prototype);
}

/*!
  \returns a list started by this object and followed by all its prototypes

  Prefer to use this over calling prototype() in a loop, as it avoids cycles.
*/
QList<const CppComponentValue *> CppComponentValue::prototypes() const
{
    QList<const CppComponentValue *> protos;
    for (const CppComponentValue *it = this; it; it = it->prototype()) {
        if (protos.contains(it))
            break;
        protos += it;
    }
    return protos;
}

FakeMetaObject::ConstPtr CppComponentValue::metaObject() const
{
    return _metaObject;
}

QString CppComponentValue::moduleName() const
{ return _moduleName; }

ComponentVersion CppComponentValue::componentVersion() const
{ return _componentVersion; }

ComponentVersion CppComponentValue::importVersion() const
{ return _importVersion; }

QString CppComponentValue::defaultPropertyName() const
{ return _metaObject->defaultPropertyName(); }

QString CppComponentValue::propertyType(const QString &propertyName) const
{
    foreach (const CppComponentValue *it, prototypes()) {
        FakeMetaObject::ConstPtr iter = it->_metaObject;
        int propIdx = iter->propertyIndex(propertyName);
        if (propIdx != -1)
            return iter->property(propIdx).typeName();
    }
    return QString();
}

bool CppComponentValue::isListProperty(const QString &propertyName) const
{
    foreach (const CppComponentValue *it, prototypes()) {
        FakeMetaObject::ConstPtr iter = it->_metaObject;
        int propIdx = iter->propertyIndex(propertyName);
        if (propIdx != -1)
            return iter->property(propIdx).isList();
    }
    return false;
}

FakeMetaEnum CppComponentValue::getEnum(const QString &typeName, const CppComponentValue **foundInScope) const
{
    foreach (const CppComponentValue *it, prototypes()) {
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

const QmlEnumValue *CppComponentValue::getEnumValue(const QString &typeName, const CppComponentValue **foundInScope) const
{
    foreach (const CppComponentValue *it, prototypes()) {
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

const ObjectValue *CppComponentValue::signalScope(const QString &signalName) const
{
#if QT_VERSION >= 0x050000
    QHash<QString, const ObjectValue *> *scopes = _signalScopes.load();
#else
    QHash<QString, const ObjectValue *> *scopes = _signalScopes;
#endif
    if (!scopes) {
        scopes = new QHash<QString, const ObjectValue *>;
        // usually not all methods are signals
        scopes->reserve(_metaObject->methodCount() / 2);
        for (int index = 0; index < _metaObject->methodCount(); ++index) {
            const FakeMetaMethod &method = _metaObject->method(index);
            if (method.methodType() != FakeMetaMethod::Signal || method.access() == FakeMetaMethod::Private)
                continue;

            const QStringList &parameterNames = method.parameterNames();
            const QStringList &parameterTypes = method.parameterTypes();
            QTC_ASSERT(parameterNames.size() == parameterTypes.size(), continue);

            ObjectValue *scope = valueOwner()->newObject(/*prototype=*/0);
            for (int i = 0; i < parameterNames.size(); ++i) {
                const QString &name = parameterNames.at(i);
                const QString &type = parameterTypes.at(i);
                if (name.isEmpty())
                    continue;
                scope->setMember(name, valueForCppName(type));
            }
            scopes->insert(generatedSlotName(method.methodName()), scope);
        }
        if (!_signalScopes.testAndSetOrdered(0, scopes)) {
            delete scopes;
#if QT_VERSION >= 0x050000
            scopes = _signalScopes.load();
#else
            scopes = _signalScopes;
#endif
        }
    }

    return scopes->value(signalName);
}

bool CppComponentValue::isWritable(const QString &propertyName) const
{
    foreach (const CppComponentValue *it, prototypes()) {
        FakeMetaObject::ConstPtr iter = it->_metaObject;
        int propIdx = iter->propertyIndex(propertyName);
        if (propIdx != -1)
            return iter->property(propIdx).isWritable();
    }
    return false;
}

bool CppComponentValue::isPointer(const QString &propertyName) const
{
    foreach (const CppComponentValue *it, prototypes()) {
        FakeMetaObject::ConstPtr iter = it->_metaObject;
        int propIdx = iter->propertyIndex(propertyName);
        if (propIdx != -1)
            return iter->property(propIdx).isPointer();
    }
    return false;
}

bool CppComponentValue::hasLocalProperty(const QString &typeName) const
{
    int idx = _metaObject->propertyIndex(typeName);
    if (idx == -1)
        return false;
    return true;
}

bool CppComponentValue::hasProperty(const QString &propertyName) const
{
    foreach (const CppComponentValue *it, prototypes()) {
        FakeMetaObject::ConstPtr iter = it->_metaObject;
        int propIdx = iter->propertyIndex(propertyName);
        if (propIdx != -1)
            return true;
    }
    return false;
}

bool CppComponentValue::isDerivedFrom(FakeMetaObject::ConstPtr base) const
{
    foreach (const CppComponentValue *it, prototypes()) {
        FakeMetaObject::ConstPtr iter = it->_metaObject;
        if (iter == base)
            return true;
    }
    return false;
}

QmlEnumValue::QmlEnumValue(const CppComponentValue *owner, int enumIndex)
    : _owner(owner)
    , _enumIndex(enumIndex)
{
    owner->valueOwner()->registerValue(this);
}

QmlEnumValue::~QmlEnumValue()
{
}

const QmlEnumValue *QmlEnumValue::asQmlEnumValue() const
{
    return this;
}

QString QmlEnumValue::name() const
{
    return _owner->metaObject()->enumerator(_enumIndex).name();
}

QStringList QmlEnumValue::keys() const
{
    return _owner->metaObject()->enumerator(_enumIndex).keys();
}

const CppComponentValue *QmlEnumValue::owner() const
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

void ValueVisitor::visit(const UnknownValue *)
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

const UnknownValue *Value::asUnknownValue() const
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

const CppComponentValue *Value::asCppComponentValue() const
{
    return 0;
}

const ASTObjectValue *Value::asAstObjectValue() const
{
    return 0;
}

const QmlEnumValue *Value::asQmlEnumValue() const
{
    return 0;
}

const QmlPrototypeReference *Value::asQmlPrototypeReference() const
{
    return 0;
}

const ASTPropertyReference *Value::asAstPropertyReference() const
{
    return 0;
}

const ASTSignal *Value::asAstSignal() const
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

void UnknownValue::accept(ValueVisitor *visitor) const
{
    visitor->visit(this);
}

const UnknownValue *UnknownValue::asUnknownValue() const
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
    const ObjectValue *prototypeObject = value_cast<ObjectValue>(_prototype);
    if (! prototypeObject) {
        if (const Reference *prototypeReference = value_cast<Reference>(_prototype))
            prototypeObject = value_cast<ObjectValue>(context->lookupReference(prototypeReference));
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

    m_next = value_cast<ObjectValue>(proto);
    if (! m_next)
        m_next = value_cast<ObjectValue>(m_context->lookupReference(proto));
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
    if (hasNext())
        return m_next;
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

FunctionValue::FunctionValue(ValueOwner *valueOwner)
    : ObjectValue(valueOwner)
{
    setClassName(QLatin1String("Function"));
    setMember(QLatin1String("length"), valueOwner->numberValue());
    setPrototype(valueOwner->functionPrototype());
}

FunctionValue::~FunctionValue()
{
}

const Value *FunctionValue::returnValue() const
{
    return valueOwner()->unknownValue();
}

int FunctionValue::namedArgumentCount() const
{
    return 0;
}

const Value *FunctionValue::argument(int) const
{
    return valueOwner()->unknownValue();
}

QString FunctionValue::argumentName(int index) const
{
    return QString::fromLatin1("arg%1").arg(index + 1);
}

int FunctionValue::optionalNamedArgumentCount() const
{
    return 0;
}

bool FunctionValue::isVariadic() const
{
    return true;
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
    : FunctionValue(valueOwner)
    , _returnValue(0)
    , _optionalNamedArgumentCount(0)
    , _isVariadic(false)
{
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

void Function::setVariadic(bool variadic)
{
    _isVariadic = variadic;
}

void Function::setOptionalNamedArgumentCount(int count)
{
    _optionalNamedArgumentCount = count;
}

int Function::namedArgumentCount() const
{
    return _arguments.size();
}

int Function::optionalNamedArgumentCount() const
{
    return _optionalNamedArgumentCount;
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

bool Function::isVariadic() const
{
    return _isVariadic;
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
            QByteArray contents = file.readAll();
            file.close();

            parseQmlTypeDescriptions(contents, &newObjects, 0, &error, &warning);
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
                                 qmlTypeFile.absoluteFilePath(), warning));
        }
    }

    return newObjects;
}

void CppQmlTypesLoader::parseQmlTypeDescriptions(const QByteArray &xml,
                                                 BuiltinObjects *newObjects,
                                                 QList<ModuleApiInfo> *newModuleApis,
                                                 QString *errorMessage,
                                                 QString *warningMessage)
{
    errorMessage->clear();
    warningMessage->clear();
    TypeDescriptionReader reader(QString::fromUtf8(xml));
    if (!reader(newObjects, newModuleApis)) {
        if (reader.errorMessage().isEmpty())
            *errorMessage = QLatin1String("unknown error");
        else
            *errorMessage = reader.errorMessage();
    }
    *warningMessage = reader.warningMessage();
}

CppQmlTypes::CppQmlTypes(ValueOwner *valueOwner)
    : _cppContextProperties(0)
    , _valueOwner(valueOwner)

{
}

const QLatin1String CppQmlTypes::defaultPackage("<default>");
const QLatin1String CppQmlTypes::cppPackage("<cpp>");

template <typename T>
void CppQmlTypes::load(const T &fakeMetaObjects, const QString &overridePackage)
{
    QList<CppComponentValue *> newCppTypes;
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
                CppComponentValue *cppValue = new CppComponentValue(
                            fmo, fmo->className(), cppPackage, ComponentVersion(), ComponentVersion(),
                            ComponentVersion::MaxVersion, _valueOwner);
                _objectsByQualifiedName[qualifiedName(cppPackage, fmo->className(), ComponentVersion())] = cppValue;
                newCppTypes += cppValue;
            }
        }
    }

    // set prototypes of cpp types
    foreach (CppComponentValue *object, newCppTypes) {
        const QString &protoCppName = object->metaObject()->superclassName();
        const CppComponentValue *proto = objectByCppName(protoCppName);
        if (proto)
            object->setPrototype(proto);
    }
}
// explicitly instantiate load for list and hash
template void CppQmlTypes::load< QList<FakeMetaObject::ConstPtr> >(const QList<FakeMetaObject::ConstPtr> &, const QString &);
template void CppQmlTypes::load< QHash<QString, FakeMetaObject::ConstPtr> >(const QHash<QString, FakeMetaObject::ConstPtr> &, const QString &);

QList<const CppComponentValue *> CppQmlTypes::createObjectsForImport(const QString &package, ComponentVersion version)
{
    QHash<QString, const CppComponentValue *> exportedObjects;

    QList<const CppComponentValue *> newObjects;

    // make new exported objects
    foreach (const FakeMetaObject::ConstPtr &fmo, _fakeMetaObjectsByPackage.value(package)) {
        // find the highest-version export for each alias
        QHash<QString, FakeMetaObject::Export> bestExports;
        foreach (const FakeMetaObject::Export &exp, fmo->exports()) {
            if (exp.package != package || (version.isValid() && exp.version > version))
                continue;

            if (bestExports.contains(exp.type)) {
                if (exp.version > bestExports.value(exp.type).version)
                    bestExports.insert(exp.type, exp);
            } else {
                bestExports.insert(exp.type, exp);
            }
        }
        if (bestExports.isEmpty())
            continue;

        // if it already exists, skip
        const QString key = qualifiedName(package, fmo->className(), version);
        if (_objectsByQualifiedName.contains(key))
            continue;

        foreach (const FakeMetaObject::Export &bestExport, bestExports) {
            QString name = bestExport.type;
            bool exported = true;
            if (name.isEmpty()) {
                exported = false;
                name = fmo->className();
            }

            CppComponentValue *newComponent = new CppComponentValue(
                        fmo, name, package, bestExport.version, version,
                        bestExport.metaObjectRevision, _valueOwner);

            // use package.cppname importversion as key
            _objectsByQualifiedName.insert(key, newComponent);
            if (exported) {
                if (!exportedObjects.contains(name) // we might have the same type in different versions
                        || (newComponent->componentVersion() > exportedObjects.value(name)->componentVersion()))
                    exportedObjects.insert(name, newComponent);
            }
            newObjects += newComponent;
        }
    }

    // set their prototypes, creating them if necessary
    foreach (const CppComponentValue *cobject, newObjects) {
        CppComponentValue *object = const_cast<CppComponentValue *>(cobject);
        while (!object->prototype()) {
            const QString &protoCppName = object->metaObject()->superclassName();
            if (protoCppName.isEmpty())
                break;

            // if the prototype already exists, done
            const QString key = qualifiedName(object->moduleName(), protoCppName, version);
            if (const CppComponentValue *proto = _objectsByQualifiedName.value(key)) {
                object->setPrototype(proto);
                break;
            }

            // get the fmo via the cpp name
            const CppComponentValue *cppProto = objectByCppName(protoCppName);
            if (!cppProto)
                break;
            FakeMetaObject::ConstPtr protoFmo = cppProto->metaObject();

            // make a new object
            CppComponentValue *proto = new CppComponentValue(
                        protoFmo, protoCppName, object->moduleName(), ComponentVersion(),
                        object->importVersion(), ComponentVersion::MaxVersion, _valueOwner);
            _objectsByQualifiedName.insert(key, proto);
            object->setPrototype(proto);

            // maybe set prototype of prototype
            object = proto;
        }
    }

    return exportedObjects.values();
}

bool CppQmlTypes::hasModule(const QString &module) const
{
    return _fakeMetaObjectsByPackage.contains(module);
}

QString CppQmlTypes::qualifiedName(const QString &module, const QString &type, ComponentVersion version)
{
    return QString::fromLatin1("%1/%2 %3").arg(
                module, type,
                version.toString());

}

const CppComponentValue *CppQmlTypes::objectByQualifiedName(const QString &name) const
{
    return _objectsByQualifiedName.value(name);
}

const CppComponentValue *CppQmlTypes::objectByQualifiedName(const QString &package, const QString &type,
                                                         ComponentVersion version) const
{
    return objectByQualifiedName(qualifiedName(package, type, version));
}

const CppComponentValue *CppQmlTypes::objectByCppName(const QString &cppName) const
{
    return objectByQualifiedName(qualifiedName(cppPackage, cppName, ComponentVersion()));
}

void CppQmlTypes::setCppContextProperties(const ObjectValue *contextProperties)
{
    _cppContextProperties = contextProperties;
}

const ObjectValue *CppQmlTypes::cppContextProperties() const
{
    return _cppContextProperties;
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
    if (const FunctionValue *valueOfMember = value_cast<FunctionValue>(
                object->lookupMember(QLatin1String("valueOf"), ContextPtr()))) {
        _result = value_cast<NumberValue>(valueOfMember->returnValue());
    }
}

void ConvertToNumber::visit(const FunctionValue *object)
{
    if (const FunctionValue *valueOfMember = value_cast<FunctionValue>(
                object->lookupMember(QLatin1String("valueOf"), ContextPtr()))) {
        _result = value_cast<NumberValue>(valueOfMember->returnValue());
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
    if (const FunctionValue *toStringMember = value_cast<FunctionValue>(
                object->lookupMember(QLatin1String("toString"), ContextPtr()))) {
        _result = value_cast<StringValue>(toStringMember->returnValue());
    }
}

void ConvertToString::visit(const FunctionValue *object)
{
    if (const FunctionValue *toStringMember = value_cast<FunctionValue>(
                object->lookupMember(QLatin1String("toString"), ContextPtr()))) {
        _result = value_cast<StringValue>(toStringMember->returnValue());
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

void ConvertToObject::visit(const NumberValue *)
{
    _result = _valueOwner->numberCtor()->returnValue();
}

void ConvertToObject::visit(const BooleanValue *)
{
    _result = _valueOwner->booleanCtor()->returnValue();
}

void ConvertToObject::visit(const StringValue *)
{
    _result = _valueOwner->stringCtor()->returnValue();
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

const ASTObjectValue *ASTObjectValue::asAstObjectValue() const
{
    return this;
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
    // may be assigned to later
    if (!_ast->expression)
        return valueOwner()->unknownValue();

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

namespace {
class UsesArgumentsArray : protected Visitor
{
    bool _usesArgumentsArray;

public:
    bool operator()(FunctionBody *ast)
    {
        if (!ast || !ast->elements)
            return false;
        _usesArgumentsArray = false;
        Node::accept(ast->elements, this);
        return _usesArgumentsArray;
    }

protected:
    bool visit(ArrayMemberExpression *ast)
    {
        if (IdentifierExpression *idExp = cast<IdentifierExpression *>(ast->base)) {
            if (idExp->name == QLatin1String("arguments"))
                _usesArgumentsArray = true;
        }
        return true;
    }

    // don't go into nested functions
    bool visit(FunctionBody *) { return false; }
};
} // anonymous namespace

ASTFunctionValue::ASTFunctionValue(FunctionExpression *ast, const Document *doc, ValueOwner *valueOwner)
    : FunctionValue(valueOwner)
    , _ast(ast)
    , _doc(doc)
{
    setPrototype(valueOwner->functionPrototype());

    for (FormalParameterList *it = ast->formals; it; it = it->next)
        _argumentNames.append(it->name.toString());

    _isVariadic = UsesArgumentsArray()(ast->body);
}

ASTFunctionValue::~ASTFunctionValue()
{
}

FunctionExpression *ASTFunctionValue::ast() const
{
    return _ast;
}

int ASTFunctionValue::namedArgumentCount() const
{
    return _argumentNames.size();
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
    return _isVariadic;
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

const QmlPrototypeReference *QmlPrototypeReference::asQmlPrototypeReference() const
{
    return this;
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
    _onChangedSlotName = generatedSlotName(propertyName);
    _onChangedSlotName += QLatin1String("Changed");
}

ASTPropertyReference::~ASTPropertyReference()
{
}

const ASTPropertyReference *ASTPropertyReference::asAstPropertyReference() const
{
    return this;
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
            && (_ast->memberType.isEmpty()
                || _ast->memberType == QLatin1String("variant")
                || _ast->memberType == QLatin1String("var")
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

    const QString memberType = _ast->memberType.toString();

    const Value *builtin = valueOwner()->defaultValueForBuiltinType(memberType);
    if (!builtin->asUndefinedValue())
        return builtin;

    if (_ast->typeModifier.isEmpty()) {
        const Value *type = referenceContext->context()->lookupType(_doc, QStringList(memberType));
        if (type)
            return type;
    }

    return referenceContext->context()->valueOwner()->undefinedValue();
}

ASTSignal::ASTSignal(UiPublicMember *ast, const Document *doc, ValueOwner *valueOwner)
    : FunctionValue(valueOwner), _ast(ast), _doc(doc)
{
    const QString &signalName = ast->name.toString();
    _slotName = generatedSlotName(signalName);

    ObjectValue *v = valueOwner->newObject(/*prototype=*/0);
    for (UiParameterList *it = ast->parameters; it; it = it->next) {
        if (!it->name.isEmpty())
            v->setMember(it->name.toString(), valueOwner->defaultValueForBuiltinType(it->type.toString()));
    }
    _bodyScope = v;
}

ASTSignal::~ASTSignal()
{
}

const ASTSignal *ASTSignal::asAstSignal() const
{
    return this;
}

int ASTSignal::namedArgumentCount() const
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
        return valueOwner()->unknownValue();
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

ImportInfo ImportInfo::moduleImport(QString uri, ComponentVersion version,
                                    const QString &as, UiImport *ast)
{
    // treat Qt 4.7 as QtQuick 1.0
    if (uri == QLatin1String("Qt") && version == ComponentVersion(4, 7)) {
        uri = QLatin1String("QtQuick");
        version = ComponentVersion(1, 0);
    }

    ImportInfo info;
    info._type = LibraryImport;
    info._name = uri;
    info._path = uri;
    info._path.replace(QLatin1Char('.'), QDir::separator());
    info._version = version;
    info._as = as;
    info._ast = ast;
    return info;
}

ImportInfo ImportInfo::pathImport(const QString &docPath, const QString &path,
                                  ComponentVersion version, const QString &as, UiImport *ast)
{
    ImportInfo info;
    info._name = path;

    QFileInfo importFileInfo(path);
    if (!importFileInfo.isAbsolute())
        importFileInfo = QFileInfo(docPath + QDir::separator() + path);
    info._path = importFileInfo.absoluteFilePath();

    if (importFileInfo.isFile())
        info._type = FileImport;
    else if (importFileInfo.isDir())
        info._type = DirectoryImport;
    else
        info._type = UnknownFileImport;

    info._version = version;
    info._as = as;
    info._ast = ast;
    return info;
}

ImportInfo ImportInfo::invalidImport(UiImport *ast)
{
    ImportInfo info;
    info._type = InvalidImport;
    info._ast = ast;
    return info;
}

ImportInfo ImportInfo::implicitDirectoryImport(const QString &directory)
{
    ImportInfo info;
    info._type = ImplicitDirectoryImport;
    info._path = directory;
    return info;
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

QString ImportInfo::path() const
{
    return _path;
}

QString ImportInfo::as() const
{
    return _as;
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

        if (!info.as().isEmpty()) {
            if (info.as() == name) {
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

        if (!info.as().isEmpty())
            processor->processProperty(info.as(), import);
        else
            import->processMembers(processor);
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

        if (info.as() == name) {
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
            processor->processProperty(info.as(), import);
    }
}

Imports::Imports(ValueOwner *valueOwner)
    : _typeScope(new TypeScope(this, valueOwner))
    , _jsImportScope(new JSImportScope(this, valueOwner))
    , _importFailed(false)
{}

void Imports::append(const Import &import)
{
    // when doing lookup, imports with 'as' clause are looked at first
    if (!import.info.as().isEmpty()) {
        _imports.append(import);
    } else {
        // find first as-import and prepend
        for (int i = 0; i < _imports.size(); ++i) {
            if (!_imports.at(i).info.as().isEmpty()) {
                _imports.insert(i, import);
                return;
            }
        }
        // not found, append
        _imports.append(import);
    }

    if (!import.valid)
        _importFailed = true;
}

void Imports::setImportFailed()
{
    _importFailed = true;
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

        if (!info.as().isEmpty()) {
            if (info.as() == firstId)
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
                if (!info.as().isEmpty()) {
                    result.prepend(QLatin1Char('.'));
                    result.prepend(info.as());
                }
                return result;
            }
        }
    }
    return QString();
}

bool Imports::importFailed() const
{
    return _importFailed;
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

        qDebug() << "  " << info.path() << " " << info.version().toString() << " as " << info.as() << " : " << import;
        MemberDumper dumper;
        import->processMembers(&dumper);
    }
}

#endif
