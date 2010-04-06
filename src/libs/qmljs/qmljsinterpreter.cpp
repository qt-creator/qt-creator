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

#include "qmljsinterpreter.h"
#include "qmljsevaluate.h"
#include "qmljslink.h"
#include "parser/qmljsast_p.h"

#include <QtCore/QFile>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QMetaObject>
#include <QtCore/QMetaProperty>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>

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

} // end of anonymous namespace

namespace QmlJS {
namespace Interpreter {

class FakeMetaEnum {
    QString m_name;
    QStringList m_keys;
    QList<int> m_values;

public:
    FakeMetaEnum(const QString &name)
        : m_name(name)
    {}

    QString name() const
    { return m_name; }

    void addKey(const QString &key, int value)
    { m_keys.append(key); m_values.append(value); }

    QString key(int index) const
    { return m_keys.at(index); }

    int keyCount() const
    { return m_keys.size(); }
};

class FakeMetaMethod {
public:
    enum {
        Signal,
        Slot,
        Method
    };

    enum {
        Private,
        Protected,
        Public
    };

public:
    FakeMetaMethod(const QString &name, const QString &returnType = QString())
        : m_name(name), m_returnType(returnType), m_methodTy(Method), m_methodAccess(Public)
    {}

    QString methodName() const
    { return m_name; }

    QStringList parameterNames() const
    { return m_paramNames; }

    QStringList parameterTypes() const
    { return m_paramTypes; }

    void addParameter(const QString &name, const QString &type)
    { m_paramNames.append(name); m_paramTypes.append(type); }

    int methodType() const
    { return m_methodTy; }
    void setMethodType(int methodType)
    { m_methodTy = methodType; }

    int access() const
    { return m_methodAccess; }

private:
    QString m_name;
    QString m_returnType;
    QStringList m_paramNames;
    QStringList m_paramTypes;
    int m_methodTy;
    int m_methodAccess;
};

class FakeMetaProperty {
    QString m_propertyName;
    QString m_type;
    bool m_isList;

public:
    FakeMetaProperty(const QString &name, const QString &type, bool isList)
        : m_propertyName(name), m_type(type), m_isList(isList)
    {}

    QString name() const
    { return m_propertyName; }

    QString typeName() const
    { return m_type; }
};

class FakeMetaObject {
    FakeMetaObject(FakeMetaObject&);
    FakeMetaObject &operator=(const FakeMetaObject&);

    QString m_name;
    QString m_package;
    int m_major;
    int m_minor;
    const FakeMetaObject *m_super;
    QString m_superName;
    QList<FakeMetaEnum> m_enums;
    QHash<QString, int> m_enumNameToIndex;
    QList<FakeMetaProperty> m_props;
    QHash<QString, int> m_propNameToIdx;
    QList<FakeMetaMethod> m_methods;
    QString m_defaultPropertyName;

public:
    FakeMetaObject(const QString &name, const QString &package, int majorVersion, int minorVersion)
        : m_name(name), m_package(package), m_major(majorVersion), m_minor(minorVersion), m_super(0)
    {}

    void setSuperclassName(const QString &superclass)
    { m_superName = superclass; }
    QString superclassName() const
    { return m_superName; }

    void setSuperclass(FakeMetaObject *superClass)
    { m_super = superClass; }
    const FakeMetaObject *superClass() const
    { return m_super; }
    QString className() const
    { return m_name; }
    QString packageName() const
    { return m_package; }

    void addEnum(const FakeMetaEnum &fakeEnum)
    { m_enumNameToIndex.insert(fakeEnum.name(), m_enums.size()); m_enums.append(fakeEnum); }
    int enumeratorCount() const
    { return m_enums.size(); }
    int enumeratorOffset() const
    { return 0; }
    FakeMetaEnum enumerator(int index) const
    { return m_enums.at(index); }
    int enumeratorIndex(const QString &name) const
    { return m_enumNameToIndex.value(name, -1); }

    void addProperty(const FakeMetaProperty &property)
    { m_propNameToIdx.insert(property.name(), m_props.size()); m_props.append(property); }
    int propertyCount() const
    { return m_props.size(); }
    int propertyOffset() const
    { return 0; }
    FakeMetaProperty property(int index) const
    { return m_props.at(index); }
    int propertyIndex(const QString &name) const
    { return m_propNameToIdx.value(name, -1); }

    void addMethod(const FakeMetaMethod &method)
    { m_methods.append(method); }
    int methodCount() const
    { return m_methods.size(); }
    int methodOffset() const
    { return 0; }
    FakeMetaMethod method(int index) const
    { return m_methods.at(index); }

    int majorVersion() const
    { return m_major; }
    int minorVersion() const
    { return m_minor; }

    QString defaultPropertyName() const
    { return m_defaultPropertyName; }

    void setDefaultPropertyName(const QString defaultPropertyName)
    { m_defaultPropertyName = defaultPropertyName; }
};

} // end of Interpreter namespace
} // end of QmlJS namespace

namespace {

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

class QmlXmlReader
{
public:
    QmlXmlReader(QIODevice *dev)
        : _xml(dev)
    {}

    bool operator()(QMap<QString, FakeMetaObject *> *objects) {
        Q_ASSERT(objects);
        _objects = objects;

        if (_xml.readNextStartElement()) {
            if (_xml.name() == "module")
                readModule();
            else
                _xml.raiseError(QCoreApplication::translate("QmlJS::Interpreter::QmlXmlReader", "The file is not module file."));
        }

        return !_xml.error();
    }

    QString errorMessage() const {
        return _xml.errorString();
    }

private:
    void unexpectedElement(const QStringRef &child, const QString &parent) {
        _xml.raiseError(QCoreApplication::translate("QmlJS::Interpreter::QmlXmlReader", "Unexpected element <%1> in <%2>").arg(child.toString(), parent));
    }

    void ignoreAttr(const QXmlStreamAttribute &attr) {
        qDebug() << "** ignoring attribute" << attr.name().toString()
                 << "in tag" << _xml.name();
    }

    void invalidAttr(const QString &value, const QString &attrName, const QString &tag) {
        _xml.raiseError(QCoreApplication::translate("QmlJS::Interpreter::QmlXmlReader", "invalid value '%1' for attribute %2 in <%3>").arg(value, attrName, tag));
    }

    void noValidAttr(const QString &attrName, const QString &tag) {
        _xml.raiseError(QCoreApplication::translate("QmlJS::Interpreter::QmlXmlReader", "<%1> has no valid %2 attribute").arg(tag, attrName));
    }

    void readModule()
    {
        Q_ASSERT(_xml.isStartElement() && _xml.name() == QLatin1String("module"));

        foreach (const QXmlStreamAttribute &attr, _xml.attributes())
            ignoreAttr(attr);

        while (_xml.readNextStartElement()) {
            if (_xml.name() == QLatin1String("type"))
                readType();
            else
                unexpectedElement(_xml.name(), QLatin1String("module"));
        }
    }

    void readType()
    {
        const QLatin1String tag("type");
        Q_ASSERT(_xml.isStartElement() && _xml.name() == tag);

        QString name, defaultPropertyName;
        int major = -1, minor = -1;
        QString extends;
        foreach (const QXmlStreamAttribute &attr, _xml.attributes()) {
            if (attr.name() == QLatin1String("name")) {
                name = attr.value().toString();
                if (name.isEmpty()) {
                    invalidAttr(name, QLatin1String("name"), tag);
                    return;
                }
            } else if (attr.name() == QLatin1String("version")) {
                QString version = attr.value().toString();
                int dotIdx = version.indexOf('.');
                if (dotIdx == -1) {
                    bool ok = false;
                    major = version.toInt(&ok);
                    if (!ok) {
                        invalidAttr(version, QLatin1String("version"), tag);
                        return;
                    }
                    minor = -1;
                } else {
                    bool ok = false;
                    major = version.left(dotIdx).toInt(&ok);
                    if (!ok) {
                        invalidAttr(version, QLatin1String("version"), tag);
                        return;
                    }
                    minor = version.mid(dotIdx + 1).toInt(&ok);
                    if (!ok) {
                        invalidAttr(version, QLatin1String("version"), tag);
                        return;
                    }
                }
            } else if (attr.name() == QLatin1String("defaultProperty")) {
                defaultPropertyName = attr.value().toString();
            } else if (attr.name() == QLatin1String("extends")) {
                if (! attr.value().isEmpty())
                    extends = attr.value().toString();
            } else {
                ignoreAttr(attr);
            }
        }

        QString className, packageName;
        split(name, &packageName, &className);
        FakeMetaObject *metaObject = new FakeMetaObject(className, packageName,
                                                        major, minor);
        if (! extends.isEmpty())
            metaObject->setSuperclassName(extends);
        if (! defaultPropertyName.isEmpty())
            metaObject->setDefaultPropertyName(defaultPropertyName);

        while (_xml.readNextStartElement()) {
            if (_xml.name() == QLatin1String("property"))
                readProperty(metaObject);
            else if (_xml.name() == QLatin1String("enum"))
                readEnum(metaObject);
            else if (_xml.name() == QLatin1String("signal"))
                readSignal(metaObject);
            else if (_xml.name() == QLatin1String("method"))
                readMethod(metaObject);
            else
                unexpectedElement(_xml.name(), tag);
        }

        _objects->insert(name, metaObject);
    }

    bool split(const QString &name, QString *packageName, QString *className) {
        int dotIdx = name.lastIndexOf(QLatin1Char('.'));
        if (dotIdx != -1) {
            if (packageName)
                *packageName = name.left(dotIdx);
            if (className)
                *className = name.mid(dotIdx + 1);
            return true;
        } else {
            if (packageName)
                packageName->clear();
            if (className)
                *className = name;
            return false;
        }
    }

    void readProperty(FakeMetaObject *metaObject)
    {
        const QLatin1String tag("property");
        Q_ASSERT(_xml.isStartElement() && _xml.name() == tag);

        QString name, type;
        bool isList = false;
        foreach (const QXmlStreamAttribute &attr, _xml.attributes()) {
            if (attr.name() == QLatin1String("name")) {
                name = attr.value().toString();
            } else if (attr.name() == QLatin1String("type")) {
                type = attr.value().toString();
            } else if (attr.name() == QLatin1String("isList")) {
                if (attr.value() == QLatin1String("true")) {
                    isList = true;
                } else if (attr.value() == QLatin1String("false")) {
                    isList = false;
                } else {
                    invalidAttr(attr.value().toString(), QLatin1String("idList"), tag);
                    return;
                }
            } else {
                ignoreAttr(attr);
            }
        }

        if (name.isEmpty())
            noValidAttr(QLatin1String("name"), tag);
        else if (type.isEmpty())
            noValidAttr(QLatin1String("type"), tag);
        else
            createProperty(metaObject, name, type, isList);

        while (_xml.readNextStartElement()) {
            unexpectedElement(_xml.name(), tag);
        }
    }

    void createProperty(FakeMetaObject *metaObject, const QString &name,
                        const QString &type, bool isList) {
        Q_ASSERT(metaObject);

        metaObject->addProperty(FakeMetaProperty(name, type, isList));
    }

    void readEnum(FakeMetaObject *metaObject)
    {
        Q_ASSERT(metaObject);

        QLatin1String tag("enum");
        Q_ASSERT(_xml.isStartElement() && _xml.name() == tag);

        QString name;
        foreach (const QXmlStreamAttribute &attr, _xml.attributes()) {
            if (attr.name() == QLatin1String("name")) {
                name = attr.value().toString();
            } else {
                ignoreAttr(attr);
            }
        }

        if (name.isEmpty()) {
            noValidAttr(QLatin1String("name"), tag);
            return;
        }

        FakeMetaEnum metaEnum(name);

        while (_xml.readNextStartElement()) {
            if (_xml.name() == QLatin1String("enumerator"))
                readEnumerator(&metaEnum);
            else
                unexpectedElement(_xml.name(), tag);
        }

        metaObject->addEnum(metaEnum);
    }

    void readEnumerator(FakeMetaEnum *metaEnum)
    {
        Q_ASSERT(metaEnum);

        QLatin1String tag("enumerator");
        Q_ASSERT(_xml.isStartElement() && _xml.name() == tag);

        QString name;
        int value = 0;
        foreach (const QXmlStreamAttribute &attr, _xml.attributes()) {
            if (attr.name() == QLatin1String("name")) {
                name = attr.value().toString();
            } else if (attr.name() == QLatin1String("value")) {
                const QString valueStr = attr.value().toString();
                bool ok = false;
                value = valueStr.toInt(&ok);
                if (!ok) {
                    invalidAttr(valueStr, QLatin1String("value"), tag);
                }
            } else {
                ignoreAttr(attr);
            }
        }

        if (name.isEmpty())
            noValidAttr(QLatin1String("name"), tag);
        else
            metaEnum->addKey(name, value);

        while (_xml.readNextStartElement()) {
            unexpectedElement(_xml.name(), tag);
        }
    }

    void readSignal(FakeMetaObject *metaObject)
    {
        Q_ASSERT(metaObject);
        QLatin1String tag("signal");
        Q_ASSERT(_xml.isStartElement() && _xml.name() == tag);

        QString name;
        foreach (const QXmlStreamAttribute &attr, _xml.attributes()) {
            if (attr.name() == QLatin1String("name")) {
                name = attr.value().toString();
            } else {
                ignoreAttr(attr);
            }
        }

        if (name.isEmpty()) {
            noValidAttr(QLatin1String("name"), tag);
            return;
        }

        FakeMetaMethod method(name);
        method.setMethodType(FakeMetaMethod::Signal);

        while (_xml.readNextStartElement()) {
            if (_xml.name() == QLatin1String("param")) {
                readParam(&method);
            } else {
                unexpectedElement(_xml.name(), tag);
            }
        }

        metaObject->addMethod(method);
    }

    void readParam(FakeMetaMethod *method)
    {
        Q_ASSERT(method);
        QLatin1String tag("param");
        Q_ASSERT(_xml.isStartElement() && _xml.name() == tag);

        QString name, type;
        foreach (const QXmlStreamAttribute &attr, _xml.attributes()) {
            if (attr.name() == QLatin1String("name")) {
                name = attr.value().toString();
            } else if (attr.name() == QLatin1String("type")) {
                type = attr.value().toString();
            } else {
                ignoreAttr(attr);
            }
        }

        // note: name attribute is optional
        if (type.isEmpty())
            noValidAttr(QLatin1String("type"), tag);

        method->addParameter(name, type);

        while (_xml.readNextStartElement()) {
            unexpectedElement(_xml.name(), tag);
        }
    }

    void readMethod(FakeMetaObject *metaObject)
    {
        Q_ASSERT(metaObject);
        QLatin1String tag("method");
        Q_ASSERT(_xml.isStartElement() && _xml.name() == tag);

        QString name, type;
        foreach (const QXmlStreamAttribute &attr, _xml.attributes()) {
            if (attr.name() == QLatin1String("name")) {
                name = attr.value().toString();
            } else if (attr.name() == QLatin1String("type")) {
                type = attr.value().toString();
            } else {
                ignoreAttr(attr);
            }
        }

        // note: type attribute is optional, in which case it's a void method.
        if (name.isEmpty()) {
            noValidAttr(QLatin1String("name"), tag);
            return;
        }

        FakeMetaMethod method(name, type);

        while (_xml.readNextStartElement()) {
            if (_xml.name() == QLatin1String("param")) {
                readParam(&method);
            } else {
                unexpectedElement(_xml.name(), tag);
            }
        }

        metaObject->addMethod(method);
    }

private:
    QXmlStreamReader _xml;
    QMap<QString, FakeMetaObject *> *_objects;
};

} // end of anonymous namespace

const int QmlObjectValue::NoVersion = -1;

QmlObjectValue::QmlObjectValue(const FakeMetaObject *metaObject, Engine *engine)
    : ObjectValue(engine),
      _metaObject(metaObject)
{
    setClassName(metaObject->className()); // ### TODO: we probably need to do more than just this...
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

    // process the meta properties
    for (int index = 0; index < _metaObject->propertyCount(); ++index) {
        FakeMetaProperty prop = _metaObject->property(index);

        processor->processProperty(prop.name(), propertyValue(prop));
    }

    // process the meta methods
    for (int index = 0; index < _metaObject->methodCount(); ++index) {
        FakeMetaMethod method = _metaObject->method(index);
        QString methodName;
        const Value *signature = findOrCreateSignature(index, method, &methodName);

        if (method.methodType() == FakeMetaMethod::Slot && method.access() == FakeMetaMethod::Public) {
            processor->processSlot(methodName, signature);

        } else if (method.methodType() == FakeMetaMethod::Signal && method.access() != FakeMetaMethod::Private) {
            // process the signal
            processor->processSignal(methodName, signature);

            QString slotName;
            slotName += QLatin1String("on");
            slotName += methodName.at(0).toUpper();
            slotName += methodName.midRef(1);

            // process the generated slot
            processor->processGeneratedSlot(slotName, signature);
        }
    }

    ObjectValue::processMembers(processor);
}

const Value *QmlObjectValue::propertyValue(const FakeMetaProperty &prop) const
{
    const QString typeName = prop.typeName();

    // ### Verify type resolving.
    QmlObjectValue *objectValue = engine()->metaTypeSystem().staticTypeForImport(typeName);
    if (objectValue)
        return objectValue;

    const Value *value = engine()->undefinedValue();
    if (typeName == QLatin1String("QByteArray")
            || typeName == QLatin1String("string")
            || typeName == QLatin1String("QString")
            || typeName == QLatin1String("QUrl")) {
        value = engine()->stringValue();
    } else if (typeName == QLatin1String("bool")) {
        value = engine()->booleanValue();
    } else if (typeName == QLatin1String("int")
            || typeName == QLatin1String("float")
            || typeName == QLatin1String("double")
            || typeName == QLatin1String("qreal")
            || typeName == QLatin1String("long")
            // ### Review: more types here?
            ) {
        value = engine()->numberValue();
    } else if (typeName == QLatin1String("QFont")) {
        ObjectValue *object = engine()->newObject(/*prototype =*/ 0);
        object->setClassName(QLatin1String("Font"));
        object->setProperty("family", engine()->stringValue());
        object->setProperty("weight", engine()->undefinedValue()); // ### make me an object
        object->setProperty("copitalization", engine()->undefinedValue()); // ### make me an object
        object->setProperty("bold", engine()->booleanValue());
        object->setProperty("italic", engine()->booleanValue());
        object->setProperty("underline", engine()->booleanValue());
        object->setProperty("overline", engine()->booleanValue());
        object->setProperty("strikeout", engine()->booleanValue());
        object->setProperty("pointSize", engine()->numberValue());
        object->setProperty("pixelSize", engine()->numberValue());
        object->setProperty("letterSpacing", engine()->numberValue());
        object->setProperty("wordSpacing", engine()->numberValue());
        value = object;
    } else if (typeName == QLatin1String("QPoint")
            || typeName == QLatin1String("QPointF")
            || typeName == QLatin1String("QVector2D")) {
        // ### cache
        ObjectValue *object = engine()->newObject(/*prototype =*/ 0);
        object->setClassName(QLatin1String("Point"));
        object->setProperty("x", engine()->numberValue());
        object->setProperty("y", engine()->numberValue());
        value = object;
    } else if (typeName == QLatin1String("QRect")
            || typeName == QLatin1String("QRectF")) {
        // ### cache
        ObjectValue *object = engine()->newObject(/*prototype =*/ 0);
        object->setClassName("Rect");
        object->setProperty("x", engine()->numberValue());
        object->setProperty("y", engine()->numberValue());
        object->setProperty("width", engine()->numberValue());
        object->setProperty("height", engine()->numberValue());
        value = object;
    } else if (typeName == QLatin1String("QVector3D")) {
        // ### cache
        ObjectValue *object = engine()->newObject(/*prototype =*/ 0);
        object->setClassName(QLatin1String("Vector3D"));
        object->setProperty("x", engine()->numberValue());
        object->setProperty("y", engine()->numberValue());
        object->setProperty("z", engine()->numberValue());
        value = object;
    } else if (typeName == QLatin1String("QColor")) {
        value = engine()->colorValue();
    } else if (typeName == QLatin1String("QDeclarativeAnchorLine")) {
        value = engine()->anchorLineValue();
    } else if (typeName == QLatin1String("QEasingCurve")) {
        // ### cache
        ObjectValue *object = engine()->newObject(/*prototype =*/ 0);
        object->setClassName(QLatin1String("EasingCurve"));
        object->setProperty("type", engine()->easingCurveNameValue());
        object->setProperty("period", engine()->numberValue());
        object->setProperty("amplitude", engine()->numberValue());
        object->setProperty("overshoot", engine()->numberValue());
        value = object;
    }

    return value;
}

QString QmlObjectValue::packageName() const
{ return _metaObject->packageName(); }

int QmlObjectValue::majorVersion() const
{ return _metaObject->majorVersion(); }

int QmlObjectValue::minorVersion() const
{ return _metaObject->minorVersion(); }

QString QmlObjectValue::defaultPropertyName() const
{ return _metaObject->defaultPropertyName(); }

QString QmlObjectValue::propertyType(const QString &propertyName) const
{
    for (const FakeMetaObject *iter = _metaObject; iter; iter = iter->superClass()) {
        int propIdx = _metaObject->propertyIndex(propertyName);
        if (propIdx != -1) {
            return _metaObject->property(propIdx).typeName();
        }
    }
    return QString();
}

bool QmlObjectValue::isEnum(const QString &typeName) const
{
    return _metaObject->enumeratorIndex(typeName) != -1;
}

bool QmlObjectValue::isDerivedFrom(const FakeMetaObject *base) const
{
    for (const FakeMetaObject *iter = _metaObject; iter; iter = iter->superClass()) {
        if (iter == base)
            return true;
    }
    return false;
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
    thisObject->setProperty("length", engine()->numberValue());
    return thisObject;
}

const Value *FunctionCtor::invoke(const Activation *activation) const
{
    ObjectValue *thisObject = activation->thisObject();
    if (activation->calledAsFunction())
        thisObject = engine()->newObject();

    thisObject->setClassName("Function");
    thisObject->setPrototype(engine()->functionPrototype());
    thisObject->setProperty("length", engine()->numberValue());
    return thisObject;
}

const Value *ArrayCtor::invoke(const Activation *activation) const
{
    ObjectValue *thisObject = activation->thisObject();
    if (activation->calledAsFunction())
        thisObject = engine()->newObject();

    thisObject->setClassName("Array");
    thisObject->setPrototype(engine()->arrayPrototype());
    thisObject->setProperty("length", engine()->numberValue());
    return thisObject;
}

const Value *StringCtor::invoke(const Activation *activation) const
{
    if (activation->calledAsFunction())
        return engine()->convertToString(activation->thisObject());

    ObjectValue *thisObject = activation->thisObject();
    thisObject->setClassName("String");
    thisObject->setPrototype(engine()->stringPrototype());
    thisObject->setProperty("length", engine()->numberValue());
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
    thisObject->setProperty("source", engine()->stringValue());
    thisObject->setProperty("global", engine()->booleanValue());
    thisObject->setProperty("ignoreCase", engine()->booleanValue());
    thisObject->setProperty("multiline", engine()->booleanValue());
    thisObject->setProperty("lastIndex", engine()->numberValue());
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

void ValueVisitor::visit(const EasingCurveNameValue *)
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

const BooleanValue *Value::asBooleanValue() const
{
    return 0;
}

const StringValue *Value::asStringValue() const
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

const EasingCurveNameValue *Value::asEasingCurveNameValue() const
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

void StringValue::accept(ValueVisitor *visitor) const
{
    visitor->visit(this);
}


ScopeChain::ScopeChain()
    : globalScope(0)
    , qmlTypes(0)
{
}

ScopeChain::QmlComponentChain::QmlComponentChain()
    : rootObject(0), ids(0)
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
    rootObject = 0;
    functionScopes.clear();
    ids = 0;
}

void ScopeChain::QmlComponentChain::add(QList<const ObjectValue *> *list) const
{
    foreach (QmlComponentChain *parent, instantiatingComponents)
        parent->add(list);

    if (rootObject)
        list->append(rootObject);
    list->append(functionScopes);
    if (ids)
        list->append(ids);
}

void ScopeChain::update()
{
    _all.clear();

    _all += globalScope;

    foreach (QmlComponentChain *parent, qmlComponentScope.instantiatingComponents)
        parent->add(&_all);

    if (qmlComponentScope.rootObject && ! qmlScopeObjects.contains(qmlComponentScope.rootObject))
        _all += qmlComponentScope.rootObject;
    _all += qmlScopeObjects;
    _all += qmlComponentScope.functionScopes;
    if (qmlComponentScope.ids)
        _all += qmlComponentScope.ids;
    if (qmlTypes)
        _all += qmlTypes;
    _all += jsScopes;
}

QList<const ObjectValue *> ScopeChain::all() const
{
    return _all;
}


Context::Context(Engine *engine)
    : _engine(engine),
      _lookupMode(JSLookup),
      _qmlScopeObjectIndex(-1),
      _qmlScopeObjectSet(false)
{
}

Context::~Context()
{
}

void Context::build(const QList<Node *> &astPath, QmlJS::Document::Ptr doc,
                    const QmlJS::Snapshot &snapshot, const QStringList &importPaths)
{
    Link link(this, doc, snapshot, importPaths);
    link.scopeChainAt(doc, astPath);
}

Engine *Context::engine() const
{
    return _engine;
}

const ScopeChain &Context::scopeChain() const
{
    return _scopeChain;
}

ScopeChain &Context::scopeChain()
{
    return _scopeChain;
}

Context::LookupMode Context::lookupMode() const
{
    return _lookupMode;
}

void Context::setLookupMode(LookupMode lookupMode)
{
    _lookupMode = lookupMode;
}

const ObjectValue *Context::typeEnvironment(const QmlJS::Document *doc) const
{
    return _typeEnvironments.value(doc->fileName(), 0);
}

void Context::setTypeEnvironment(const QmlJS::Document *doc, const ObjectValue *typeEnvironment)
{
    _typeEnvironments[doc->fileName()] = typeEnvironment;
}

const Value *Context::lookup(const QString &name)
{
    QList<const ObjectValue *> scopes = _scopeChain.all();
    for (int index = scopes.size() - 1; index != -1; --index) {
        const ObjectValue *scope = scopes.at(index);

        if (const Value *member = scope->lookupMember(name, this)) {
            if (_lookupMode == JSLookup || ! dynamic_cast<const ASTVariableReference *>(member)) {
                return member;
            }
        }
    }

    return _engine->undefinedValue();
}

const ObjectValue *Context::lookupType(const QmlJS::Document *doc, UiQualifiedId *qmlTypeName)
{
    const ObjectValue *objectValue = typeEnvironment(doc);

    for (UiQualifiedId *iter = qmlTypeName; objectValue && iter; iter = iter->next) {
        if (! iter->name)
            return 0;

        const Value *value = objectValue->property(iter->name->asString(), this);
        if (!value)
            return 0;

        objectValue = value->asObjectValue();
    }

    return objectValue;
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

const Value *Reference::value(Context *) const
{
    return _engine->undefinedValue();
}

void EasingCurveNameValue::accept(ValueVisitor *visitor) const
{
    visitor->visit(this);
}

QSet<QString> EasingCurveNameValue::_curveNames;
QSet<QString> EasingCurveNameValue::curveNames()
{
    if (_curveNames.isEmpty()) {
        _curveNames = QSet<QString>()
                      << "Linear"
                      << "InQuad"
                      << "OutQuad"
                      << "InOutQuad"
                      << "OutInQuad"
                      << "InCubic"
                      << "OutCubic"
                      << "InOutCubic"
                      << "OutInCubic"
                      << "InQuart"
                      << "OutQuart"
                      << "InOutQuart"
                      << "OutInQuart"
                      << "InQuint"
                      << "OutQuint"
                      << "InOutQuint"
                      << "OutInQuint"
                      << "InSine"
                      << "OutSine"
                      << "InOutSine"
                      << "OutInSine"
                      << "InExpo"
                      << "OutExpo"
                      << "InOutExpo"
                      << "OutInExpo"
                      << "InCirc"
                      << "OutCirc"
                      << "InOutCirc"
                      << "OutInCirc"
                      << "InElastic"
                      << "OutElastic"
                      << "InOutElastic"
                      << "OutInElastic"
                      << "InBack"
                      << "OutBack"
                      << "InOutBack"
                      << "OutInBack"
                      << "InBounce"
                      << "OutBounce"
                      << "InOutBounce"
                      << "OutInBounce";
    }

    return _curveNames;
}

const EasingCurveNameValue *EasingCurveNameValue::asEasingCurveNameValue() const
{
    return this;
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

const ObjectValue *ObjectValue::prototype(Context *context) const
{
    const ObjectValue *prototypeObject = value_cast<const ObjectValue *>(_prototype);
    if (! prototypeObject) {
        if (const Reference *prototypeReference = value_cast<const Reference *>(_prototype)) {
            prototypeObject = value_cast<const ObjectValue *>(prototypeReference->value(context));
        }
    }
    return prototypeObject;
}

void ObjectValue::setPrototype(const Value *prototype)
{
    // ### FIXME: Check for cycles.
    _prototype = prototype;
}

void ObjectValue::setProperty(const QString &name, const Value *value)
{
    _members[name] = value;
}

void ObjectValue::removeProperty(const QString &name)
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

const Value *ObjectValue::property(const QString &name, Context *context) const
{
    return lookupMember(name, context);
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

const Value *ObjectValue::lookupMember(const QString &name, Context *context, bool examinePrototypes) const
{
    if (const Value *m = _members.value(name))
        return m;
    else {
        LookupMember slowLookup(name);
        processMembers(&slowLookup);
        if (slowLookup.value())
            return slowLookup.value();
    }

    if (examinePrototypes) {
        const ObjectValue *prototypeObject = prototype(context);
        if (prototypeObject) {
            if (const Value *m = prototypeObject->lookupMember(name, context))
                return m;
        }
    }

    return 0;
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

const Value *Function::property(const QString &name, Context *context) const
{
    if (name == "length")
        return engine()->numberValue();

    return FunctionValue::property(name, context);
}

const Value *Function::invoke(const Activation *activation) const
{
    return activation->thisObject(); // ### FIXME it should return undefined
}

////////////////////////////////////////////////////////////////////////////////
// typing environment
////////////////////////////////////////////////////////////////////////////////

QList<const FakeMetaObject *> MetaTypeSystem::_metaObjects;

QStringList MetaTypeSystem::load(const QFileInfoList &xmlFiles)
{
    QMap<QString, FakeMetaObject *> objects;

    QStringList errorMsgs;

    foreach (const QFileInfo &xmlFile, xmlFiles) {
        QFile file(xmlFile.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly)) {
            QmlXmlReader read(&file);
            if (!read(&objects)) {
                errorMsgs.append(read.errorMessage());
            }
            file.close();
        } else {
            errorMsgs.append(QCoreApplication::translate("QmlJS::Interpreter::QmlXmlReader", "%1: %2").arg(xmlFile.absoluteFilePath(),
                                                       file.errorString()));
        }
    }

    if (errorMsgs.isEmpty()) {
        qDeleteAll(_metaObjects);
        _metaObjects.clear();

        foreach (FakeMetaObject *obj, objects.values()) {
            const QString superName = obj->superclassName();
            if (! superName.isEmpty()) {
                obj->setSuperclass(objects.value(superName, 0));
            }
            _metaObjects.append(obj);
        }
    }

    return errorMsgs;
}

void MetaTypeSystem::reload(Engine *interpreter)
{
    QHash<const FakeMetaObject *, QmlObjectValue *> qmlObjects;
    _importedTypes.clear();

    foreach (const FakeMetaObject *metaObject, _metaObjects) {
        QmlObjectValue *objectValue = new QmlObjectValue(metaObject, interpreter);
        qmlObjects.insert(metaObject, objectValue);
        _importedTypes[metaObject->packageName()].append(objectValue);
    }

    foreach (const FakeMetaObject *metaObject, _metaObjects) {
        QmlObjectValue *objectValue = qmlObjects.value(metaObject);
        if (!objectValue)
            continue;
        objectValue->setPrototype(qmlObjects.value(metaObject->superClass(), 0));
    }
}

QList<QmlObjectValue *> MetaTypeSystem::staticTypesForImport(const QString &packageName, int majorVersion, int minorVersion) const
{
    QMap<QString, QmlObjectValue *> objectValuesByName;

    foreach (QmlObjectValue *qmlObjectValue, _importedTypes.value(packageName)) {
        if (qmlObjectValue->majorVersion() < majorVersion ||
            (qmlObjectValue->majorVersion() == majorVersion && qmlObjectValue->minorVersion() <= minorVersion)) {
            // we got a candidate.
            const QString typeName = qmlObjectValue->className();
            QmlObjectValue *previousCandidate = objectValuesByName.value(typeName, 0);
            if (previousCandidate) {
                // check if our new candidate is newer than the one we found previously
                if (qmlObjectValue->majorVersion() > previousCandidate->majorVersion() ||
                    (qmlObjectValue->majorVersion() == previousCandidate->majorVersion() && qmlObjectValue->minorVersion() > previousCandidate->minorVersion())) {
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

QmlObjectValue *MetaTypeSystem::staticTypeForImport(const QString &qualifiedName) const
{
    QString name = qualifiedName;
    QString packageName;
    int dotIdx = name.indexOf(QLatin1Char('.'));
    if (dotIdx != -1) {
        packageName = name.left(dotIdx);
        name = name.mid(dotIdx + 1);
    }

    QmlObjectValue *previousCandidate = 0;
    foreach (QmlObjectValue *qmlObjectValue, _importedTypes.value(packageName)) {
        const QString typeName = qmlObjectValue->className();
        if (typeName != name)
            continue;

        if (previousCandidate) {
            // check if our new candidate is newer than the one we found previously
            if (qmlObjectValue->majorVersion() > previousCandidate->majorVersion() ||
                (qmlObjectValue->majorVersion() == previousCandidate->majorVersion() && qmlObjectValue->minorVersion() > previousCandidate->minorVersion())) {
                // the new candidate has a higher version no. than the one we found previously, so replace it
                previousCandidate = qmlObjectValue;
            }
        } else {
            previousCandidate = qmlObjectValue;
        }
    }

    return previousCandidate;
}

bool MetaTypeSystem::hasPackage(const QString &package) const
{
    return _importedTypes.contains(package);
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

void TypeId::visit(const EasingCurveNameValue *)
{
    _result = QLatin1String("string");
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
      _convertToNumber(this),
      _convertToString(this),
      _convertToObject(this)
{
    initializePrototypes();

    _metaTypeSystem.reload(this);
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

const BooleanValue *Engine::booleanValue() const
{
    return &_booleanValue;
}

const StringValue *Engine::stringValue() const
{
    return &_stringValue;
}

const EasingCurveNameValue *Engine::easingCurveNameValue() const
{
    return &_easingCurveNameValue;
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
    object->setProperty(name, function);
}

void Engine::addFunction(ObjectValue *object, const QString &name, int argumentCount)
{
    Function *function = newFunction();
    for (int i = 0; i < argumentCount; ++i)
        function->addArgument(undefinedValue()); // ### introduce unknownValue
    object->setProperty(name, function);
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
    _objectCtor->setProperty("prototype", _objectPrototype);

    _functionCtor = new FunctionCtor(this);
    _functionCtor->setPrototype(_functionPrototype);
    _functionCtor->setProperty("prototype", _functionPrototype);

    _arrayCtor = new ArrayCtor(this);
    _arrayCtor->setPrototype(_functionPrototype);
    _arrayCtor->setProperty("prototype", _arrayPrototype);

    _stringCtor = new StringCtor(this);
    _stringCtor->setPrototype(_functionPrototype);
    _stringCtor->setProperty("prototype", _stringPrototype);

    _booleanCtor = new BooleanCtor(this);
    _booleanCtor->setPrototype(_functionPrototype);
    _booleanCtor->setProperty("prototype", _booleanPrototype);

    _numberCtor = new NumberCtor(this);
    _numberCtor->setPrototype(_functionPrototype);
    _numberCtor->setProperty("prototype", _numberPrototype);

    _dateCtor = new DateCtor(this);
    _dateCtor->setPrototype(_functionPrototype);
    _dateCtor->setProperty("prototype", _datePrototype);

    _regexpCtor = new RegExpCtor(this);
    _regexpCtor->setPrototype(_functionPrototype);
    _regexpCtor->setProperty("prototype", _regexpPrototype);

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
    _functionPrototype->setProperty("constructor", _functionCtor);
    addFunction(_functionPrototype, "toString", stringValue(), 0);
    addFunction(_functionPrototype, "apply", 2);
    addFunction(_functionPrototype, "call", 1);
    addFunction(_functionPrototype, "bind", 1);

    // set up the default Array prototype
    addFunction(_arrayCtor, "isArray", booleanValue(), 1);

    _arrayPrototype->setProperty("constructor", _arrayCtor);
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

    _stringPrototype->setProperty("constructor", _stringCtor);
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

    _booleanPrototype->setProperty("constructor", _booleanCtor);
    addFunction(_booleanPrototype, "toString", stringValue(), 0);
    addFunction(_booleanPrototype, "valueOf", booleanValue(), 0);

    // set up the default Number prototype
    _numberCtor->setProperty("MAX_VALUE", numberValue());
    _numberCtor->setProperty("MIN_VALUE", numberValue());
    _numberCtor->setProperty("NaN", numberValue());
    _numberCtor->setProperty("NEGATIVE_INFINITY", numberValue());
    _numberCtor->setProperty("POSITIVE_INFINITY", numberValue());

    addFunction(_numberCtor, "fromCharCode", 0);

    _numberPrototype->setProperty("constructor", _numberCtor);
    addFunction(_numberPrototype, "toString", stringValue(), 0);
    addFunction(_numberPrototype, "toLocaleString", stringValue(), 0);
    addFunction(_numberPrototype, "valueOf", numberValue(), 0);
    addFunction(_numberPrototype, "toFixed", numberValue(), 1);
    addFunction(_numberPrototype, "toExponential", numberValue(), 1);
    addFunction(_numberPrototype, "toPrecision", numberValue(), 1);

    // set up the Math object
    _mathObject = newObject();
    _mathObject->setProperty("E", numberValue());
    _mathObject->setProperty("LN10", numberValue());
    _mathObject->setProperty("LN2", numberValue());
    _mathObject->setProperty("LOG2E", numberValue());
    _mathObject->setProperty("LOG10E", numberValue());
    _mathObject->setProperty("PI", numberValue());
    _mathObject->setProperty("SQRT1_2", numberValue());
    _mathObject->setProperty("SQRT2", numberValue());

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

    _datePrototype->setProperty("constructor", _dateCtor);
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
    _regexpPrototype->setProperty("constructor", _regexpCtor);
    addFunction(_regexpPrototype, "exec", newArray(), 1);
    addFunction(_regexpPrototype, "test", booleanValue(), 1);
    addFunction(_regexpPrototype, "toString", stringValue(), 0);

    // fill the Global object
    _globalObject->setProperty("Math", _mathObject);
    _globalObject->setProperty("Object", objectCtor());
    _globalObject->setProperty("Function", functionCtor());
    _globalObject->setProperty("Array", arrayCtor());
    _globalObject->setProperty("String", stringCtor());
    _globalObject->setProperty("Boolean", booleanCtor());
    _globalObject->setProperty("Number", numberCtor());
    _globalObject->setProperty("Date", dateCtor());
    _globalObject->setProperty("RegExp", regexpCtor());


    //types
    _qtObject = newObject(/*prototype */ 0);
    addFunction(_qtObject, QLatin1String("rgba"), 4);
    addFunction(_qtObject, QLatin1String("hsla"), 4);
    addFunction(_qtObject, QLatin1String("rect"), 4);
    addFunction(_qtObject, QLatin1String("point"), 2);
    addFunction(_qtObject, QLatin1String("size"), 2);
    addFunction(_qtObject, QLatin1String("vector3d"), 3);

    //color helpers
    addFunction(_qtObject, QLatin1String("lighter"), 1);
    addFunction(_qtObject, QLatin1String("darker"), 1);
    addFunction(_qtObject, QLatin1String("tint"), 2);

    //misc methods
    addFunction(_qtObject, QLatin1String("closestAngle"), 2);
    addFunction(_qtObject, QLatin1String("playSound"), 1);
    addFunction(_qtObject, QLatin1String("openUrlExternally"), 1);
    addFunction(_qtObject, QLatin1String("md5"), 1);
    addFunction(_qtObject, QLatin1String("btoa"), 1);
    addFunction(_qtObject, QLatin1String("atob"), 1);
    addFunction(_qtObject, QLatin1String("quit"), 0);
    addFunction(_qtObject, QLatin1String("resolvedUrl"), 1);

    //firebug/webkit compat
    ObjectValue *consoleObject = newObject(/*prototype */ 0);
    addFunction(consoleObject, QLatin1String("log"), 1);
    addFunction(consoleObject, QLatin1String("debug"), 1);

    _globalObject->setProperty(QLatin1String("console"), consoleObject);

    _globalObject->setProperty(QLatin1String("Qt"), _qtObject);
}

const ObjectValue *Engine::qmlKeysObject()
{
    return _qmlKeysObject;
}

const Value *Engine::defaultValueForBuiltinType(const QString &typeName) const
{
    if (typeName == QLatin1String("string") || typeName == QLatin1String("url"))
        return stringValue();
    else if (typeName == QLatin1String("bool"))
        return booleanValue();
    else if (typeName == QLatin1String("int") || typeName == QLatin1String("real"))
        return numberValue();
    // ### more types...

    return undefinedValue();
}

ASTObjectValue::ASTObjectValue(UiQualifiedId *typeName,
                               UiObjectInitializer *initializer,
                               const QmlJS::Document *doc,
                               Engine *engine)
    : ObjectValue(engine), _typeName(typeName), _initializer(initializer), _doc(doc)
{
    if (_initializer) {
        for (UiObjectMemberList *it = _initializer->members; it; it = it->next) {
            UiObjectMember *member = it->member;
            if (UiPublicMember *def = cast<UiPublicMember *>(member)) {
                if (def->type == UiPublicMember::Property && def->name && def->memberType) {
                    ASTPropertyReference *ref = new ASTPropertyReference(def, _doc, engine);
                    _properties.append(ref);
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

ASTVariableReference::ASTVariableReference(VariableDeclaration *ast, Engine *engine)
    : Reference(engine), _ast(ast)
{
}

ASTVariableReference::~ASTVariableReference()
{
}

const Value *ASTVariableReference::value(Context *context) const
{
    Evaluate check(context);
    return check(_ast->expression);
}

ASTFunctionValue::ASTFunctionValue(FunctionDeclaration *ast, Engine *engine)
    : FunctionValue(engine), _ast(ast)
{
    setPrototype(engine->functionPrototype());

    for (FormalParameterList *it = ast->formals; it; it = it->next)
        _argumentNames.append(it->name);
}

ASTFunctionValue::~ASTFunctionValue()
{
}

FunctionDeclaration *ASTFunctionValue::ast() const
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

const Value *QmlPrototypeReference::value(Context *context) const
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

const Value *ASTPropertyReference::value(Context *context) const
{
    if (_ast->expression) {
        Evaluate check(context);
        return check(_ast->expression);
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

const Value *ASTSignalReference::value(Context *) const
{
    return engine()->undefinedValue();
}
