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

#ifndef QMLJS_VALUEOWNER_H
#define QMLJS_VALUEOWNER_H

#include "qmljs_global.h"
#include "qmljsinterpreter.h"

#include <QList>

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
    ValueOwner(const SharedValueOwner *shared = 0);
    ~ValueOwner();

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
    const ObjectValue *qmlKeysObject();
    const ObjectValue *qmlFontObject();
    const ObjectValue *qmlPointObject();
    const ObjectValue *qmlSizeObject();
    const ObjectValue *qmlRectObject();
    const ObjectValue *qmlVector3DObject();

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

#endif // QMLJS_VALUEOWNER_H
