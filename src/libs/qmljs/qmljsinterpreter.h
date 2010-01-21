/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QMLJS_INTERPRETER_H
#define QMLJS_INTERPRETER_H

#include "qmljs_global.h"

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QHash>
#include <QtCore/QSet>

namespace QmlJS {
namespace Interpreter {

////////////////////////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////////////////////////
class Engine;
class Value;
class NullValue;
class UndefinedValue;
class NumberValue;
class BooleanValue;
class StringValue;
class ObjectValue;
class FunctionValue;

////////////////////////////////////////////////////////////////////////////////
// Value visitor
////////////////////////////////////////////////////////////////////////////////
class QMLJS_EXPORT ValueVisitor
{
public:
    ValueVisitor();
    virtual ~ValueVisitor();

    virtual void visit(const NullValue *);
    virtual void visit(const UndefinedValue *);
    virtual void visit(const NumberValue *);
    virtual void visit(const BooleanValue *);
    virtual void visit(const StringValue *);
    virtual void visit(const ObjectValue *);
    virtual void visit(const FunctionValue *);
};

////////////////////////////////////////////////////////////////////////////////
// QML/JS value
////////////////////////////////////////////////////////////////////////////////
class QMLJS_EXPORT Value
{
    Value(const Value &other);
    void operator = (const Value &other);

public:
    Value();
    virtual ~Value();

    virtual const NullValue *asNullValue() const;
    virtual const UndefinedValue *asUndefinedValue() const;
    virtual const NumberValue *asNumberValue() const;
    virtual const BooleanValue *asBooleanValue() const;
    virtual const StringValue *asStringValue() const;
    virtual const ObjectValue *asObjectValue() const;
    virtual const FunctionValue *asFunctionValue() const;

    virtual void accept(ValueVisitor *) const = 0;
};

template <typename _RetTy> _RetTy value_cast(const Value *v);

template <> Q_INLINE_TEMPLATE const NullValue *value_cast(const Value *v)
{
    if (v) return v->asNullValue();
    else   return 0;
}

template <> Q_INLINE_TEMPLATE const UndefinedValue *value_cast(const Value *v)
{
    if (v) return v->asUndefinedValue();
    else   return 0;
}

template <> Q_INLINE_TEMPLATE const NumberValue *value_cast(const Value *v)
{
    if (v) return v->asNumberValue();
    else   return 0;
}

template <> Q_INLINE_TEMPLATE const BooleanValue *value_cast(const Value *v)
{
    if (v) return v->asBooleanValue();
    else   return 0;
}

template <> Q_INLINE_TEMPLATE const StringValue *value_cast(const Value *v)
{
    if (v) return v->asStringValue();
    else   return 0;
}

template <> Q_INLINE_TEMPLATE const ObjectValue *value_cast(const Value *v)
{
    if (v) return v->asObjectValue();
    else   return 0;
}

template <> Q_INLINE_TEMPLATE const FunctionValue *value_cast(const Value *v)
{
    if (v) return v->asFunctionValue();
    else   return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Execution environment
////////////////////////////////////////////////////////////////////////////////
class QMLJS_EXPORT Environment
{
public:
    Environment();
    virtual ~Environment();

    virtual const Environment *parent() const;
    virtual const Value *lookup(const QString &name) const;
    virtual const Value *lookupMember(const QString &name) const;
};


////////////////////////////////////////////////////////////////////////////////
// Value nodes
////////////////////////////////////////////////////////////////////////////////
class QMLJS_EXPORT NullValue: public Value
{
public:
    virtual const NullValue *asNullValue() const;
    virtual void accept(ValueVisitor *visitor) const;
};

class QMLJS_EXPORT UndefinedValue: public Value
{
public:
    virtual const UndefinedValue *asUndefinedValue() const;
    virtual void accept(ValueVisitor *visitor) const;
};

class QMLJS_EXPORT NumberValue: public Value
{
public:
    virtual const NumberValue *asNumberValue() const;
    virtual void accept(ValueVisitor *visitor) const;
};

class QMLJS_EXPORT BooleanValue: public Value
{
public:
    virtual const BooleanValue *asBooleanValue() const;
    virtual void accept(ValueVisitor *visitor) const;
};

class QMLJS_EXPORT StringValue: public Value
{
public:
    virtual const StringValue *asStringValue() const;
    virtual void accept(ValueVisitor *visitor) const;
};

class QMLJS_EXPORT ObjectValue: public Value, public Environment
{
public:
    ObjectValue(Engine *engine);

    Engine *engine() const;

    QString className() const;
    void setClassName(const QString &className);

    const ObjectValue *prototype() const;
    void setPrototype(const ObjectValue *prototype);

    const ObjectValue *scope() const;
    void setScope(const ObjectValue *scope);

    typedef QHash<QString, const Value *>::const_iterator MemberIterator;

    MemberIterator firstMember() const;
    MemberIterator lastMember() const;

    virtual const Value *property(const QString &name) const;
    virtual void setProperty(const QString &name, const Value *value);
    virtual void removeProperty(const QString &name);
    virtual const Value *member(const QString &name) const;

    // Environment interface
    virtual const Environment *parent() const;
    virtual const Value *lookupMember(const QString &name) const;

    // Value interface
    virtual const ObjectValue *asObjectValue() const;
    virtual void accept(ValueVisitor *visitor) const;

private:
    bool isValidPrototype(const ObjectValue *prototype, QSet<const ObjectValue *> *processed) const;

private:
    Engine *_engine;
    const ObjectValue *_prototype;
    const ObjectValue *_scope;
    QHash<QString, const Value *> _members;
    QString _className;
};

class QMLJS_EXPORT FunctionValue: public ObjectValue
{
public:
    FunctionValue(Engine *engine);

    virtual const Value *returnValue() const;

    virtual int argumentCount() const;
    virtual const Value *argument(int /*index*/) const;

    virtual const Value *call(const Value *thisValue,
                              const QList<const Value *> & /*actuals*/ = QList<const Value *>()) const;

    // Value interface
    virtual const FunctionValue *asFunctionValue() const;
    virtual void accept(ValueVisitor *visitor) const;
};

class QMLJS_EXPORT Function: public FunctionValue
{
public:
    Function(Engine *engine);

    void addArgument(const Value *argument);

    typedef QList<const Value *>::const_iterator ArgumentIterator;

    ArgumentIterator firstArgument() const;
    ArgumentIterator lastArgument() const;

    void setReturnValue(const Value *returnValue);

    // ObjectValue interface
    virtual const Value *property(const QString &name) const;

    // FunctionValue interface
    virtual const Value *returnValue() const;
    virtual int argumentCount() const;
    virtual const Value *argument(int index) const;
    virtual const Value *call(const Value */*thisValue*/, const QList<const Value *> & /*actuals*/) const;

private:
    QList<const Value *> _arguments;
    const Value *_returnValue;
};


////////////////////////////////////////////////////////////////////////////////
// typing environment
////////////////////////////////////////////////////////////////////////////////

class ConvertToNumber: protected ValueVisitor // ECMAScript ToInt()
{
public:
    ConvertToNumber(Engine *engine);

    const Value *operator()(const Value *value);

protected:
    const Value *switchResult(const Value *value);

    virtual void visit(const NullValue *);
    virtual void visit(const UndefinedValue *);
    virtual void visit(const NumberValue *);
    virtual void visit(const BooleanValue *);
    virtual void visit(const StringValue *);
    virtual void visit(const ObjectValue *);
    virtual void visit(const FunctionValue *);

private:
    Engine *_engine;
    const Value *_result;
};

class ConvertToString: protected ValueVisitor // ECMAScript ToString
{
public:
    ConvertToString(Engine *engine);

    const Value *operator()(const Value *value);

protected:
    const Value *switchResult(const Value *value);

    virtual void visit(const NullValue *);
    virtual void visit(const UndefinedValue *);
    virtual void visit(const NumberValue *);
    virtual void visit(const BooleanValue *);
    virtual void visit(const StringValue *);
    virtual void visit(const ObjectValue *);
    virtual void visit(const FunctionValue *);

private:
    Engine *_engine;
    const Value *_result;
};

class ConvertToObject: protected ValueVisitor // ECMAScript ToObject
{
public:
    ConvertToObject(Engine *engine);

    const Value *operator()(const Value *value);

protected:
    const Value *switchResult(const Value *value);

    virtual void visit(const NullValue *);
    virtual void visit(const UndefinedValue *);
    virtual void visit(const NumberValue *);
    virtual void visit(const BooleanValue *);
    virtual void visit(const StringValue *);
    virtual void visit(const ObjectValue *);
    virtual void visit(const FunctionValue *);

private:
    Engine *_engine;
    const Value *_result;
};

class TypeId: protected ValueVisitor
{
    QString _result;

public:
    QString operator()(const Value *value);

protected:
    virtual void visit(const NullValue *);
    virtual void visit(const UndefinedValue *);
    virtual void visit(const NumberValue *);
    virtual void visit(const BooleanValue *);
    virtual void visit(const StringValue *);
    virtual void visit(const ObjectValue *object);
    virtual void visit(const FunctionValue *object);
};

class QMLJS_EXPORT Engine
{
    Engine(const Engine &other);
    void operator = (const Engine &other);

public:
    Engine();
    ~Engine();

    const NullValue *nullValue() const;
    const UndefinedValue *undefinedValue() const;
    const NumberValue *numberValue() const;
    const BooleanValue *booleanValue() const;
    const StringValue *stringValue() const;

    const Value *newArrayValue(); // ### remove me

    ObjectValue *newObject();
    Function *newFunction();

    // global object
    ObjectValue *globalObject() const;
    const ObjectValue *mathObject() const;

    // prototypes
    ObjectValue *objectPrototype() const;
    ObjectValue *functionPrototype() const;
    ObjectValue *numberPrototype() const;
    ObjectValue *booleanPrototype() const;
    ObjectValue *stringPrototype() const;
    ObjectValue *arrayPrototype() const;
    ObjectValue *datePrototype() const;
    ObjectValue *regexpPrototype() const;

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
    const Value *convertToBool(const Value *value);
    const Value *convertToNumber(const Value *value);
    const Value *convertToString(const Value *value);
    const Value *convertToObject(const Value *value);
    QString typeId(const Value *value);

    const Value *call(const FunctionValue *function, const QList<const Value *> &actuals = QList<const Value *>());
    const Value *call(const FunctionValue *function, const ObjectValue *thisValue,
                      const QList<const Value *> &actuals = QList<const Value *>());

private:
    void initializePrototypes();

    ObjectValue *newObject(const ObjectValue *prototype);

    void addFunction(ObjectValue *object, const QString &name, const Value *result, int argumentCount);
    void addFunction(ObjectValue *object, const QString &name, int argumentCount);

private:
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

    NullValue _nullValue;
    UndefinedValue _undefinedValue;
    NumberValue _numberValue;
    BooleanValue _booleanValue;
    StringValue _stringValue;
    QList<ObjectValue *> _objects;

    ConvertToNumber _convertToNumber;
    ConvertToString _convertToString;
    ConvertToObject _convertToObject;
    TypeId _typeId;
};

} } // end of namespace QmlJS::Interpreter

#endif // QMLJS_INTERPRETER_H
