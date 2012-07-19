/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "qmljsvalueowner.h"

#include "qmljscontext.h"

using namespace QmlJS;

/*!
    \class QmlJS::ValueOwner
    \brief Manages the lifetime of \l{QmlJS::Value}s.
    \sa QmlJS::Value

    Values are usually created on a ValueOwner. When the ValueOwner is destroyed
    it deletes all values it has registered.

    A ValueOwner also provides access to various default values.
*/

namespace {

class QtObjectPrototypeReference : public Reference
{
public:
    QtObjectPrototypeReference(ValueOwner *owner)
        : Reference(owner)
    {}

private:
    virtual const Value *value(ReferenceContext *referenceContext) const
    {
        return referenceContext->context()->valueOwner()->cppQmlTypes().objectByCppName(QLatin1String("Qt"));
    }
};

} // end of anonymous namespace


// globally shared data
class QmlJS::SharedValueOwner : public ValueOwner
{
public:
    SharedValueOwner();

    ObjectValue *_objectPrototype;
    ObjectValue *_functionPrototype;
    ObjectValue *_numberPrototype;
    ObjectValue *_booleanPrototype;
    ObjectValue *_stringPrototype;
    ObjectValue *_arrayPrototype;
    ObjectValue *_datePrototype;
    ObjectValue *_regexpPrototype;

    Function *_objectCtor;
    Function *_functionCtor;
    Function *_arrayCtor;
    Function *_stringCtor;
    Function *_booleanCtor;
    Function *_numberCtor;
    Function *_dateCtor;
    Function *_regexpCtor;

    ObjectValue *_globalObject;
    ObjectValue *_mathObject;
    ObjectValue *_qtObject;
    ObjectValue *_qmlKeysObject;
    ObjectValue *_qmlFontObject;
    ObjectValue *_qmlPointObject;
    ObjectValue *_qmlSizeObject;
    ObjectValue *_qmlRectObject;
    ObjectValue *_qmlVector3DObject;

    NullValue _nullValue;
    UndefinedValue _undefinedValue;
    UnknownValue _unknownValue;
    NumberValue _numberValue;
    RealValue _realValue;
    IntValue _intValue;
    BooleanValue _booleanValue;
    StringValue _stringValue;
    UrlValue _urlValue;
    ColorValue _colorValue;
    AnchorLineValue _anchorLineValue;
};
Q_GLOBAL_STATIC(SharedValueOwner, sharedValueOwner)

SharedValueOwner::SharedValueOwner()
    : ValueOwner(this) // need to avoid recursing in ValueOwner ctor
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

    ObjectValue *objectInstance = newObject();
    objectInstance->setClassName("Object");
    objectInstance->setMember("length", numberValue());
    _objectCtor = new Function(this);
    _objectCtor->setMember("prototype", _objectPrototype);
    _objectCtor->setReturnValue(objectInstance);
    _objectCtor->addArgument(unknownValue(), "value");
    _objectCtor->setOptionalNamedArgumentCount(1);

    FunctionValue *functionInstance = new FunctionValue(this);
    _functionCtor = new Function(this);
    _functionCtor->setMember("prototype", _functionPrototype);
    _functionCtor->setReturnValue(functionInstance);
    _functionCtor->setVariadic(true);

    ObjectValue *arrayInstance = newObject(_arrayPrototype);
    arrayInstance->setClassName("Array");
    arrayInstance->setMember("length", numberValue());
    _arrayCtor = new Function(this);
    _arrayCtor->setMember("prototype", _arrayPrototype);
    _arrayCtor->setReturnValue(arrayInstance);
    _arrayCtor->setVariadic(true);

    ObjectValue *stringInstance = newObject(_stringPrototype);
    stringInstance->setClassName("String");
    stringInstance->setMember("length", numberValue());
    _stringCtor = new Function(this);
    _stringCtor->setMember("prototype", _stringPrototype);
    _stringCtor->setReturnValue(stringInstance);
    _stringCtor->addArgument(unknownValue(), "value");
    _stringCtor->setOptionalNamedArgumentCount(1);

    ObjectValue *booleanInstance = newObject(_booleanPrototype);
    booleanInstance->setClassName("Boolean");
    _booleanCtor = new Function(this);
    _booleanCtor->setMember("prototype", _booleanPrototype);
    _booleanCtor->setReturnValue(booleanInstance);
    _booleanCtor->addArgument(unknownValue(), "value");

    ObjectValue *numberInstance = newObject(_numberPrototype);
    numberInstance->setClassName("Number");
    _numberCtor = new Function(this);
    _numberCtor->setMember("prototype", _numberPrototype);
    _numberCtor->setReturnValue(numberInstance);
    _numberCtor->addArgument(unknownValue(), "value");
    _numberCtor->setOptionalNamedArgumentCount(1);

    ObjectValue *dateInstance = newObject(_datePrototype);
    dateInstance->setClassName("Date");
    _dateCtor = new Function(this);
    _dateCtor->setMember("prototype", _datePrototype);
    _dateCtor->setReturnValue(dateInstance);
    _dateCtor->setVariadic(true);

    ObjectValue *regexpInstance = newObject(_regexpPrototype);
    regexpInstance->setClassName("RegExp");
    regexpInstance->setMember("source", stringValue());
    regexpInstance->setMember("global", booleanValue());
    regexpInstance->setMember("ignoreCase", booleanValue());
    regexpInstance->setMember("multiline", booleanValue());
    regexpInstance->setMember("lastIndex", numberValue());
    _regexpCtor = new Function(this);
    _regexpCtor->setMember("prototype", _regexpPrototype);
    _regexpCtor->setReturnValue(regexpInstance);
    _regexpCtor->addArgument(unknownValue(), "pattern");
    _regexpCtor->addArgument(unknownValue(), "flags");

    addFunction(_objectCtor, "getPrototypeOf", 1);
    addFunction(_objectCtor, "getOwnPropertyDescriptor", 2);
    addFunction(_objectCtor, "getOwnPropertyNames", arrayInstance, 1);
    addFunction(_objectCtor, "create", 1, 1);
    addFunction(_objectCtor, "defineProperty", 3);
    addFunction(_objectCtor, "defineProperties", 2);
    addFunction(_objectCtor, "seal", 1);
    addFunction(_objectCtor, "freeze", 1);
    addFunction(_objectCtor, "preventExtensions", 1);
    addFunction(_objectCtor, "isSealed", booleanValue(), 1);
    addFunction(_objectCtor, "isFrozen", booleanValue(), 1);
    addFunction(_objectCtor, "isExtensible", booleanValue(), 1);
    addFunction(_objectCtor, "keys", arrayInstance, 1);

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
    addFunction(_functionPrototype, "call", 1, 0, true);
    addFunction(_functionPrototype, "bind", 1, 0, true);

    // set up the default Array prototype
    addFunction(_arrayCtor, "isArray", booleanValue(), 1);

    _arrayPrototype->setMember("constructor", _arrayCtor);
    addFunction(_arrayPrototype, "toString", stringValue(), 0);
    addFunction(_arrayPrototype, "toLocalString", stringValue(), 0);
    addFunction(_arrayPrototype, "concat", 0, 0, true);
    addFunction(_arrayPrototype, "join", 1);
    addFunction(_arrayPrototype, "pop", 0);
    addFunction(_arrayPrototype, "push", 0, 0, true);
    addFunction(_arrayPrototype, "reverse", 0);
    addFunction(_arrayPrototype, "shift", 0);
    addFunction(_arrayPrototype, "slice", 2);
    addFunction(_arrayPrototype, "sort", 1);
    addFunction(_arrayPrototype, "splice", 2);
    addFunction(_arrayPrototype, "unshift", 0, 0, true);
    addFunction(_arrayPrototype, "indexOf", numberValue(), 2, 1);
    addFunction(_arrayPrototype, "lastIndexOf", numberValue(), 2, 1);
    addFunction(_arrayPrototype, "every", 2, 1);
    addFunction(_arrayPrototype, "some", 2, 1);
    addFunction(_arrayPrototype, "forEach", 2, 1);
    addFunction(_arrayPrototype, "map", 2, 1);
    addFunction(_arrayPrototype, "filter", 2, 1);
    addFunction(_arrayPrototype, "reduce", 2, 1);
    addFunction(_arrayPrototype, "reduceRight", 2, 1);

    // set up the default String prototype
    addFunction(_stringCtor, "fromCharCode", stringValue(), 0, 0, true);

    _stringPrototype->setMember("constructor", _stringCtor);
    addFunction(_stringPrototype, "toString", stringValue(), 0);
    addFunction(_stringPrototype, "valueOf", stringValue(), 0);
    addFunction(_stringPrototype, "charAt", stringValue(), 1);
    addFunction(_stringPrototype, "charCodeAt", stringValue(), 1);
    addFunction(_stringPrototype, "concat", stringValue(), 0, 0, true);
    addFunction(_stringPrototype, "indexOf", numberValue(), 2);
    addFunction(_stringPrototype, "lastIndexOf", numberValue(), 2);
    addFunction(_stringPrototype, "localeCompare", booleanValue(), 1);
    addFunction(_stringPrototype, "match", arrayInstance, 1);
    addFunction(_stringPrototype, "replace", stringValue(), 2);
    addFunction(_stringPrototype, "search", numberValue(), 1);
    addFunction(_stringPrototype, "slice", stringValue(), 2);
    addFunction(_stringPrototype, "split", arrayInstance, 1);
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
    addFunction(_numberPrototype, "toString", stringValue(), 1, 1);
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
    addFunction(_mathObject, "max", numberValue(), 0, 0, true);
    addFunction(_mathObject, "min", numberValue(), 0, 0, true);
    addFunction(_mathObject, "pow", numberValue(), 2);
    addFunction(_mathObject, "random", numberValue(), 1);
    addFunction(_mathObject, "round", numberValue(), 1);
    addFunction(_mathObject, "sin", numberValue(), 1);
    addFunction(_mathObject, "sqrt", numberValue(), 1);
    addFunction(_mathObject, "tan", numberValue(), 1);

    // set up the default Boolean prototype
    addFunction(_dateCtor, "parse", numberValue(), 1);
    addFunction(_dateCtor, "UTC", numberValue(), 7, 5);
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
    addFunction(_datePrototype, "setSeconds", 2, 1);
    addFunction(_datePrototype, "setUTCSeconds", 2, 1);
    addFunction(_datePrototype, "setMinutes", 3, 2);
    addFunction(_datePrototype, "setUTCMinutes", 3, 2);
    addFunction(_datePrototype, "setHours", 4, 3);
    addFunction(_datePrototype, "setUTCHours", 4, 3);
    addFunction(_datePrototype, "setDate", 1);
    addFunction(_datePrototype, "setUTCDate", 1);
    addFunction(_datePrototype, "setMonth", 2, 1);
    addFunction(_datePrototype, "setUTCMonth", 2, 1);
    addFunction(_datePrototype, "setFullYear", 3, 2);
    addFunction(_datePrototype, "setUTCFullYear", 3, 2);
    addFunction(_datePrototype, "toUTCString", stringValue(), 0);
    addFunction(_datePrototype, "toISOString", stringValue(), 0);
    addFunction(_datePrototype, "toJSON", stringValue(), 1);

    // set up the default Boolean prototype
    _regexpPrototype->setMember("constructor", _regexpCtor);
    addFunction(_regexpPrototype, "exec", arrayInstance, 1);
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
    f->addArgument(unknownValue(), "data");
    f = addFunction(xmlHttpRequest, "abort");
    xmlHttpRequest->setMember("status", numberValue());
    xmlHttpRequest->setMember("statusText", stringValue());
    f = addFunction(xmlHttpRequest, "getResponseHeader");
    f->addArgument(stringValue(), "header");
    f = addFunction(xmlHttpRequest, "getAllResponseHeaders");
    xmlHttpRequest->setMember("responseText", stringValue());
    xmlHttpRequest->setMember("responseXML", unknownValue());

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
    f->setOptionalNamedArgumentCount(1);
    f = addFunction(json, "stringify", stringValue());
    f->addArgument(unknownValue(), "value");
    f->addArgument(unknownValue(), "replacer");
    f->addArgument(unknownValue(), "space");
    f->setOptionalNamedArgumentCount(2);
    _globalObject->setMember("JSON", json);

    // QML objects
    _qmlFontObject = newObject(/*prototype =*/ 0);
    _qmlFontObject->setClassName(QLatin1String("Font"));
    _qmlFontObject->setMember("family", stringValue());
    _qmlFontObject->setMember("weight", unknownValue()); // ### make me an object
    _qmlFontObject->setMember("capitalization", unknownValue()); // ### make me an object
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

    // global Qt object, in alphabetic order
    _qtObject = newObject(new QtObjectPrototypeReference(this));
    addFunction(_qtObject, QLatin1String("atob"), &_stringValue, 1);
    addFunction(_qtObject, QLatin1String("btoa"), &_stringValue, 1);
    addFunction(_qtObject, QLatin1String("createComponent"), 1);
    addFunction(_qtObject, QLatin1String("createQmlObject"), 3);
    addFunction(_qtObject, QLatin1String("darker"), &_colorValue, 1);
    addFunction(_qtObject, QLatin1String("fontFamilies"), 0);
    addFunction(_qtObject, QLatin1String("formatDate"), &_stringValue, 2);
    addFunction(_qtObject, QLatin1String("formatDateTime"), &_stringValue, 2);
    addFunction(_qtObject, QLatin1String("formatTime"), &_stringValue, 2);
    addFunction(_qtObject, QLatin1String("hsla"), &_colorValue, 4);
    addFunction(_qtObject, QLatin1String("include"), 2);
    addFunction(_qtObject, QLatin1String("isQtObject"), &_booleanValue, 1);
    addFunction(_qtObject, QLatin1String("lighter"), &_colorValue, 1);
    addFunction(_qtObject, QLatin1String("md5"), &_stringValue, 1);
    addFunction(_qtObject, QLatin1String("openUrlExternally"), &_booleanValue, 1);
    addFunction(_qtObject, QLatin1String("point"), _qmlPointObject, 2);
    addFunction(_qtObject, QLatin1String("quit"), 0);
    addFunction(_qtObject, QLatin1String("rect"), _qmlRectObject, 4);
    addFunction(_qtObject, QLatin1String("resolvedUrl"), &_urlValue, 1);
    addFunction(_qtObject, QLatin1String("rgba"), &_colorValue, 4);
    addFunction(_qtObject, QLatin1String("size"), _qmlSizeObject, 2);
    addFunction(_qtObject, QLatin1String("tint"), &_colorValue, 2);
    addFunction(_qtObject, QLatin1String("vector3d"), _qmlVector3DObject, 3);
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
}


ValueOwner::ValueOwner(const SharedValueOwner *shared)
    : _convertToNumber(this)
    , _convertToString(this)
    , _convertToObject(this)
    , _cppQmlTypes(this)
{
    if (shared)
        _shared = shared;
    else
        _shared = sharedValueOwner();
}

ValueOwner::~ValueOwner()
{
    qDeleteAll(_registeredValues);
}

const NullValue *ValueOwner::nullValue() const
{
    return &_shared->_nullValue;
}

const UndefinedValue *ValueOwner::undefinedValue() const
{
    return &_shared->_undefinedValue;
}

const UnknownValue *ValueOwner::unknownValue() const
{
    return &_shared->_unknownValue;
}

const NumberValue *ValueOwner::numberValue() const
{
    return &_shared->_numberValue;
}

const RealValue *ValueOwner::realValue() const
{
    return &_shared->_realValue;
}

const IntValue *ValueOwner::intValue() const
{
    return &_shared->_intValue;
}

const BooleanValue *ValueOwner::booleanValue() const
{
    return &_shared->_booleanValue;
}

const StringValue *ValueOwner::stringValue() const
{
    return &_shared->_stringValue;
}

const UrlValue *ValueOwner::urlValue() const
{
    return &_shared->_urlValue;
}

const ColorValue *ValueOwner::colorValue() const
{
    return &_shared->_colorValue;
}

const AnchorLineValue *ValueOwner::anchorLineValue() const
{
    return &_shared->_anchorLineValue;
}

ObjectValue *ValueOwner::newObject()
{
    return newObject(_shared->_objectPrototype);
}

ObjectValue *ValueOwner::newObject(const Value *prototype)
{
    ObjectValue *object = new ObjectValue(this);
    object->setPrototype(prototype);
    return object;
}

const ObjectValue *ValueOwner::globalObject() const
{
    return _shared->_globalObject;
}

const ObjectValue *ValueOwner::objectPrototype() const
{
    return _shared->_objectPrototype;
}

const ObjectValue *ValueOwner::functionPrototype() const
{
    return _shared->_functionPrototype;
}

const ObjectValue *ValueOwner::numberPrototype() const
{
    return _shared->_numberPrototype;
}

const ObjectValue *ValueOwner::booleanPrototype() const
{
    return _shared->_booleanPrototype;
}

const ObjectValue *ValueOwner::stringPrototype() const
{
    return _shared->_stringPrototype;
}

const ObjectValue *ValueOwner::arrayPrototype() const
{
    return _shared->_arrayPrototype;
}

const ObjectValue *ValueOwner::datePrototype() const
{
    return _shared->_datePrototype;
}

const ObjectValue *ValueOwner::regexpPrototype() const
{
    return _shared->_regexpPrototype;
}

const FunctionValue *ValueOwner::objectCtor() const
{
    return _shared->_objectCtor;
}

const FunctionValue *ValueOwner::functionCtor() const
{
    return _shared->_functionCtor;
}

const FunctionValue *ValueOwner::arrayCtor() const
{
    return _shared->_arrayCtor;
}

const FunctionValue *ValueOwner::stringCtor() const
{
    return _shared->_stringCtor;
}

const FunctionValue *ValueOwner::booleanCtor() const
{
    return _shared->_booleanCtor;
}

const FunctionValue *ValueOwner::numberCtor() const
{
    return _shared->_numberCtor;
}

const FunctionValue *ValueOwner::dateCtor() const
{
    return _shared->_dateCtor;
}

const FunctionValue *ValueOwner::regexpCtor() const
{
    return _shared->_regexpCtor;
}

const ObjectValue *ValueOwner::mathObject() const
{
    return _shared->_mathObject;
}

const ObjectValue *ValueOwner::qtObject() const
{
    return _shared->_qtObject;
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

Function *ValueOwner::addFunction(ObjectValue *object, const QString &name, const Value *result, int argumentCount, int optionalCount, bool variadic)
{
    Function *function = addFunction(object, name, argumentCount, optionalCount, variadic);
    function->setReturnValue(result);
    return function;
}

Function *ValueOwner::addFunction(ObjectValue *object, const QString &name, int argumentCount, int optionalCount, bool variadic)
{
    Function *function = new Function(this);
    for (int i = 0; i < argumentCount; ++i)
        function->addArgument(unknownValue());
    function->setVariadic(variadic);
    function->setOptionalNamedArgumentCount(optionalCount);
    object->setMember(name, function);
    return function;
}

const ObjectValue *ValueOwner::qmlKeysObject()
{
    return _shared->_qmlKeysObject;
}

const ObjectValue *ValueOwner::qmlFontObject()
{
    return _shared->_qmlFontObject;
}

const ObjectValue *ValueOwner::qmlPointObject()
{
    return _shared->_qmlPointObject;
}

const ObjectValue *ValueOwner::qmlSizeObject()
{
    return _shared->_qmlSizeObject;
}

const ObjectValue *ValueOwner::qmlRectObject()
{
    return _shared->_qmlRectObject;
}

const ObjectValue *ValueOwner::qmlVector3DObject()
{
    return _shared->_qmlVector3DObject;
}

const Value *ValueOwner::defaultValueForBuiltinType(const QString &name) const
{
    // this list is defined in ProcessAST::visit(UiPublicMember) in qdeclarativescript.cpp
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
    } else if (name == QLatin1String("var")
               || name == QLatin1String("variant")) {
        return unknownValue();
    }
    return undefinedValue();
}
