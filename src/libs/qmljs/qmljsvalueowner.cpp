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
    _globalObject->setClassName(QLatin1String("Global"));

    ObjectValue *objectInstance = newObject();
    objectInstance->setClassName(QLatin1String("Object"));
    objectInstance->setMember(QLatin1String("length"), numberValue());
    _objectCtor = new Function(this);
    _objectCtor->setMember(QLatin1String("prototype"), _objectPrototype);
    _objectCtor->setReturnValue(objectInstance);
    _objectCtor->addArgument(unknownValue(), QLatin1String("value"));
    _objectCtor->setOptionalNamedArgumentCount(1);

    FunctionValue *functionInstance = new FunctionValue(this);
    _functionCtor = new Function(this);
    _functionCtor->setMember(QLatin1String("prototype"), _functionPrototype);
    _functionCtor->setReturnValue(functionInstance);
    _functionCtor->setVariadic(true);

    ObjectValue *arrayInstance = newObject(_arrayPrototype);
    arrayInstance->setClassName(QLatin1String("Array"));
    arrayInstance->setMember(QLatin1String("length"), numberValue());
    _arrayCtor = new Function(this);
    _arrayCtor->setMember(QLatin1String("prototype"), _arrayPrototype);
    _arrayCtor->setReturnValue(arrayInstance);
    _arrayCtor->setVariadic(true);

    ObjectValue *stringInstance = newObject(_stringPrototype);
    stringInstance->setClassName(QLatin1String("String"));
    stringInstance->setMember(QLatin1String("length"), numberValue());
    _stringCtor = new Function(this);
    _stringCtor->setMember(QLatin1String("prototype"), _stringPrototype);
    _stringCtor->setReturnValue(stringInstance);
    _stringCtor->addArgument(unknownValue(), QLatin1String("value"));
    _stringCtor->setOptionalNamedArgumentCount(1);

    ObjectValue *booleanInstance = newObject(_booleanPrototype);
    booleanInstance->setClassName(QLatin1String("Boolean"));
    _booleanCtor = new Function(this);
    _booleanCtor->setMember(QLatin1String("prototype"), _booleanPrototype);
    _booleanCtor->setReturnValue(booleanInstance);
    _booleanCtor->addArgument(unknownValue(), QLatin1String("value"));

    ObjectValue *numberInstance = newObject(_numberPrototype);
    numberInstance->setClassName(QLatin1String("Number"));
    _numberCtor = new Function(this);
    _numberCtor->setMember(QLatin1String("prototype"), _numberPrototype);
    _numberCtor->setReturnValue(numberInstance);
    _numberCtor->addArgument(unknownValue(), QLatin1String("value"));
    _numberCtor->setOptionalNamedArgumentCount(1);

    ObjectValue *dateInstance = newObject(_datePrototype);
    dateInstance->setClassName(QLatin1String("Date"));
    _dateCtor = new Function(this);
    _dateCtor->setMember(QLatin1String("prototype"), _datePrototype);
    _dateCtor->setReturnValue(dateInstance);
    _dateCtor->setVariadic(true);

    ObjectValue *regexpInstance = newObject(_regexpPrototype);
    regexpInstance->setClassName(QLatin1String("RegExp"));
    regexpInstance->setMember(QLatin1String("source"), stringValue());
    regexpInstance->setMember(QLatin1String("global"), booleanValue());
    regexpInstance->setMember(QLatin1String("ignoreCase"), booleanValue());
    regexpInstance->setMember(QLatin1String("multiline"), booleanValue());
    regexpInstance->setMember(QLatin1String("lastIndex"), numberValue());
    _regexpCtor = new Function(this);
    _regexpCtor->setMember(QLatin1String("prototype"), _regexpPrototype);
    _regexpCtor->setReturnValue(regexpInstance);
    _regexpCtor->addArgument(unknownValue(), QLatin1String("pattern"));
    _regexpCtor->addArgument(unknownValue(), QLatin1String("flags"));

    addFunction(_objectCtor, QLatin1String("getPrototypeOf"), 1);
    addFunction(_objectCtor, QLatin1String("getOwnPropertyDescriptor"), 2);
    addFunction(_objectCtor, QLatin1String("getOwnPropertyNames"), arrayInstance, 1);
    addFunction(_objectCtor, QLatin1String("create"), 1, 1);
    addFunction(_objectCtor, QLatin1String("defineProperty"), 3);
    addFunction(_objectCtor, QLatin1String("defineProperties"), 2);
    addFunction(_objectCtor, QLatin1String("seal"), 1);
    addFunction(_objectCtor, QLatin1String("freeze"), 1);
    addFunction(_objectCtor, QLatin1String("preventExtensions"), 1);
    addFunction(_objectCtor, QLatin1String("isSealed"), booleanValue(), 1);
    addFunction(_objectCtor, QLatin1String("isFrozen"), booleanValue(), 1);
    addFunction(_objectCtor, QLatin1String("isExtensible"), booleanValue(), 1);
    addFunction(_objectCtor, QLatin1String("keys"), arrayInstance, 1);

    addFunction(_objectPrototype, QLatin1String("toString"), stringValue(), 0);
    addFunction(_objectPrototype, QLatin1String("toLocaleString"), stringValue(), 0);
    addFunction(_objectPrototype, QLatin1String("valueOf"), 0); // ### FIXME it should return thisObject
    addFunction(_objectPrototype, QLatin1String("hasOwnProperty"), booleanValue(), 1);
    addFunction(_objectPrototype, QLatin1String("isPrototypeOf"), booleanValue(), 1);
    addFunction(_objectPrototype, QLatin1String("propertyIsEnumerable"), booleanValue(), 1);

    // set up the default Function prototype
    _functionPrototype->setMember(QLatin1String("constructor"), _functionCtor);
    addFunction(_functionPrototype, QLatin1String("toString"), stringValue(), 0);
    addFunction(_functionPrototype, QLatin1String("apply"), 2);
    addFunction(_functionPrototype, QLatin1String("call"), 1, 0, true);
    addFunction(_functionPrototype, QLatin1String("bind"), 1, 0, true);

    // set up the default Array prototype
    addFunction(_arrayCtor, QLatin1String("isArray"), booleanValue(), 1);

    _arrayPrototype->setMember(QLatin1String("constructor"), _arrayCtor);
    addFunction(_arrayPrototype, QLatin1String("toString"), stringValue(), 0);
    addFunction(_arrayPrototype, QLatin1String("toLocalString"), stringValue(), 0);
    addFunction(_arrayPrototype, QLatin1String("concat"), 0, 0, true);
    addFunction(_arrayPrototype, QLatin1String("join"), 1);
    addFunction(_arrayPrototype, QLatin1String("pop"), 0);
    addFunction(_arrayPrototype, QLatin1String("push"), 0, 0, true);
    addFunction(_arrayPrototype, QLatin1String("reverse"), 0);
    addFunction(_arrayPrototype, QLatin1String("shift"), 0);
    addFunction(_arrayPrototype, QLatin1String("slice"), 2);
    addFunction(_arrayPrototype, QLatin1String("sort"), 1);
    addFunction(_arrayPrototype, QLatin1String("splice"), 2);
    addFunction(_arrayPrototype, QLatin1String("unshift"), 0, 0, true);
    addFunction(_arrayPrototype, QLatin1String("indexOf"), numberValue(), 2, 1);
    addFunction(_arrayPrototype, QLatin1String("lastIndexOf"), numberValue(), 2, 1);
    addFunction(_arrayPrototype, QLatin1String("every"), 2, 1);
    addFunction(_arrayPrototype, QLatin1String("some"), 2, 1);
    addFunction(_arrayPrototype, QLatin1String("forEach"), 2, 1);
    addFunction(_arrayPrototype, QLatin1String("map"), 2, 1);
    addFunction(_arrayPrototype, QLatin1String("filter"), 2, 1);
    addFunction(_arrayPrototype, QLatin1String("reduce"), 2, 1);
    addFunction(_arrayPrototype, QLatin1String("reduceRight"), 2, 1);

    // set up the default String prototype
    addFunction(_stringCtor, QLatin1String("fromCharCode"), stringValue(), 0, 0, true);

    _stringPrototype->setMember(QLatin1String("constructor"), _stringCtor);
    addFunction(_stringPrototype, QLatin1String("toString"), stringValue(), 0);
    addFunction(_stringPrototype, QLatin1String("valueOf"), stringValue(), 0);
    addFunction(_stringPrototype, QLatin1String("charAt"), stringValue(), 1);
    addFunction(_stringPrototype, QLatin1String("charCodeAt"), stringValue(), 1);
    addFunction(_stringPrototype, QLatin1String("concat"), stringValue(), 0, 0, true);
    addFunction(_stringPrototype, QLatin1String("indexOf"), numberValue(), 2);
    addFunction(_stringPrototype, QLatin1String("lastIndexOf"), numberValue(), 2);
    addFunction(_stringPrototype, QLatin1String("localeCompare"), booleanValue(), 1);
    addFunction(_stringPrototype, QLatin1String("match"), arrayInstance, 1);
    addFunction(_stringPrototype, QLatin1String("replace"), stringValue(), 2);
    addFunction(_stringPrototype, QLatin1String("search"), numberValue(), 1);
    addFunction(_stringPrototype, QLatin1String("slice"), stringValue(), 2);
    addFunction(_stringPrototype, QLatin1String("split"), arrayInstance, 1);
    addFunction(_stringPrototype, QLatin1String("substring"), stringValue(), 2);
    addFunction(_stringPrototype, QLatin1String("toLowerCase"), stringValue(), 0);
    addFunction(_stringPrototype, QLatin1String("toLocaleLowerCase"), stringValue(), 0);
    addFunction(_stringPrototype, QLatin1String("toUpperCase"), stringValue(), 0);
    addFunction(_stringPrototype, QLatin1String("toLocaleUpperCase"), stringValue(), 0);
    addFunction(_stringPrototype, QLatin1String("trim"), stringValue(), 0);

    // set up the default Boolean prototype
    addFunction(_booleanCtor, QLatin1String("fromCharCode"), 0);

    _booleanPrototype->setMember(QLatin1String("constructor"), _booleanCtor);
    addFunction(_booleanPrototype, QLatin1String("toString"), stringValue(), 0);
    addFunction(_booleanPrototype, QLatin1String("valueOf"), booleanValue(), 0);

    // set up the default Number prototype
    _numberCtor->setMember(QLatin1String("MAX_VALUE"), numberValue());
    _numberCtor->setMember(QLatin1String("MIN_VALUE"), numberValue());
    _numberCtor->setMember(QLatin1String("NaN"), numberValue());
    _numberCtor->setMember(QLatin1String("NEGATIVE_INFINITY"), numberValue());
    _numberCtor->setMember(QLatin1String("POSITIVE_INFINITY"), numberValue());

    addFunction(_numberCtor, QLatin1String("fromCharCode"), 0);

    _numberPrototype->setMember(QLatin1String("constructor"), _numberCtor);
    addFunction(_numberPrototype, QLatin1String("toString"), stringValue(), 1, 1);
    addFunction(_numberPrototype, QLatin1String("toLocaleString"), stringValue(), 0);
    addFunction(_numberPrototype, QLatin1String("valueOf"), numberValue(), 0);
    addFunction(_numberPrototype, QLatin1String("toFixed"), numberValue(), 1);
    addFunction(_numberPrototype, QLatin1String("toExponential"), numberValue(), 1);
    addFunction(_numberPrototype, QLatin1String("toPrecision"), numberValue(), 1);

    // set up the Math object
    _mathObject = newObject();
    _mathObject->setMember(QLatin1String("E"), numberValue());
    _mathObject->setMember(QLatin1String("LN10"), numberValue());
    _mathObject->setMember(QLatin1String("LN2"), numberValue());
    _mathObject->setMember(QLatin1String("LOG2E"), numberValue());
    _mathObject->setMember(QLatin1String("LOG10E"), numberValue());
    _mathObject->setMember(QLatin1String("PI"), numberValue());
    _mathObject->setMember(QLatin1String("SQRT1_2"), numberValue());
    _mathObject->setMember(QLatin1String("SQRT2"), numberValue());

    addFunction(_mathObject, QLatin1String("abs"), numberValue(), 1);
    addFunction(_mathObject, QLatin1String("acos"), numberValue(), 1);
    addFunction(_mathObject, QLatin1String("asin"), numberValue(), 1);
    addFunction(_mathObject, QLatin1String("atan"), numberValue(), 1);
    addFunction(_mathObject, QLatin1String("atan2"), numberValue(), 2);
    addFunction(_mathObject, QLatin1String("ceil"), numberValue(), 1);
    addFunction(_mathObject, QLatin1String("cos"), numberValue(), 1);
    addFunction(_mathObject, QLatin1String("exp"), numberValue(), 1);
    addFunction(_mathObject, QLatin1String("floor"), numberValue(), 1);
    addFunction(_mathObject, QLatin1String("log"), numberValue(), 1);
    addFunction(_mathObject, QLatin1String("max"), numberValue(), 0, 0, true);
    addFunction(_mathObject, QLatin1String("min"), numberValue(), 0, 0, true);
    addFunction(_mathObject, QLatin1String("pow"), numberValue(), 2);
    addFunction(_mathObject, QLatin1String("random"), numberValue(), 1);
    addFunction(_mathObject, QLatin1String("round"), numberValue(), 1);
    addFunction(_mathObject, QLatin1String("sin"), numberValue(), 1);
    addFunction(_mathObject, QLatin1String("sqrt"), numberValue(), 1);
    addFunction(_mathObject, QLatin1String("tan"), numberValue(), 1);

    // set up the default Boolean prototype
    addFunction(_dateCtor, QLatin1String("parse"), numberValue(), 1);
    addFunction(_dateCtor, QLatin1String("UTC"), numberValue(), 7, 5);
    addFunction(_dateCtor, QLatin1String("now"), numberValue(), 0);

    _datePrototype->setMember(QLatin1String("constructor"), _dateCtor);
    addFunction(_datePrototype, QLatin1String("toString"), stringValue(), 0);
    addFunction(_datePrototype, QLatin1String("toDateString"), stringValue(), 0);
    addFunction(_datePrototype, QLatin1String("toTimeString"), stringValue(), 0);
    addFunction(_datePrototype, QLatin1String("toLocaleString"), stringValue(), 0);
    addFunction(_datePrototype, QLatin1String("toLocaleDateString"), stringValue(), 0);
    addFunction(_datePrototype, QLatin1String("toLocaleTimeString"), stringValue(), 0);
    addFunction(_datePrototype, QLatin1String("valueOf"), numberValue(), 0);
    addFunction(_datePrototype, QLatin1String("getTime"), numberValue(), 0);
    addFunction(_datePrototype, QLatin1String("getFullYear"), numberValue(), 0);
    addFunction(_datePrototype, QLatin1String("getUTCFullYear"), numberValue(), 0);
    addFunction(_datePrototype, QLatin1String("getMonth"), numberValue(), 0);
    addFunction(_datePrototype, QLatin1String("getUTCMonth"), numberValue(), 0);
    addFunction(_datePrototype, QLatin1String("getDate"), numberValue(), 0);
    addFunction(_datePrototype, QLatin1String("getUTCDate"), numberValue(), 0);
    addFunction(_datePrototype, QLatin1String("getHours"), numberValue(), 0);
    addFunction(_datePrototype, QLatin1String("getUTCHours"), numberValue(), 0);
    addFunction(_datePrototype, QLatin1String("getMinutes"), numberValue(), 0);
    addFunction(_datePrototype, QLatin1String("getUTCMinutes"), numberValue(), 0);
    addFunction(_datePrototype, QLatin1String("getSeconds"), numberValue(), 0);
    addFunction(_datePrototype, QLatin1String("getUTCSeconds"), numberValue(), 0);
    addFunction(_datePrototype, QLatin1String("getMilliseconds"), numberValue(), 0);
    addFunction(_datePrototype, QLatin1String("getUTCMilliseconds"), numberValue(), 0);
    addFunction(_datePrototype, QLatin1String("getTimezoneOffset"), numberValue(), 0);
    addFunction(_datePrototype, QLatin1String("setTime"), 1);
    addFunction(_datePrototype, QLatin1String("setMilliseconds"), 1);
    addFunction(_datePrototype, QLatin1String("setUTCMilliseconds"), 1);
    addFunction(_datePrototype, QLatin1String("setSeconds"), 2, 1);
    addFunction(_datePrototype, QLatin1String("setUTCSeconds"), 2, 1);
    addFunction(_datePrototype, QLatin1String("setMinutes"), 3, 2);
    addFunction(_datePrototype, QLatin1String("setUTCMinutes"), 3, 2);
    addFunction(_datePrototype, QLatin1String("setHours"), 4, 3);
    addFunction(_datePrototype, QLatin1String("setUTCHours"), 4, 3);
    addFunction(_datePrototype, QLatin1String("setDate"), 1);
    addFunction(_datePrototype, QLatin1String("setUTCDate"), 1);
    addFunction(_datePrototype, QLatin1String("setMonth"), 2, 1);
    addFunction(_datePrototype, QLatin1String("setUTCMonth"), 2, 1);
    addFunction(_datePrototype, QLatin1String("setFullYear"), 3, 2);
    addFunction(_datePrototype, QLatin1String("setUTCFullYear"), 3, 2);
    addFunction(_datePrototype, QLatin1String("toUTCString"), stringValue(), 0);
    addFunction(_datePrototype, QLatin1String("toISOString"), stringValue(), 0);
    addFunction(_datePrototype, QLatin1String("toJSON"), stringValue(), 1);

    // set up the default Boolean prototype
    _regexpPrototype->setMember(QLatin1String("constructor"), _regexpCtor);
    addFunction(_regexpPrototype, QLatin1String("exec"), arrayInstance, 1);
    addFunction(_regexpPrototype, QLatin1String("test"), booleanValue(), 1);
    addFunction(_regexpPrototype, QLatin1String("toString"), stringValue(), 0);

    // fill the Global object
    _globalObject->setMember(QLatin1String("Math"), _mathObject);
    _globalObject->setMember(QLatin1String("Object"), objectCtor());
    _globalObject->setMember(QLatin1String("Function"), functionCtor());
    _globalObject->setMember(QLatin1String("Array"), arrayCtor());
    _globalObject->setMember(QLatin1String("String"), stringCtor());
    _globalObject->setMember(QLatin1String("Boolean"), booleanCtor());
    _globalObject->setMember(QLatin1String("Number"), numberCtor());
    _globalObject->setMember(QLatin1String("Date"), dateCtor());
    _globalObject->setMember(QLatin1String("RegExp"), regexpCtor());

    Function *f = 0;

    // XMLHttpRequest
    ObjectValue *xmlHttpRequest = newObject();
    xmlHttpRequest->setMember(QLatin1String("onreadystatechange"), functionPrototype());
    xmlHttpRequest->setMember(QLatin1String("UNSENT"), numberValue());
    xmlHttpRequest->setMember(QLatin1String("OPENED"), numberValue());
    xmlHttpRequest->setMember(QLatin1String("HEADERS_RECEIVED"), numberValue());
    xmlHttpRequest->setMember(QLatin1String("LOADING"), numberValue());
    xmlHttpRequest->setMember(QLatin1String("DONE"), numberValue());
    xmlHttpRequest->setMember(QLatin1String("readyState"), numberValue());
    f = addFunction(xmlHttpRequest, QLatin1String("open"));
    f->addArgument(stringValue(), QLatin1String("method"));
    f->addArgument(stringValue(), QLatin1String("url"));
    f->addArgument(booleanValue(), QLatin1String("async"));
    f->addArgument(stringValue(), QLatin1String("user"));
    f->addArgument(stringValue(), QLatin1String("password"));
    f = addFunction(xmlHttpRequest, QLatin1String("setRequestHeader"));
    f->addArgument(stringValue(), QLatin1String("header"));
    f->addArgument(stringValue(), QLatin1String("value"));
    f = addFunction(xmlHttpRequest, QLatin1String("send"));
    f->addArgument(unknownValue(), QLatin1String("data"));
    f = addFunction(xmlHttpRequest, QLatin1String("abort"));
    xmlHttpRequest->setMember(QLatin1String("status"), numberValue());
    xmlHttpRequest->setMember(QLatin1String("statusText"), stringValue());
    f = addFunction(xmlHttpRequest, QLatin1String("getResponseHeader"));
    f->addArgument(stringValue(), QLatin1String("header"));
    f = addFunction(xmlHttpRequest, QLatin1String("getAllResponseHeaders"));
    xmlHttpRequest->setMember(QLatin1String("responseText"), stringValue());
    xmlHttpRequest->setMember(QLatin1String("responseXML"), unknownValue());

    f = addFunction(_globalObject, QLatin1String("XMLHttpRequest"), xmlHttpRequest);
    f->setMember(QLatin1String("prototype"), xmlHttpRequest);
    xmlHttpRequest->setMember(QLatin1String("constructor"), f);

    // Database API
    ObjectValue *db = newObject();
    f = addFunction(db, QLatin1String("transaction"));
    f->addArgument(functionPrototype(), QLatin1String("callback"));
    f = addFunction(db, QLatin1String("readTransaction"));
    f->addArgument(functionPrototype(), QLatin1String("callback"));
    f->setMember(QLatin1String("version"), stringValue());
    f = addFunction(db, QLatin1String("changeVersion"));
    f->addArgument(stringValue(), QLatin1String("oldVersion"));
    f->addArgument(stringValue(), QLatin1String("newVersion"));
    f->addArgument(functionPrototype(), QLatin1String("callback"));

    f = addFunction(_globalObject, QLatin1String("openDatabaseSync"), db);
    f->addArgument(stringValue(), QLatin1String("name"));
    f->addArgument(stringValue(), QLatin1String("version"));
    f->addArgument(stringValue(), QLatin1String("displayName"));
    f->addArgument(numberValue(), QLatin1String("estimatedSize"));
    f->addArgument(functionPrototype(), QLatin1String("callback"));

    // JSON
    ObjectValue *json = newObject();
    f = addFunction(json, QLatin1String("parse"), objectPrototype());
    f->addArgument(stringValue(), QLatin1String("text"));
    f->addArgument(functionPrototype(), QLatin1String("reviver"));
    f->setOptionalNamedArgumentCount(1);
    f = addFunction(json, QLatin1String("stringify"), stringValue());
    f->addArgument(unknownValue(), QLatin1String("value"));
    f->addArgument(unknownValue(), QLatin1String("replacer"));
    f->addArgument(unknownValue(), QLatin1String("space"));
    f->setOptionalNamedArgumentCount(2);
    _globalObject->setMember(QLatin1String("JSON"), json);

    // QML objects
    _qmlFontObject = newObject(/*prototype =*/ 0);
    _qmlFontObject->setClassName(QLatin1String("Font"));
    _qmlFontObject->setMember(QLatin1String("family"), stringValue());
    _qmlFontObject->setMember(QLatin1String("weight"), unknownValue()); // ### make me an object
    _qmlFontObject->setMember(QLatin1String("capitalization"), unknownValue()); // ### make me an object
    _qmlFontObject->setMember(QLatin1String("bold"), booleanValue());
    _qmlFontObject->setMember(QLatin1String("italic"), booleanValue());
    _qmlFontObject->setMember(QLatin1String("underline"), booleanValue());
    _qmlFontObject->setMember(QLatin1String("overline"), booleanValue());
    _qmlFontObject->setMember(QLatin1String("strikeout"), booleanValue());
    _qmlFontObject->setMember(QLatin1String("pointSize"), intValue());
    _qmlFontObject->setMember(QLatin1String("pixelSize"), intValue());
    _qmlFontObject->setMember(QLatin1String("letterSpacing"), realValue());
    _qmlFontObject->setMember(QLatin1String("wordSpacing"), realValue());

    _qmlPointObject = newObject(/*prototype =*/ 0);
    _qmlPointObject->setClassName(QLatin1String("Point"));
    _qmlPointObject->setMember(QLatin1String("x"), numberValue());
    _qmlPointObject->setMember(QLatin1String("y"), numberValue());

    _qmlSizeObject = newObject(/*prototype =*/ 0);
    _qmlSizeObject->setClassName(QLatin1String("Size"));
    _qmlSizeObject->setMember(QLatin1String("width"), numberValue());
    _qmlSizeObject->setMember(QLatin1String("height"), numberValue());

    _qmlRectObject = newObject(/*prototype =*/ 0);
    _qmlRectObject->setClassName(QLatin1String("Rect"));
    _qmlRectObject->setMember(QLatin1String("x"), numberValue());
    _qmlRectObject->setMember(QLatin1String("y"), numberValue());
    _qmlRectObject->setMember(QLatin1String("width"), numberValue());
    _qmlRectObject->setMember(QLatin1String("height"), numberValue());

    _qmlVector3DObject = newObject(/*prototype =*/ 0);
    _qmlVector3DObject->setClassName(QLatin1String("Vector3D"));
    _qmlVector3DObject->setMember(QLatin1String("x"), realValue());
    _qmlVector3DObject->setMember(QLatin1String("y"), realValue());
    _qmlVector3DObject->setMember(QLatin1String("z"), realValue());

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
