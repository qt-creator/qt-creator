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

#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljs_global.h>
#include <qmljs/qmljsmetatypesystem.h>
#include <qmljs/parser/qmljsastfwd_p.h>

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QHash>
#include <QtCore/QSet>

namespace QmlJS {

class NameId;
class Document;

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
class Reference;
class EasingCurveNameValue;

typedef QList<const Value *> ValueList;

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
    virtual void visit(const Reference *);
    virtual void visit(const EasingCurveNameValue *);
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
    virtual const Reference *asReference() const;
    virtual const EasingCurveNameValue *asEasingCurveNameValue() const;

    virtual void accept(ValueVisitor *) const = 0;

    virtual bool getSourceLocation(QString *fileName, int *line, int *column) const;
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

template <> Q_INLINE_TEMPLATE const Reference *value_cast(const Value *v)
{
    if (v) return v->asReference();
    else   return 0;
}

template <> Q_INLINE_TEMPLATE const EasingCurveNameValue *value_cast(const Value *v)
{
    if (v) return v->asEasingCurveNameValue();
    else   return 0;
}

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

class QMLJS_EXPORT MemberProcessor
{
    MemberProcessor(const MemberProcessor &other);
    void operator = (const MemberProcessor &other);

public:
    MemberProcessor();
    virtual ~MemberProcessor();

    // Returns false to stop the processor.
    virtual bool processProperty(const QString &name, const Value *value);
    virtual bool processEnumerator(const QString &name, const Value *value);
    virtual bool processSignal(const QString &name, const Value *value);
    virtual bool processSlot(const QString &name, const Value *value);
    virtual bool processGeneratedSlot(const QString &name, const Value *value);
};

class QMLJS_EXPORT Context
{
public:
    typedef QList<const ObjectValue *> ScopeChain;

    enum LookupMode {
        JSLookup,
        QmlLookup
    };

public:
    Context(Engine *engine);
    ~Context();

    void build(AST::Node *node, const Document::Ptr doc, const Snapshot &snapshot);

    Engine *engine() const;
    ScopeChain scopeChain() const;

    LookupMode lookupMode() const;
    void setLookupMode(LookupMode lookupMode);

    const ObjectValue *typeEnvironment(const Document *doc) const;
    void setTypeEnvironment(const Document *doc, const ObjectValue *typeEnvironment);

    void pushScope(const ObjectValue *object);
    void popScope();

    void markQmlScopeObject();
    void setQmlScopeObject(const ObjectValue *scopeObject);
    const ObjectValue *qmlScopeObject() const;

    const Value *lookup(const QString &name);
    const ObjectValue *lookupType(const Document *doc, AST::UiQualifiedId *qmlTypeName);

    const Value *property(const ObjectValue *object, const QString &name) const;
    void setProperty(const ObjectValue *object, const QString &name, const Value *value);

private:
    typedef QHash<QString, const Value *> Properties;

    Engine *_engine;
    LookupMode _lookupMode;
    QHash<const ObjectValue *, Properties> _properties;
    QHash<const Document *, const ObjectValue *> _typeEnvironments;
    ScopeChain _scopeChain;
    int _qmlScopeObjectIndex;
    bool _qmlScopeObjectSet;
};

class QMLJS_EXPORT Reference: public Value
{
public:
    Reference(Engine *engine);
    virtual ~Reference();

    Engine *engine() const;
    virtual const Value *value(Context *context) const;

    // Value interface
    virtual const Reference *asReference() const;
    virtual void accept(ValueVisitor *) const;

private:
    Engine *_engine;
};

class QMLJS_EXPORT EasingCurveNameValue: public Value
{
    static QSet<QString> _curveNames;

public:
    static QSet<QString> curveNames();

    // Value interface
    virtual const EasingCurveNameValue *asEasingCurveNameValue() const;
    virtual void accept(ValueVisitor *) const;
};

class QMLJS_EXPORT ObjectValue: public Value
{
public:
    ObjectValue(Engine *engine);
    virtual ~ObjectValue();

    Engine *engine() const;

    QString className() const;
    void setClassName(const QString &className);

    const ObjectValue *prototype(Context *context) const;
    void setPrototype(const Value *prototype);

    virtual void processMembers(MemberProcessor *processor) const;

    virtual const Value *property(const QString &name, Context *context) const;
    virtual void setProperty(const QString &name, const Value *value);
    virtual void removeProperty(const QString &name);

    virtual const Value *lookupMember(const QString &name, Context *context) const;

    // Value interface
    virtual const ObjectValue *asObjectValue() const;
    virtual void accept(ValueVisitor *visitor) const;

private:
    bool checkPrototype(const ObjectValue *prototype, QSet<const ObjectValue *> *processed) const;

private:
    Engine *_engine;
    const Value *_prototype;
    QHash<QString, const Value *> _members;
    QString _className;
};

#ifndef NO_DECLARATIVE_BACKEND

class QmlObjectValue: public ObjectValue
{
public:
    QmlObjectValue(const QMetaObject *metaObject, const QString &qmlTypeName, int majorVersion, int minorVersion, Engine *engine);
    virtual ~QmlObjectValue();

    virtual const Value *lookupMember(const QString &name, Context *context) const;
    virtual void processMembers(MemberProcessor *processor) const;
    const Value *propertyValue(const QMetaProperty &prop) const;

    QString qmlTypeName() const
    { return _qmlTypeName; }

    int majorVersion() const
    { return _majorVersion; }

    int minorVersion() const
    { return _minorVersion; }

protected:
    const Value *findOrCreateSignature(int index, const QMetaMethod &method, QString *methodName) const;
    bool isDerivedFrom(const QMetaObject *base) const;

private:
    const QMetaObject *_metaObject;
    QString _qmlTypeName;
    int _majorVersion;
    int _minorVersion;
    mutable QHash<int, const Value *> _metaSignature;
};

#endif // !NO_DECLARATIVE_BACKEND

class QMLJS_EXPORT Activation
{
public:
    explicit Activation(Context *parentContext = 0);
    virtual ~Activation();

    Context *context() const;
    Context *parentContext() const;

    bool calledAsConstructor() const;
    void setCalledAsConstructor(bool calledAsConstructor);

    bool calledAsFunction() const;
    void setCalledAsFunction(bool calledAsFunction);

    ObjectValue *thisObject() const;
    void setThisObject(ObjectValue *thisObject);

    ValueList arguments() const;
    void setArguments(const ValueList &arguments);

private:
    ObjectValue *_thisObject;
    ValueList _arguments;
    bool _calledAsFunction;
    Context *_parentContext;
};


class QMLJS_EXPORT FunctionValue: public ObjectValue
{
public:
    FunctionValue(Engine *engine);
    virtual ~FunctionValue();

    // [[construct]]
    const Value *construct(const ValueList &actuals = ValueList()) const;

    // [[call]]
    const Value *call(const ValueList &actuals = ValueList()) const;

    const Value *call(const ObjectValue *thisObject,
                      const ValueList &actuals = ValueList()) const;


    virtual const Value *returnValue() const;

    virtual int argumentCount() const;
    virtual const Value *argument(int index) const;
    virtual QString argumentName(int index) const;
    virtual bool isVariadic() const;

    virtual const Value *invoke(const Activation *activation) const;

    // Value interface
    virtual const FunctionValue *asFunctionValue() const;
    virtual void accept(ValueVisitor *visitor) const;
};

class QMLJS_EXPORT Function: public FunctionValue
{
public:
    Function(Engine *engine);
    virtual ~Function();

    void addArgument(const Value *argument);
    void setReturnValue(const Value *returnValue);

    // ObjectValue interface
    virtual const Value *property(const QString &name, Context *context) const;

    // FunctionValue interface
    virtual const Value *returnValue() const;
    virtual int argumentCount() const;
    virtual const Value *argument(int index) const;
    virtual const Value *invoke(const Activation *activation) const;

private:
    ValueList _arguments;
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
    const EasingCurveNameValue *easingCurveNameValue() const;

    ObjectValue *newObject(const ObjectValue *prototype);
    ObjectValue *newObject();
    Function *newFunction();
    const Value *newArray(); // ### remove me

    // QML objects
    const ObjectValue *qmlKeysObject();
    const Value *defaultValueForBuiltinType(const QString &typeName) const;
#ifndef NO_DECLARATIVE_BACKEND
    QmlObjectValue *newQmlObject(const QString &name, const QString &prefix, int majorVersion, int minorVersion);
#endif

    // global object
    ObjectValue *globalObject() const;
    const ObjectValue *mathObject() const;
    const ObjectValue *qtObject() const;

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
    const Value *convertToBoolean(const Value *value);
    const Value *convertToNumber(const Value *value);
    const Value *convertToString(const Value *value);
    const Value *convertToObject(const Value *value);
    QString typeId(const Value *value);

    // typing:
    const MetaTypeSystem &metaTypeSystem() const
    { return _metaTypeSystem; }

    void registerValue(Value *value); // internal

private:
    void initializePrototypes();

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
    ObjectValue *_qtObject;
#ifndef NO_DECLARATIVE_BACKEND
    ObjectValue *_qmlKeysObject;
#endif

    NullValue _nullValue;
    UndefinedValue _undefinedValue;
    NumberValue _numberValue;
    BooleanValue _booleanValue;
    StringValue _stringValue;
    EasingCurveNameValue _easingCurveNameValue;
    QList<Value *> _registeredValues;

    ConvertToNumber _convertToNumber;
    ConvertToString _convertToString;
    ConvertToObject _convertToObject;
    TypeId _typeId;

    MetaTypeSystem _metaTypeSystem;
};


// internal
class QMLJS_EXPORT QmlPrototypeReference: public Reference
{
public:
    QmlPrototypeReference(AST::UiQualifiedId *qmlTypeName, const Document *doc, Engine *engine);
    virtual ~QmlPrototypeReference();

    AST::UiQualifiedId *qmlTypeName() const;

    virtual const Value *value(Context *context) const;

private:
    AST::UiQualifiedId *_qmlTypeName;
    const Document *_doc;
};

class QMLJS_EXPORT ASTVariableReference: public Reference
{
    AST::VariableDeclaration *_ast;

public:
    ASTVariableReference(AST::VariableDeclaration *ast, Engine *engine);
    virtual ~ASTVariableReference();

    virtual const Value *value(Context *context) const;
};

class QMLJS_EXPORT ASTFunctionValue: public FunctionValue
{
    AST::FunctionDeclaration *_ast;
    QList<NameId *> _argumentNames;

public:
    ASTFunctionValue(AST::FunctionDeclaration *ast, Engine *engine);
    virtual ~ASTFunctionValue();

    AST::FunctionDeclaration *ast() const;

    virtual const Value *returnValue() const;
    virtual int argumentCount() const;
    virtual const Value *argument(int) const;
    virtual QString argumentName(int index) const;
    virtual bool isVariadic() const;
};

class QMLJS_EXPORT ASTPropertyReference: public Reference
{
    AST::UiPublicMember *_ast;
    const Document *_doc;
    QString _onChangedSlotName;

public:
    ASTPropertyReference(AST::UiPublicMember *ast, const Document *doc, Engine *engine);
    virtual ~ASTPropertyReference();

    AST::UiPublicMember *ast() const { return _ast; }
    QString onChangedSlotName() const { return _onChangedSlotName; }

    virtual bool getSourceLocation(QString *fileName, int *line, int *column) const;
    virtual const Value *value(Context *context) const;
};

class QMLJS_EXPORT ASTSignalReference: public Reference
{
    AST::UiPublicMember *_ast;
    const Document *_doc;
    QString _slotName;

public:
    ASTSignalReference(AST::UiPublicMember *ast, const Document *doc, Engine *engine);
    virtual ~ASTSignalReference();

    AST::UiPublicMember *ast() const { return _ast; }
    QString slotName() const { return _slotName; }

    virtual bool getSourceLocation(QString *fileName, int *line, int *column) const;
    virtual const Value *value(Context *context) const;
};

class QMLJS_EXPORT ASTObjectValue: public ObjectValue
{
    AST::UiQualifiedId *_typeName;
    AST::UiObjectInitializer *_initializer;
    const Document *_doc;
    QList<ASTPropertyReference *> _properties;
    QList<ASTSignalReference *> _signals;

public:
    ASTObjectValue(AST::UiQualifiedId *typeName,
                   AST::UiObjectInitializer *initializer,
                   const Document *doc,
                   Engine *engine);
    virtual ~ASTObjectValue();

    bool getSourceLocation(QString *fileName, int *line, int *column) const;
    virtual void processMembers(MemberProcessor *processor) const;
};

} } // end of namespace QmlJS::Interpreter

#endif // QMLJS_INTERPRETER_H
