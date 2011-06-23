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
    MetaFunction(const FakeMetaMethod &method, Engine *engine)
        : FunctionValue(engine), _method(method)
    {
    }

    virtual const Value *returnValue() const
    {
        return engine()->undefinedValue();
    }

    virtual int argumentCount() const
    {
        return _method.parameterNames().size();
    }

    virtual const Value *argument(int) const
    {
        return engine()->undefinedValue();
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
        return engine()->undefinedValue();
    }
};

} // end of anonymous namespace

QmlObjectValue::QmlObjectValue(FakeMetaObject::ConstPtr metaObject, const QString &className,
                               const QString &packageName, const ComponentVersion version, Engine *engine)
    : ObjectValue(engine),
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
        value = new MetaFunction(method, engine());
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
            processor->processEnumerator(e.key(i), engine()->numberValue());
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
            processor->processGeneratedSlot(slotName, engine()->undefinedValue());
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
    QmlObjectValue *objectValue = engine()->cppQmlTypes().typeByCppName(typeName);
    if (objectValue) {
        FakeMetaObject::Export exp = objectValue->metaObject()->exportInPackage(packageName());
        if (exp.isValid())
            objectValue = engine()->cppQmlTypes().typeByQualifiedName(exp.packageNameVersion);
        return objectValue;
    }

    const Value *value = engine()->undefinedValue();
    if (typeName == QLatin1String("QByteArray")
            || typeName == QLatin1String("string")
            || typeName == QLatin1String("QString")) {
        value = engine()->stringValue();
    } else if (typeName == QLatin1String("QUrl")) {
        value = engine()->urlValue();
    } else if (typeName == QLatin1String("bool")) {
        value = engine()->booleanValue();
    } else if (typeName == QLatin1String("int")
               || typeName == QLatin1String("long")) {
        value = engine()->intValue();
    }  else if (typeName == QLatin1String("float")
                || typeName == QLatin1String("double")
                || typeName == QLatin1String("qreal")) {
        // ### Review: more types here?
        value = engine()->realValue();
    } else if (typeName == QLatin1String("QFont")) {
        value = engine()->qmlFontObject();
    } else if (typeName == QLatin1String("QPoint")
            || typeName == QLatin1String("QPointF")
            || typeName == QLatin1String("QVector2D")) {
        value = engine()->qmlPointObject();
    } else if (typeName == QLatin1String("QSize")
            || typeName == QLatin1String("QSizeF")) {
        value = engine()->qmlSizeObject();
    } else if (typeName == QLatin1String("QRect")
            || typeName == QLatin1String("QRectF")) {
        value = engine()->qmlRectObject();
    } else if (typeName == QLatin1String("QVector3D")) {
        value = engine()->qmlVector3DObject();
    } else if (typeName == QLatin1String("QColor")) {
        value = engine()->colorValue();
    } else if (typeName == QLatin1String("QDeclarativeAnchorLine")) {
        value = engine()->anchorLineValue();
    }

    // might be an enum
    const QmlObjectValue *base = this;
    const QStringList components = typeName.split(QLatin1String("::"));
    if (components.size() == 2) {
        base = engine()->cppQmlTypes().typeByCppName(components.first());
        typeName = components.last();
    }
    if (base) {
        const FakeMetaEnum &metaEnum = base->getEnum(typeName);
        if (metaEnum.isValid())
            value = new QmlEnumValue(metaEnum, engine());
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
    QHashIterator<QString, QmlObjectValue *> it(engine()->cppQmlTypes().types());
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

QmlEnumValue::QmlEnumValue(const FakeMetaEnum &metaEnum, Engine *engine)
    : NumberValue(),
      _metaEnum(new FakeMetaEnum(metaEnum))
{
    engine->registerValue(this);
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

namespace {

////////////////////////////////////////////////////////////////////////////////
// constructors
////////////////////////////////////////////////////////////////////////////////
class ObjectCtor: public Function
{
public:
    ObjectCtor(Engine *engine);

    virtual const Value *invoke(const Activation *activation) const;
};

class FunctionCtor: public Function
{
public:
    FunctionCtor(Engine *engine);

    virtual const Value *invoke(const Activation *activation) const;
};

class ArrayCtor: public Function
{
public:
    ArrayCtor(Engine *engine);

    virtual const Value *invoke(const Activation *activation) const;
};

class StringCtor: public Function
{
public:
    StringCtor(Engine *engine);

    virtual const Value *invoke(const Activation *activation) const;
};

class BooleanCtor: public Function
{
public:
    BooleanCtor(Engine *engine);

    virtual const Value *invoke(const Activation *activation) const;
};

class NumberCtor: public Function
{
public:
    NumberCtor(Engine *engine);

    virtual const Value *invoke(const Activation *activation) const;
};

class DateCtor: public Function
{
public:
    DateCtor(Engine *engine);

    virtual const Value *invoke(const Activation *activation) const;
};

class RegExpCtor: public Function
{
public:
    RegExpCtor(Engine *engine);

    virtual const Value *invoke(const Activation *activation) const;
};

ObjectCtor::ObjectCtor(Engine *engine)
    : Function(engine)
{
}

FunctionCtor::FunctionCtor(Engine *engine)
    : Function(engine)
{
}

ArrayCtor::ArrayCtor(Engine *engine)
    : Function(engine)
{
}

StringCtor::StringCtor(Engine *engine)
    : Function(engine)
{
}

BooleanCtor::BooleanCtor(Engine *engine)
    : Function(engine)
{
}

NumberCtor::NumberCtor(Engine *engine)
    : Function(engine)
{
}

DateCtor::DateCtor(Engine *engine)
    : Function(engine)
{
}

RegExpCtor::RegExpCtor(Engine *engine)
    : Function(engine)
{
}

const Value *ObjectCtor::invoke(const Activation *activation) const
{
    ObjectValue *thisObject = activation->thisObject();
    if (activation->calledAsFunction())
        thisObject = engine()->newObject();

    thisObject->setClassName("Object");
    thisObject->setPrototype(engine()->objectPrototype());
    thisObject->setMember("length", engine()->numberValue());
    return thisObject;
}

const Value *FunctionCtor::invoke(const Activation *activation) const
{
    ObjectValue *thisObject = activation->thisObject();
    if (activation->calledAsFunction())
        thisObject = engine()->newObject();

    thisObject->setClassName("Function");
    thisObject->setPrototype(engine()->functionPrototype());
    thisObject->setMember("length", engine()->numberValue());
    return thisObject;
}

const Value *ArrayCtor::invoke(const Activation *activation) const
{
    ObjectValue *thisObject = activation->thisObject();
    if (activation->calledAsFunction())
        thisObject = engine()->newObject();

    thisObject->setClassName("Array");
    thisObject->setPrototype(engine()->arrayPrototype());
    thisObject->setMember("length", engine()->numberValue());
    return thisObject;
}

const Value *StringCtor::invoke(const Activation *activation) const
{
    if (activation->calledAsFunction())
        return engine()->convertToString(activation->thisObject());

    ObjectValue *thisObject = activation->thisObject();
    thisObject->setClassName("String");
    thisObject->setPrototype(engine()->stringPrototype());
    thisObject->setMember("length", engine()->numberValue());
    return thisObject;
}

const Value *BooleanCtor::invoke(const Activation *activation) const
{
    if (activation->calledAsFunction())
        return engine()->convertToBoolean(activation->thisObject());

    ObjectValue *thisObject = activation->thisObject();
    thisObject->setClassName("Boolean");
    thisObject->setPrototype(engine()->booleanPrototype());
    return thisObject;
}

const Value *NumberCtor::invoke(const Activation *activation) const
{
    if (activation->calledAsFunction())
        return engine()->convertToNumber(activation->thisObject());

    ObjectValue *thisObject = activation->thisObject();
    thisObject->setClassName("Number");
    thisObject->setPrototype(engine()->numberPrototype());
    return thisObject;
}

const Value *DateCtor::invoke(const Activation *activation) const
{
    if (activation->calledAsFunction())
        return engine()->stringValue();

    ObjectValue *thisObject = activation->thisObject();
    thisObject->setClassName("Date");
    thisObject->setPrototype(engine()->datePrototype());
    return thisObject;
}

const Value *RegExpCtor::invoke(const Activation *activation) const
{
    ObjectValue *thisObject = activation->thisObject();
    if (activation->calledAsFunction())
        thisObject = engine()->newObject();

    thisObject->setClassName("RegExp");
    thisObject->setPrototype(engine()->regexpPrototype());
    thisObject->setMember("source", engine()->stringValue());
    thisObject->setMember("global", engine()->booleanValue());
    thisObject->setMember("ignoreCase", engine()->booleanValue());
    thisObject->setMember("multiline", engine()->booleanValue());
    thisObject->setMember("lastIndex", engine()->numberValue());
    return thisObject;
}

} // end of anonymous namespace

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
      _engine(new Engine),
      _qmlScopeObjectIndex(-1),
      _qmlScopeObjectSet(false)
{
}

Context::~Context()
{
}

// the engine is only guaranteed to live as long as the context
Engine *Context::engine() const
{
    return _engine.data();
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
    return _engine->undefinedValue();
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

const Value *Context::property(const ObjectValue *object, const QString &name) const
{
    const Properties properties = _properties.value(object);
    return properties.value(name, engine()->undefinedValue());
}

void Context::setProperty(const ObjectValue *object, const QString &name, const Value *value)
{
    _properties[object].insert(name, value);
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

Reference::Reference(Engine *engine)
    : _engine(engine)
{
    _engine->registerValue(this);
}

Reference::~Reference()
{
}

Engine *Reference::engine() const
{
    return _engine;
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
    return _engine->undefinedValue();
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

ObjectValue::ObjectValue(Engine *engine)
    : _engine(engine),
      _prototype(0)
{
    engine->registerValue(this);
}

ObjectValue::~ObjectValue()
{
}

Engine *ObjectValue::engine() const
{
    return _engine;
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

FunctionValue::FunctionValue(Engine *engine)
    : ObjectValue(engine)
{
}

FunctionValue::~FunctionValue()
{
}

const Value *FunctionValue::construct(const ValueList &actuals) const
{
    Activation activation;
    activation.setCalledAsConstructor(true);
    activation.setThisObject(engine()->newObject());
    activation.setArguments(actuals);
    return invoke(&activation);
}

const Value *FunctionValue::call(const ValueList &actuals) const
{
    Activation activation;
    activation.setCalledAsFunction(true);
    activation.setThisObject(engine()->globalObject()); // ### FIXME: it should be `null'
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
    return engine()->undefinedValue();
}

int FunctionValue::argumentCount() const
{
    return 0;
}

const Value *FunctionValue::argument(int) const
{
    return engine()->undefinedValue();
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

Function::Function(Engine *engine)
    : FunctionValue(engine), _returnValue(0)
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
        return engine()->numberValue();
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
QList<QmlObjectValue *> CppQmlTypes::load(Engine *engine, const T &objects)
{
    // load
    QList<QmlObjectValue *> loadedObjects;
    QList<QmlObjectValue *> newObjects;
    foreach (FakeMetaObject::ConstPtr metaObject, objects) {
        foreach (const FakeMetaObject::Export &exp, metaObject->exports()) {
            bool wasCreated;
            QmlObjectValue *loadedObject = getOrCreate(engine, metaObject, exp, &wasCreated);
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
template QList<QmlObjectValue *> CppQmlTypes::load< QList<FakeMetaObject::ConstPtr> >(Engine *, const QList<FakeMetaObject::ConstPtr> &);
template QList<QmlObjectValue *> CppQmlTypes::load< QHash<QString, FakeMetaObject::ConstPtr> >(Engine *, const QHash<QString, FakeMetaObject::ConstPtr> &);

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
    Engine *engine,
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
                metaObject, exp.type, exp.package, exp.version, engine);
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
        object = getOrCreate(cppObject->engine(), metaObject, exp);
    } else {
        // make a convenience object that does not get added to _typesByPackage
        const QString qname = qualifiedName(package, cppName, ComponentVersion());
        object = typeByQualifiedName(qname);
        if (!object) {
            object = new QmlObjectValue(
                        metaObject, cppName, package, ComponentVersion(), cppObject->engine());
            _typesByFullyQualifiedName[qname] = object;
        }
    }

    return object;
}

ConvertToNumber::ConvertToNumber(Engine *engine)
    : _engine(engine), _result(0)
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
    _result = _engine->numberValue();
}

void ConvertToNumber::visit(const UndefinedValue *)
{
    _result = _engine->numberValue();
}

void ConvertToNumber::visit(const NumberValue *value)
{
    _result = value;
}

void ConvertToNumber::visit(const BooleanValue *)
{
    _result = _engine->numberValue();
}

void ConvertToNumber::visit(const StringValue *)
{
    _result = _engine->numberValue();
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

ConvertToString::ConvertToString(Engine *engine)
    : _engine(engine), _result(0)
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
    _result = _engine->stringValue();
}

void ConvertToString::visit(const UndefinedValue *)
{
    _result = _engine->stringValue();
}

void ConvertToString::visit(const NumberValue *)
{
    _result = _engine->stringValue();
}

void ConvertToString::visit(const BooleanValue *)
{
    _result = _engine->stringValue();
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

ConvertToObject::ConvertToObject(Engine *engine)
    : _engine(engine), _result(0)
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
    _result = _engine->nullValue();
}

void ConvertToObject::visit(const NumberValue *value)
{
    ValueList actuals;
    actuals.append(value);
    _result = _engine->numberCtor()->construct(actuals);
}

void ConvertToObject::visit(const BooleanValue *value)
{
    ValueList actuals;
    actuals.append(value);
    _result = _engine->booleanCtor()->construct(actuals);
}

void ConvertToObject::visit(const StringValue *value)
{
    ValueList actuals;
    actuals.append(value);
    _result = _engine->stringCtor()->construct(actuals);
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

Engine::Engine()
    : _objectPrototype(0),
      _functionPrototype(0),
      _numberPrototype(0),
      _booleanPrototype(0),
      _stringPrototype(0),
      _arrayPrototype(0),
      _datePrototype(0),
      _regexpPrototype(0),
      _objectCtor(0),
      _functionCtor(0),
      _arrayCtor(0),
      _stringCtor(0),
      _booleanCtor(0),
      _numberCtor(0),
      _dateCtor(0),
      _regexpCtor(0),
      _globalObject(0),
      _mathObject(0),
      _qtObject(0),
      _qmlKeysObject(0),
      _qmlFontObject(0),
      _qmlPointObject(0),
      _qmlSizeObject(0),
      _qmlRectObject(0),
      _qmlVector3DObject(0),
      _convertToNumber(this),
      _convertToString(this),
      _convertToObject(this)
{
    initializePrototypes();

    _cppQmlTypes.load(this, CppQmlTypesLoader::builtinObjects);

    // the 'Qt' object is dumped even though it is not exported
    // it contains useful information, in particular on enums - add the
    // object as a prototype to our custom Qt object to offer these for completion
    _qtObject->setPrototype(_cppQmlTypes.typeByCppName(QLatin1String("Qt")));
}

Engine::~Engine()
{
    qDeleteAll(_registeredValues);
}

const NullValue *Engine::nullValue() const
{
    return &_nullValue;
}

const UndefinedValue *Engine::undefinedValue() const
{
    return &_undefinedValue;
}

const NumberValue *Engine::numberValue() const
{
    return &_numberValue;
}

const RealValue *Engine::realValue() const
{
    return &_realValue;
}

const IntValue *Engine::intValue() const
{
    return &_intValue;
}

const BooleanValue *Engine::booleanValue() const
{
    return &_booleanValue;
}

const StringValue *Engine::stringValue() const
{
    return &_stringValue;
}

const UrlValue *Engine::urlValue() const
{
    return &_urlValue;
}

const ColorValue *Engine::colorValue() const
{
    return &_colorValue;
}

const AnchorLineValue *Engine::anchorLineValue() const
{
    return &_anchorLineValue;
}

const Value *Engine::newArray()
{
    return arrayCtor()->construct();
}

ObjectValue *Engine::newObject()
{
    return newObject(_objectPrototype);
}

ObjectValue *Engine::newObject(const ObjectValue *prototype)
{
    ObjectValue *object = new ObjectValue(this);
    object->setPrototype(prototype);
    return object;
}

Function *Engine::newFunction()
{
    Function *function = new Function(this);
    function->setPrototype(functionPrototype());
    return function;
}

ObjectValue *Engine::globalObject() const
{
    return _globalObject;
}

ObjectValue *Engine::objectPrototype() const
{
    return _objectPrototype;
}

ObjectValue *Engine::functionPrototype() const
{
    return _functionPrototype;
}

ObjectValue *Engine::numberPrototype() const
{
    return _numberPrototype;
}

ObjectValue *Engine::booleanPrototype() const
{
    return _booleanPrototype;
}

ObjectValue *Engine::stringPrototype() const
{
    return _stringPrototype;
}

ObjectValue *Engine::arrayPrototype() const
{
    return _arrayPrototype;
}

ObjectValue *Engine::datePrototype() const
{
    return _datePrototype;
}

ObjectValue *Engine::regexpPrototype() const
{
    return _regexpPrototype;
}

const FunctionValue *Engine::objectCtor() const
{
    return _objectCtor;
}

const FunctionValue *Engine::functionCtor() const
{
    return _functionCtor;
}

const FunctionValue *Engine::arrayCtor() const
{
    return _arrayCtor;
}

const FunctionValue *Engine::stringCtor() const
{
    return _stringCtor;
}

const FunctionValue *Engine::booleanCtor() const
{
    return _booleanCtor;
}

const FunctionValue *Engine::numberCtor() const
{
    return _numberCtor;
}

const FunctionValue *Engine::dateCtor() const
{
    return _dateCtor;
}

const FunctionValue *Engine::regexpCtor() const
{
    return _regexpCtor;
}

const ObjectValue *Engine::mathObject() const
{
    return _mathObject;
}

const ObjectValue *Engine::qtObject() const
{
    return _qtObject;
}

void Engine::registerValue(Value *value)
{
    QMutexLocker locker(&_mutex);
    _registeredValues.append(value);
}

const Value *Engine::convertToBoolean(const Value *value)
{
    return _convertToNumber(value);  // ### implement convert to bool
}

const Value *Engine::convertToNumber(const Value *value)
{
    return _convertToNumber(value);
}

const Value *Engine::convertToString(const Value *value)
{
    return _convertToString(value);
}

const Value *Engine::convertToObject(const Value *value)
{
    return _convertToObject(value);
}

QString Engine::typeId(const Value *value)
{
    return _typeId(value);
}

void Engine::addFunction(ObjectValue *object, const QString &name, const Value *result, int argumentCount)
{
    Function *function = newFunction();
    function->setReturnValue(result);
    for (int i = 0; i < argumentCount; ++i)
        function->addArgument(undefinedValue()); // ### introduce unknownValue
    object->setMember(name, function);
}

void Engine::addFunction(ObjectValue *object, const QString &name, int argumentCount)
{
    Function *function = newFunction();
    for (int i = 0; i < argumentCount; ++i)
        function->addArgument(undefinedValue()); // ### introduce unknownValue
    object->setMember(name, function);
}

void Engine::initializePrototypes()
{
    _objectPrototype   = newObject(/*prototype = */ 0);
    _functionPrototype = newObject(_objectPrototype);
    _numberPrototype   = newObject(_objectPrototype);
    _booleanPrototype  = newObject(_objectPrototype);
    _stringPrototype   = newObject(_objectPrototype);
    _arrayPrototype    = newObject(_objectPrototype);
    _datePrototype     = newObject(_objectPrototype);
    _regexpPrototype   = newObject(_objectPrototype);

    // set up the Global object
    _globalObject = newObject();
    _globalObject->setClassName("Global");

    // set up the default Object prototype
    _objectCtor = new ObjectCtor(this);
    _objectCtor->setPrototype(_functionPrototype);
    _objectCtor->setMember("prototype", _objectPrototype);
    _objectCtor->setReturnValue(newObject());

    _functionCtor = new FunctionCtor(this);
    _functionCtor->setPrototype(_functionPrototype);
    _functionCtor->setMember("prototype", _functionPrototype);
    _functionCtor->setReturnValue(newFunction());

    _arrayCtor = new ArrayCtor(this);
    _arrayCtor->setPrototype(_functionPrototype);
    _arrayCtor->setMember("prototype", _arrayPrototype);
    _arrayCtor->setReturnValue(newArray());

    _stringCtor = new StringCtor(this);
    _stringCtor->setPrototype(_functionPrototype);
    _stringCtor->setMember("prototype", _stringPrototype);
    _stringCtor->setReturnValue(stringValue());

    _booleanCtor = new BooleanCtor(this);
    _booleanCtor->setPrototype(_functionPrototype);
    _booleanCtor->setMember("prototype", _booleanPrototype);
    _booleanCtor->setReturnValue(booleanValue());

    _numberCtor = new NumberCtor(this);
    _numberCtor->setPrototype(_functionPrototype);
    _numberCtor->setMember("prototype", _numberPrototype);
    _numberCtor->setReturnValue(numberValue());

    _dateCtor = new DateCtor(this);
    _dateCtor->setPrototype(_functionPrototype);
    _dateCtor->setMember("prototype", _datePrototype);
    _dateCtor->setReturnValue(_datePrototype);

    _regexpCtor = new RegExpCtor(this);
    _regexpCtor->setPrototype(_functionPrototype);
    _regexpCtor->setMember("prototype", _regexpPrototype);
    _regexpCtor->setReturnValue(_regexpPrototype);

    addFunction(_objectCtor, "getPrototypeOf", 1);
    addFunction(_objectCtor, "getOwnPropertyDescriptor", 2);
    addFunction(_objectCtor, "getOwnPropertyNames", newArray(), 1);
    addFunction(_objectCtor, "create", 1);
    addFunction(_objectCtor, "defineProperty", 3);
    addFunction(_objectCtor, "defineProperties", 2);
    addFunction(_objectCtor, "seal", 1);
    addFunction(_objectCtor, "freeze", 1);
    addFunction(_objectCtor, "preventExtensions", 1);
    addFunction(_objectCtor, "isSealed", booleanValue(), 1);
    addFunction(_objectCtor, "isFrozen", booleanValue(), 1);
    addFunction(_objectCtor, "isExtensible", booleanValue(), 1);
    addFunction(_objectCtor, "keys", newArray(), 1);

    addFunction(_objectPrototype, "toString", stringValue(), 0);
    addFunction(_objectPrototype, "toLocaleString", stringValue(), 0);
    addFunction(_objectPrototype, "valueOf", 0); // ### FIXME it should return thisObject
    addFunction(_objectPrototype, "hasOwnProperty", booleanValue(), 1);
    addFunction(_objectPrototype, "isPrototypeOf", booleanValue(), 1);
    addFunction(_objectPrototype, "propertyIsEnumerable", booleanValue(), 1);

    // set up the default Function prototype
    _functionPrototype->setMember("constructor", _functionCtor);
    addFunction(_functionPrototype, "toString", stringValue(), 0);
    addFunction(_functionPrototype, "apply", 2);
    addFunction(_functionPrototype, "call", 1);
    addFunction(_functionPrototype, "bind", 1);

    // set up the default Array prototype
    addFunction(_arrayCtor, "isArray", booleanValue(), 1);

    _arrayPrototype->setMember("constructor", _arrayCtor);
    addFunction(_arrayPrototype, "toString", stringValue(), 0);
    addFunction(_arrayPrototype, "toLocalString", stringValue(), 0);
    addFunction(_arrayPrototype, "concat", 0);
    addFunction(_arrayPrototype, "join", 1);
    addFunction(_arrayPrototype, "pop", 0);
    addFunction(_arrayPrototype, "push", 0);
    addFunction(_arrayPrototype, "reverse", 0);
    addFunction(_arrayPrototype, "shift", 0);
    addFunction(_arrayPrototype, "slice", 2);
    addFunction(_arrayPrototype, "sort", 1);
    addFunction(_arrayPrototype, "splice", 2);
    addFunction(_arrayPrototype, "unshift", 0);
    addFunction(_arrayPrototype, "indexOf", numberValue(), 1);
    addFunction(_arrayPrototype, "lastIndexOf", numberValue(), 1);
    addFunction(_arrayPrototype, "every", 1);
    addFunction(_arrayPrototype, "some", 1);
    addFunction(_arrayPrototype, "forEach", 1);
    addFunction(_arrayPrototype, "map", 1);
    addFunction(_arrayPrototype, "filter", 1);
    addFunction(_arrayPrototype, "reduce", 1);
    addFunction(_arrayPrototype, "reduceRight", 1);

    // set up the default String prototype
    addFunction(_stringCtor, "fromCharCode", stringValue(), 0);

    _stringPrototype->setMember("constructor", _stringCtor);
    addFunction(_stringPrototype, "toString", stringValue(), 0);
    addFunction(_stringPrototype, "valueOf", stringValue(), 0);
    addFunction(_stringPrototype, "charAt", stringValue(), 1);
    addFunction(_stringPrototype, "charCodeAt", stringValue(), 1);
    addFunction(_stringPrototype, "concat", stringValue(), 0);
    addFunction(_stringPrototype, "indexOf", numberValue(), 2);
    addFunction(_stringPrototype, "lastIndexOf", numberValue(), 2);
    addFunction(_stringPrototype, "localeCompare", booleanValue(), 1);
    addFunction(_stringPrototype, "match", newArray(), 1);
    addFunction(_stringPrototype, "replace", stringValue(), 2);
    addFunction(_stringPrototype, "search", numberValue(), 1);
    addFunction(_stringPrototype, "slice", stringValue(), 2);
    addFunction(_stringPrototype, "split", newArray(), 1);
    addFunction(_stringPrototype, "substring", stringValue(), 2);
    addFunction(_stringPrototype, "toLowerCase", stringValue(), 0);
    addFunction(_stringPrototype, "toLocaleLowerCase", stringValue(), 0);
    addFunction(_stringPrototype, "toUpperCase", stringValue(), 0);
    addFunction(_stringPrototype, "toLocaleUpperCase", stringValue(), 0);
    addFunction(_stringPrototype, "trim", stringValue(), 0);

    // set up the default Boolean prototype
    addFunction(_booleanCtor, "fromCharCode", 0);

    _booleanPrototype->setMember("constructor", _booleanCtor);
    addFunction(_booleanPrototype, "toString", stringValue(), 0);
    addFunction(_booleanPrototype, "valueOf", booleanValue(), 0);

    // set up the default Number prototype
    _numberCtor->setMember("MAX_VALUE", numberValue());
    _numberCtor->setMember("MIN_VALUE", numberValue());
    _numberCtor->setMember("NaN", numberValue());
    _numberCtor->setMember("NEGATIVE_INFINITY", numberValue());
    _numberCtor->setMember("POSITIVE_INFINITY", numberValue());

    addFunction(_numberCtor, "fromCharCode", 0);

    _numberPrototype->setMember("constructor", _numberCtor);
    addFunction(_numberPrototype, "toString", stringValue(), 0);
    addFunction(_numberPrototype, "toLocaleString", stringValue(), 0);
    addFunction(_numberPrototype, "valueOf", numberValue(), 0);
    addFunction(_numberPrototype, "toFixed", numberValue(), 1);
    addFunction(_numberPrototype, "toExponential", numberValue(), 1);
    addFunction(_numberPrototype, "toPrecision", numberValue(), 1);

    // set up the Math object
    _mathObject = newObject();
    _mathObject->setMember("E", numberValue());
    _mathObject->setMember("LN10", numberValue());
    _mathObject->setMember("LN2", numberValue());
    _mathObject->setMember("LOG2E", numberValue());
    _mathObject->setMember("LOG10E", numberValue());
    _mathObject->setMember("PI", numberValue());
    _mathObject->setMember("SQRT1_2", numberValue());
    _mathObject->setMember("SQRT2", numberValue());

    addFunction(_mathObject, "abs", numberValue(), 1);
    addFunction(_mathObject, "acos", numberValue(), 1);
    addFunction(_mathObject, "asin", numberValue(), 1);
    addFunction(_mathObject, "atan", numberValue(), 1);
    addFunction(_mathObject, "atan2", numberValue(), 2);
    addFunction(_mathObject, "ceil", numberValue(), 1);
    addFunction(_mathObject, "cos", numberValue(), 1);
    addFunction(_mathObject, "exp", numberValue(), 1);
    addFunction(_mathObject, "floor", numberValue(), 1);
    addFunction(_mathObject, "log", numberValue(), 1);
    addFunction(_mathObject, "max", numberValue(), 0);
    addFunction(_mathObject, "min", numberValue(), 0);
    addFunction(_mathObject, "pow", numberValue(), 2);
    addFunction(_mathObject, "random", numberValue(), 1);
    addFunction(_mathObject, "round", numberValue(), 1);
    addFunction(_mathObject, "sin", numberValue(), 1);
    addFunction(_mathObject, "sqrt", numberValue(), 1);
    addFunction(_mathObject, "tan", numberValue(), 1);

    // set up the default Boolean prototype
    addFunction(_dateCtor, "parse", numberValue(), 1);
    addFunction(_dateCtor, "now", numberValue(), 0);

    _datePrototype->setMember("constructor", _dateCtor);
    addFunction(_datePrototype, "toString", stringValue(), 0);
    addFunction(_datePrototype, "toDateString", stringValue(), 0);
    addFunction(_datePrototype, "toTimeString", stringValue(), 0);
    addFunction(_datePrototype, "toLocaleString", stringValue(), 0);
    addFunction(_datePrototype, "toLocaleDateString", stringValue(), 0);
    addFunction(_datePrototype, "toLocaleTimeString", stringValue(), 0);
    addFunction(_datePrototype, "valueOf", numberValue(), 0);
    addFunction(_datePrototype, "getTime", numberValue(), 0);
    addFunction(_datePrototype, "getFullYear", numberValue(), 0);
    addFunction(_datePrototype, "getUTCFullYear", numberValue(), 0);
    addFunction(_datePrototype, "getMonth", numberValue(), 0);
    addFunction(_datePrototype, "getUTCMonth", numberValue(), 0);
    addFunction(_datePrototype, "getDate", numberValue(), 0);
    addFunction(_datePrototype, "getUTCDate", numberValue(), 0);
    addFunction(_datePrototype, "getHours", numberValue(), 0);
    addFunction(_datePrototype, "getUTCHours", numberValue(), 0);
    addFunction(_datePrototype, "getMinutes", numberValue(), 0);
    addFunction(_datePrototype, "getUTCMinutes", numberValue(), 0);
    addFunction(_datePrototype, "getSeconds", numberValue(), 0);
    addFunction(_datePrototype, "getUTCSeconds", numberValue(), 0);
    addFunction(_datePrototype, "getMilliseconds", numberValue(), 0);
    addFunction(_datePrototype, "getUTCMilliseconds", numberValue(), 0);
    addFunction(_datePrototype, "getTimezoneOffset", numberValue(), 0);
    addFunction(_datePrototype, "setTime", 1);
    addFunction(_datePrototype, "setMilliseconds", 1);
    addFunction(_datePrototype, "setUTCMilliseconds", 1);
    addFunction(_datePrototype, "setSeconds", 1);
    addFunction(_datePrototype, "setUTCSeconds", 1);
    addFunction(_datePrototype, "setMinutes", 1);
    addFunction(_datePrototype, "setUTCMinutes", 1);
    addFunction(_datePrototype, "setHours", 1);
    addFunction(_datePrototype, "setUTCHours", 1);
    addFunction(_datePrototype, "setDate", 1);
    addFunction(_datePrototype, "setUTCDate", 1);
    addFunction(_datePrototype, "setMonth", 1);
    addFunction(_datePrototype, "setUTCMonth", 1);
    addFunction(_datePrototype, "setFullYear", 1);
    addFunction(_datePrototype, "setUTCFullYear", 1);
    addFunction(_datePrototype, "toUTCString", stringValue(), 0);
    addFunction(_datePrototype, "toISOString", stringValue(), 0);
    addFunction(_datePrototype, "toJSON", stringValue(), 1);

    // set up the default Boolean prototype
    _regexpPrototype->setMember("constructor", _regexpCtor);
    addFunction(_regexpPrototype, "exec", newArray(), 1);
    addFunction(_regexpPrototype, "test", booleanValue(), 1);
    addFunction(_regexpPrototype, "toString", stringValue(), 0);

    // fill the Global object
    _globalObject->setMember("Math", _mathObject);
    _globalObject->setMember("Object", objectCtor());
    _globalObject->setMember("Function", functionCtor());
    _globalObject->setMember("Array", arrayCtor());
    _globalObject->setMember("String", stringCtor());
    _globalObject->setMember("Boolean", booleanCtor());
    _globalObject->setMember("Number", numberCtor());
    _globalObject->setMember("Date", dateCtor());
    _globalObject->setMember("RegExp", regexpCtor());


    // global Qt object, in alphabetic order
    _qtObject = newObject(/*prototype */ 0);
    addFunction(_qtObject, QLatin1String("atob"), 1);
    addFunction(_qtObject, QLatin1String("btoa"), 1);
    addFunction(_qtObject, QLatin1String("createComponent"), 1);
    addFunction(_qtObject, QLatin1String("createQmlObject"), 3);
    addFunction(_qtObject, QLatin1String("darker"), 1);
    addFunction(_qtObject, QLatin1String("fontFamilies"), 0);
    addFunction(_qtObject, QLatin1String("formatDate"), 2);
    addFunction(_qtObject, QLatin1String("formatDateTime"), 2);
    addFunction(_qtObject, QLatin1String("formatTime"), 2);
    addFunction(_qtObject, QLatin1String("hsla"), 4);
    addFunction(_qtObject, QLatin1String("include"), 2);
    addFunction(_qtObject, QLatin1String("isQtObject"), 1);
    addFunction(_qtObject, QLatin1String("lighter"), 1);
    addFunction(_qtObject, QLatin1String("md5"), 1);
    addFunction(_qtObject, QLatin1String("openUrlExternally"), 1);
    addFunction(_qtObject, QLatin1String("point"), 2);
    addFunction(_qtObject, QLatin1String("quit"), 0);
    addFunction(_qtObject, QLatin1String("rect"), 4);
    addFunction(_qtObject, QLatin1String("resolvedUrl"), 1);
    addFunction(_qtObject, QLatin1String("rgba"), 4);
    addFunction(_qtObject, QLatin1String("size"), 2);
    addFunction(_qtObject, QLatin1String("tint"), 2);
    addFunction(_qtObject, QLatin1String("vector3d"), 3);
    _globalObject->setMember(QLatin1String("Qt"), _qtObject);

    // firebug/webkit compat
    ObjectValue *consoleObject = newObject(/*prototype */ 0);
    addFunction(consoleObject, QLatin1String("log"), 1);
    addFunction(consoleObject, QLatin1String("debug"), 1);
    _globalObject->setMember(QLatin1String("console"), consoleObject);

    // translation functions
    addFunction(_globalObject, QLatin1String("qsTr"), 3);
    addFunction(_globalObject, QLatin1String("QT_TR_NOOP"), 1);
    addFunction(_globalObject, QLatin1String("qsTranslate"), 5);
    addFunction(_globalObject, QLatin1String("QT_TRANSLATE_NOOP"), 2);
    addFunction(_globalObject, QLatin1String("qsTrId"), 2);
    addFunction(_globalObject, QLatin1String("QT_TRID_NOOP"), 1);

    // QML objects
    _qmlFontObject = newObject(/*prototype =*/ 0);
    _qmlFontObject->setClassName(QLatin1String("Font"));
    _qmlFontObject->setMember("family", stringValue());
    _qmlFontObject->setMember("weight", undefinedValue()); // ### make me an object
    _qmlFontObject->setMember("capitalization", undefinedValue()); // ### make me an object
    _qmlFontObject->setMember("bold", booleanValue());
    _qmlFontObject->setMember("italic", booleanValue());
    _qmlFontObject->setMember("underline", booleanValue());
    _qmlFontObject->setMember("overline", booleanValue());
    _qmlFontObject->setMember("strikeout", booleanValue());
    _qmlFontObject->setMember("pointSize", intValue());
    _qmlFontObject->setMember("pixelSize", intValue());
    _qmlFontObject->setMember("letterSpacing", realValue());
    _qmlFontObject->setMember("wordSpacing", realValue());

    _qmlPointObject = newObject(/*prototype =*/ 0);
    _qmlPointObject->setClassName(QLatin1String("Point"));
    _qmlPointObject->setMember("x", numberValue());
    _qmlPointObject->setMember("y", numberValue());

    _qmlSizeObject = newObject(/*prototype =*/ 0);
    _qmlSizeObject->setClassName(QLatin1String("Size"));
    _qmlSizeObject->setMember("width", numberValue());
    _qmlSizeObject->setMember("height", numberValue());

    _qmlRectObject = newObject(/*prototype =*/ 0);
    _qmlRectObject->setClassName("Rect");
    _qmlRectObject->setMember("x", numberValue());
    _qmlRectObject->setMember("y", numberValue());
    _qmlRectObject->setMember("width", numberValue());
    _qmlRectObject->setMember("height", numberValue());

    _qmlVector3DObject = newObject(/*prototype =*/ 0);
    _qmlVector3DObject->setClassName(QLatin1String("Vector3D"));
    _qmlVector3DObject->setMember("x", realValue());
    _qmlVector3DObject->setMember("y", realValue());
    _qmlVector3DObject->setMember("z", realValue());
}

const ObjectValue *Engine::qmlKeysObject()
{
    return _qmlKeysObject;
}

const ObjectValue *Engine::qmlFontObject()
{
    return _qmlFontObject;
}

const ObjectValue *Engine::qmlPointObject()
{
    return _qmlPointObject;
}

const ObjectValue *Engine::qmlSizeObject()
{
    return _qmlSizeObject;
}

const ObjectValue *Engine::qmlRectObject()
{
    return _qmlRectObject;
}

const ObjectValue *Engine::qmlVector3DObject()
{
    return _qmlVector3DObject;
}

const Value *Engine::defaultValueForBuiltinType(const QString &typeName) const
{
    if (typeName == QLatin1String("string"))
        return stringValue();
    else if (typeName == QLatin1String("url"))
        return urlValue();
    else if (typeName == QLatin1String("bool"))
        return booleanValue();
    else if (typeName == QLatin1String("int"))
        return intValue();
    else if (typeName == QLatin1String("real"))
        return realValue();
    else if (typeName == QLatin1String("color"))
        return colorValue();
    // ### more types...

    return undefinedValue();
}

ASTObjectValue::ASTObjectValue(UiQualifiedId *typeName,
                               UiObjectInitializer *initializer,
                               const QmlJS::Document *doc,
                               Engine *engine)
    : ObjectValue(engine), _typeName(typeName), _initializer(initializer), _doc(doc), _defaultPropertyRef(0)
{
    if (_initializer) {
        for (UiObjectMemberList *it = _initializer->members; it; it = it->next) {
            UiObjectMember *member = it->member;
            if (UiPublicMember *def = cast<UiPublicMember *>(member)) {
                if (def->type == UiPublicMember::Property && def->name && def->memberType) {
                    ASTPropertyReference *ref = new ASTPropertyReference(def, _doc, engine);
                    _properties.append(ref);
                    if (def->defaultToken.isValid())
                        _defaultPropertyRef = ref;
                } else if (def->type == UiPublicMember::Signal && def->name) {
                    ASTSignalReference *ref = new ASTSignalReference(def, _doc, engine);
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

ASTVariableReference::ASTVariableReference(VariableDeclaration *ast, Engine *engine)
    : Reference(engine), _ast(ast)
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

ASTFunctionValue::ASTFunctionValue(FunctionExpression *ast, const QmlJS::Document *doc, Engine *engine)
    : FunctionValue(engine), _ast(ast), _doc(doc)
{
    setPrototype(engine->functionPrototype());

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
    return engine()->undefinedValue();
}

int ASTFunctionValue::argumentCount() const
{
    return _argumentNames.size();
}

const Value *ASTFunctionValue::argument(int) const
{
    return engine()->undefinedValue();
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
                                             Engine *engine)
    : Reference(engine),
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

ASTPropertyReference::ASTPropertyReference(UiPublicMember *ast, const QmlJS::Document *doc, Engine *engine)
    : Reference(engine), _ast(ast), _doc(doc)
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
        return engine()->defaultValueForBuiltinType(_ast->memberType->asString());

    return engine()->undefinedValue();
}

ASTSignalReference::ASTSignalReference(UiPublicMember *ast, const QmlJS::Document *doc, Engine *engine)
    : Reference(engine), _ast(ast), _doc(doc)
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
    return engine()->undefinedValue();
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

TypeScope::TypeScope(const Imports *imports, Engine *engine)
    : ObjectValue(engine)
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

JSImportScope::JSImportScope(const Imports *imports, Engine *engine)
    : ObjectValue(engine)
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

Imports::Imports(Engine *engine)
    : _typeScope(new TypeScope(this, engine))
    , _jsImportScope(new JSImportScope(this, engine))
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
