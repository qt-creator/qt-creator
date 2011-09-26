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

#include "qmljsvalueowner.h"

using namespace QmlJS;

namespace {

////////////////////////////////////////////////////////////////////////////////
// constructors
////////////////////////////////////////////////////////////////////////////////
class ObjectCtor: public Function
{
public:
    ObjectCtor(ValueOwner *valueOwner);

    virtual const Value *invoke(const Activation *activation) const;
};

class FunctionCtor: public Function
{
public:
    FunctionCtor(ValueOwner *valueOwner);

    virtual const Value *invoke(const Activation *activation) const;
};

class ArrayCtor: public Function
{
public:
    ArrayCtor(ValueOwner *valueOwner);

    virtual const Value *invoke(const Activation *activation) const;
};

class StringCtor: public Function
{
public:
    StringCtor(ValueOwner *valueOwner);

    virtual const Value *invoke(const Activation *activation) const;
};

class BooleanCtor: public Function
{
public:
    BooleanCtor(ValueOwner *valueOwner);

    virtual const Value *invoke(const Activation *activation) const;
};

class NumberCtor: public Function
{
public:
    NumberCtor(ValueOwner *valueOwner);

    virtual const Value *invoke(const Activation *activation) const;
};

class DateCtor: public Function
{
public:
    DateCtor(ValueOwner *valueOwner);

    virtual const Value *invoke(const Activation *activation) const;
};

class RegExpCtor: public Function
{
public:
    RegExpCtor(ValueOwner *valueOwner);

    virtual const Value *invoke(const Activation *activation) const;
};

ObjectCtor::ObjectCtor(ValueOwner *valueOwner)
    : Function(valueOwner)
{
}

FunctionCtor::FunctionCtor(ValueOwner *valueOwner)
    : Function(valueOwner)
{
}

ArrayCtor::ArrayCtor(ValueOwner *valueOwner)
    : Function(valueOwner)
{
}

StringCtor::StringCtor(ValueOwner *valueOwner)
    : Function(valueOwner)
{
}

BooleanCtor::BooleanCtor(ValueOwner *valueOwner)
    : Function(valueOwner)
{
}

NumberCtor::NumberCtor(ValueOwner *valueOwner)
    : Function(valueOwner)
{
}

DateCtor::DateCtor(ValueOwner *valueOwner)
    : Function(valueOwner)
{
}

RegExpCtor::RegExpCtor(ValueOwner *valueOwner)
    : Function(valueOwner)
{
}

const Value *ObjectCtor::invoke(const Activation *activation) const
{
    ObjectValue *thisObject = activation->thisObject();
    if (activation->calledAsFunction())
        thisObject = valueOwner()->newObject();

    thisObject->setClassName("Object");
    thisObject->setPrototype(valueOwner()->objectPrototype());
    thisObject->setMember("length", valueOwner()->numberValue());
    return thisObject;
}

const Value *FunctionCtor::invoke(const Activation *activation) const
{
    ObjectValue *thisObject = activation->thisObject();
    if (activation->calledAsFunction())
        thisObject = valueOwner()->newObject();

    thisObject->setClassName("Function");
    thisObject->setPrototype(valueOwner()->functionPrototype());
    thisObject->setMember("length", valueOwner()->numberValue());
    return thisObject;
}

const Value *ArrayCtor::invoke(const Activation *activation) const
{
    ObjectValue *thisObject = activation->thisObject();
    if (activation->calledAsFunction())
        thisObject = valueOwner()->newObject();

    thisObject->setClassName("Array");
    thisObject->setPrototype(valueOwner()->arrayPrototype());
    thisObject->setMember("length", valueOwner()->numberValue());
    return thisObject;
}

const Value *StringCtor::invoke(const Activation *activation) const
{
    if (activation->calledAsFunction())
        return valueOwner()->convertToString(activation->thisObject());

    ObjectValue *thisObject = activation->thisObject();
    thisObject->setClassName("String");
    thisObject->setPrototype(valueOwner()->stringPrototype());
    thisObject->setMember("length", valueOwner()->numberValue());
    return thisObject;
}

const Value *BooleanCtor::invoke(const Activation *activation) const
{
    if (activation->calledAsFunction())
        return valueOwner()->convertToBoolean(activation->thisObject());

    ObjectValue *thisObject = activation->thisObject();
    thisObject->setClassName("Boolean");
    thisObject->setPrototype(valueOwner()->booleanPrototype());
    return thisObject;
}

const Value *NumberCtor::invoke(const Activation *activation) const
{
    if (activation->calledAsFunction())
        return valueOwner()->convertToNumber(activation->thisObject());

    ObjectValue *thisObject = activation->thisObject();
    thisObject->setClassName("Number");
    thisObject->setPrototype(valueOwner()->numberPrototype());
    return thisObject;
}

const Value *DateCtor::invoke(const Activation *activation) const
{
    if (activation->calledAsFunction())
        return valueOwner()->stringValue();

    ObjectValue *thisObject = activation->thisObject();
    thisObject->setClassName("Date");
    thisObject->setPrototype(valueOwner()->datePrototype());
    return thisObject;
}

const Value *RegExpCtor::invoke(const Activation *activation) const
{
    ObjectValue *thisObject = activation->thisObject();
    if (activation->calledAsFunction())
        thisObject = valueOwner()->newObject();

    thisObject->setClassName("RegExp");
    thisObject->setPrototype(valueOwner()->regexpPrototype());
    thisObject->setMember("source", valueOwner()->stringValue());
    thisObject->setMember("global", valueOwner()->booleanValue());
    thisObject->setMember("ignoreCase", valueOwner()->booleanValue());
    thisObject->setMember("multiline", valueOwner()->booleanValue());
    thisObject->setMember("lastIndex", valueOwner()->numberValue());
    return thisObject;
}

} // end of anonymous namespace


ValueOwner::ValueOwner()
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
      _convertToObject(this),
      _cppQmlTypes(this)
{
    initializePrototypes();
}

ValueOwner::~ValueOwner()
{
    qDeleteAll(_registeredValues);
}

const NullValue *ValueOwner::nullValue() const
{
    return &_nullValue;
}

const UndefinedValue *ValueOwner::undefinedValue() const
{
    return &_undefinedValue;
}

const NumberValue *ValueOwner::numberValue() const
{
    return &_numberValue;
}

const RealValue *ValueOwner::realValue() const
{
    return &_realValue;
}

const IntValue *ValueOwner::intValue() const
{
    return &_intValue;
}

const BooleanValue *ValueOwner::booleanValue() const
{
    return &_booleanValue;
}

const StringValue *ValueOwner::stringValue() const
{
    return &_stringValue;
}

const UrlValue *ValueOwner::urlValue() const
{
    return &_urlValue;
}

const ColorValue *ValueOwner::colorValue() const
{
    return &_colorValue;
}

const AnchorLineValue *ValueOwner::anchorLineValue() const
{
    return &_anchorLineValue;
}

const Value *ValueOwner::newArray()
{
    return arrayCtor()->construct();
}

ObjectValue *ValueOwner::newObject()
{
    return newObject(_objectPrototype);
}

ObjectValue *ValueOwner::newObject(const ObjectValue *prototype)
{
    ObjectValue *object = new ObjectValue(this);
    object->setPrototype(prototype);
    return object;
}

Function *ValueOwner::newFunction()
{
    Function *function = new Function(this);
    function->setPrototype(functionPrototype());
    return function;
}

ObjectValue *ValueOwner::globalObject() const
{
    return _globalObject;
}

ObjectValue *ValueOwner::objectPrototype() const
{
    return _objectPrototype;
}

ObjectValue *ValueOwner::functionPrototype() const
{
    return _functionPrototype;
}

ObjectValue *ValueOwner::numberPrototype() const
{
    return _numberPrototype;
}

ObjectValue *ValueOwner::booleanPrototype() const
{
    return _booleanPrototype;
}

ObjectValue *ValueOwner::stringPrototype() const
{
    return _stringPrototype;
}

ObjectValue *ValueOwner::arrayPrototype() const
{
    return _arrayPrototype;
}

ObjectValue *ValueOwner::datePrototype() const
{
    return _datePrototype;
}

ObjectValue *ValueOwner::regexpPrototype() const
{
    return _regexpPrototype;
}

const FunctionValue *ValueOwner::objectCtor() const
{
    return _objectCtor;
}

const FunctionValue *ValueOwner::functionCtor() const
{
    return _functionCtor;
}

const FunctionValue *ValueOwner::arrayCtor() const
{
    return _arrayCtor;
}

const FunctionValue *ValueOwner::stringCtor() const
{
    return _stringCtor;
}

const FunctionValue *ValueOwner::booleanCtor() const
{
    return _booleanCtor;
}

const FunctionValue *ValueOwner::numberCtor() const
{
    return _numberCtor;
}

const FunctionValue *ValueOwner::dateCtor() const
{
    return _dateCtor;
}

const FunctionValue *ValueOwner::regexpCtor() const
{
    return _regexpCtor;
}

const ObjectValue *ValueOwner::mathObject() const
{
    return _mathObject;
}

const ObjectValue *ValueOwner::qtObject() const
{
    return _qtObject;
}

void ValueOwner::registerValue(Value *value)
{
    // ### get rid of this lock
    QMutexLocker locker(&_mutex);
    _registeredValues.append(value);
}

const Value *ValueOwner::convertToBoolean(const Value *value)
{
    return _convertToNumber(value);  // ### implement convert to bool
}

const Value *ValueOwner::convertToNumber(const Value *value)
{
    return _convertToNumber(value);
}

const Value *ValueOwner::convertToString(const Value *value)
{
    return _convertToString(value);
}

const Value *ValueOwner::convertToObject(const Value *value)
{
    return _convertToObject(value);
}

QString ValueOwner::typeId(const Value *value)
{
    return _typeId(value);
}

Function *ValueOwner::addFunction(ObjectValue *object, const QString &name, const Value *result, int argumentCount)
{
    Function *function = newFunction();
    function->setReturnValue(result);
    for (int i = 0; i < argumentCount; ++i)
        function->addArgument(undefinedValue()); // ### introduce unknownValue
    object->setMember(name, function);
    return function;
}

Function *ValueOwner::addFunction(ObjectValue *object, const QString &name, int argumentCount)
{
    Function *function = newFunction();
    for (int i = 0; i < argumentCount; ++i)
        function->addArgument(undefinedValue()); // ### introduce unknownValue
    object->setMember(name, function);
    return function;
}

void ValueOwner::initializePrototypes()
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

    Function *f = 0;

    // XMLHttpRequest
    ObjectValue *xmlHttpRequest = newObject();
    xmlHttpRequest->setMember("onreadystatechange", functionPrototype());
    xmlHttpRequest->setMember("UNSENT", numberValue());
    xmlHttpRequest->setMember("OPENED", numberValue());
    xmlHttpRequest->setMember("HEADERS_RECEIVED", numberValue());
    xmlHttpRequest->setMember("LOADING", numberValue());
    xmlHttpRequest->setMember("DONE", numberValue());
    xmlHttpRequest->setMember("readyState", numberValue());
    f = addFunction(xmlHttpRequest, "open");
    f->addArgument(stringValue(), "method");
    f->addArgument(stringValue(), "url");
    f->addArgument(booleanValue(), "async");
    f->addArgument(stringValue(), "user");
    f->addArgument(stringValue(), "password");
    f = addFunction(xmlHttpRequest, "setRequestHeader");
    f->addArgument(stringValue(), "header");
    f->addArgument(stringValue(), "value");
    f = addFunction(xmlHttpRequest, "send");
    f->addArgument(undefinedValue(), "data");
    f = addFunction(xmlHttpRequest, "abort");
    xmlHttpRequest->setMember("status", numberValue());
    xmlHttpRequest->setMember("statusText", stringValue());
    f = addFunction(xmlHttpRequest, "getResponseHeader");
    f->addArgument(stringValue(), "header");
    f = addFunction(xmlHttpRequest, "getAllResponseHeaders");
    xmlHttpRequest->setMember("responseText", stringValue());
    xmlHttpRequest->setMember("responseXML", undefinedValue());

    f = addFunction(_globalObject, "XMLHttpRequest", xmlHttpRequest);
    f->setMember("prototype", xmlHttpRequest);
    xmlHttpRequest->setMember("constructor", f);

    // Database API
    ObjectValue *db = newObject();
    f = addFunction(db, "transaction");
    f->addArgument(functionPrototype(), "callback");
    f = addFunction(db, "readTransaction");
    f->addArgument(functionPrototype(), "callback");
    f->setMember("version", stringValue());
    f = addFunction(db, "changeVersion");
    f->addArgument(stringValue(), "oldVersion");
    f->addArgument(stringValue(), "newVersion");
    f->addArgument(functionPrototype(), "callback");

    f = addFunction(_globalObject, "openDatabaseSync", db);
    f->addArgument(stringValue(), "name");
    f->addArgument(stringValue(), "version");
    f->addArgument(stringValue(), "displayName");
    f->addArgument(numberValue(), "estimatedSize");
    f->addArgument(functionPrototype(), "callback");

    // JSON
    ObjectValue *json = newObject();
    f = addFunction(json, "parse", objectPrototype());
    f->addArgument(stringValue(), "text");
    f->addArgument(functionPrototype(), "reviver");
    f = addFunction(json, "stringify", stringValue());
    f->addArgument(undefinedValue(), "value");
    f->addArgument(undefinedValue(), "replacer");
    f->addArgument(undefinedValue(), "space");
    _globalObject->setMember("JSON", json);

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

const ObjectValue *ValueOwner::qmlKeysObject()
{
    return _qmlKeysObject;
}

const ObjectValue *ValueOwner::qmlFontObject()
{
    return _qmlFontObject;
}

const ObjectValue *ValueOwner::qmlPointObject()
{
    return _qmlPointObject;
}

const ObjectValue *ValueOwner::qmlSizeObject()
{
    return _qmlSizeObject;
}

const ObjectValue *ValueOwner::qmlRectObject()
{
    return _qmlRectObject;
}

const ObjectValue *ValueOwner::qmlVector3DObject()
{
    return _qmlVector3DObject;
}

const Value *ValueOwner::defaultValueForBuiltinType(const QString &name) const
{
    if (name == QLatin1String("int")) {
            return intValue();
    } else if (name == QLatin1String("bool")) {
        return booleanValue();
    }  else if (name == QLatin1String("double")
                || name == QLatin1String("real")) {
        return realValue();
    } else if (name == QLatin1String("string")) {
        return stringValue();
    } else if (name == QLatin1String("url")) {
        return urlValue();
    } else if (name == QLatin1String("color")) {
        return colorValue();
    } else if (name == QLatin1String("date")) {
        return datePrototype();
    }
    // ### variant
    return undefinedValue();
}
