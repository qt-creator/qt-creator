// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljs_global.h"
#include "qmljsinterpreter.h"

#include <QList>
#include <QMutex>

namespace QmlJS {

////////////////////////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////////////////////////
class Value;
class NullValue;
class UndefinedValue;
class UnknownValue;
class NumberValue;
class IntValue;
class RealValue;
class BooleanValue;
class StringValue;
class UrlValue;
class ObjectValue;
class FunctionValue;
class Reference;
class ColorValue;
class AnchorLineValue;
class Imports;
class TypeScope;
class JSImportScope;
class Function;
class SharedValueOwner;

class QMLJS_EXPORT ValueOwner
{
    Q_DISABLE_COPY(ValueOwner)

public:
    static SharedValueOwner *sharedValueOwner(QString kind = QString());
    ValueOwner(const SharedValueOwner *shared = nullptr);
    virtual ~ValueOwner();

    const NullValue *nullValue() const;
    const UndefinedValue *undefinedValue() const;
    const UnknownValue *unknownValue() const;
    const NumberValue *numberValue() const;
    const RealValue *realValue() const;
    const IntValue *intValue() const;
    const BooleanValue *booleanValue() const;
    const StringValue *stringValue() const;
    const UrlValue *urlValue() const;
    const ColorValue *colorValue() const;
    const AnchorLineValue *anchorLineValue() const;

    ObjectValue *newObject(const Value *prototype);
    ObjectValue *newObject();

    // QML objects
    const ObjectValue *qmlFontObject();
    const ObjectValue *qmlPaletteObject();
    const ObjectValue *qmlPointObject();
    const ObjectValue *qmlSizeObject();
    const ObjectValue *qmlRectObject();
    const ObjectValue *qmlVector2DObject();
    const ObjectValue *qmlVector3DObject();
    const ObjectValue *qmlVector4DObject();
    const ObjectValue *qmlQuaternionObject();
    const ObjectValue *qmlMatrix4x4Object();

    // converts builtin types, such as int, string to a Value
    const Value *defaultValueForBuiltinType(const QString &typeName) const;

    // global object
    const ObjectValue *globalObject() const;
    const ObjectValue *mathObject() const;
    const ObjectValue *qtObject() const;

    // prototypes
    const ObjectValue *objectPrototype() const;
    const ObjectValue *functionPrototype() const;
    const ObjectValue *numberPrototype() const;
    const ObjectValue *booleanPrototype() const;
    const ObjectValue *stringPrototype() const;
    const ObjectValue *arrayPrototype() const;
    const ObjectValue *datePrototype() const;
    const ObjectValue *regexpPrototype() const;

    // ctors
    const FunctionValue *objectCtor() const;
    const FunctionValue *functionCtor() const;
    const FunctionValue *arrayCtor() const;
    const FunctionValue *stringCtor() const;
    const FunctionValue *booleanCtor() const;
    const FunctionValue *numberCtor() const;
    const FunctionValue *dateCtor() const;
    const FunctionValue *regexpCtor() const;

    // operators
    const Value *convertToBoolean(const Value *value);
    const Value *convertToNumber(const Value *value);
    const Value *convertToString(const Value *value);
    const Value *convertToObject(const Value *value);
    QString typeId(const Value *value);

    // typing:
    CppQmlTypes &cppQmlTypes()
    { return _cppQmlTypes; }
    const CppQmlTypes &cppQmlTypes() const
    { return _cppQmlTypes; }

    void registerValue(Value *value); // internal

protected:
    Function *addFunction(ObjectValue *object, const QString &name, const Value *result,
                          int argumentCount = 0, int optionalCount = 0, bool variadic = false);
    Function *addFunction(ObjectValue *object, const QString &name,
                          int argumentCount = 0, int optionalCount = 0, bool variadic = false);

    QList<Value *> _registeredValues;
    QMutex _mutex;

    ConvertToNumber _convertToNumber;
    ConvertToString _convertToString;
    ConvertToObject _convertToObject;
    TypeId _typeId;
    CppQmlTypes _cppQmlTypes;

    const SharedValueOwner *_shared;
};

} // namespace QmlJS
