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

#ifndef QMLJS_INTERPRETER_H
#define QMLJS_INTERPRETER_H

#include <languageutils/componentversion.h>
#include <languageutils/fakemetaobject.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljs_global.h>
#include <qmljs/parser/qmljsastfwd_p.h>

#include <QtCore/QFileInfoList>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QHash>
#include <QtCore/QSet>
#include <QtCore/QMutex>

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
    virtual void visit(const ColorValue *);
    virtual void visit(const AnchorLineValue *);
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
    virtual const IntValue *asIntValue() const;
    virtual const RealValue *asRealValue() const;
    virtual const BooleanValue *asBooleanValue() const;
    virtual const StringValue *asStringValue() const;
    virtual const UrlValue *asUrlValue() const;
    virtual const ObjectValue *asObjectValue() const;
    virtual const FunctionValue *asFunctionValue() const;
    virtual const Reference *asReference() const;
    virtual const ColorValue *asColorValue() const;
    virtual const AnchorLineValue *asAnchorLineValue() const;

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

template <> Q_INLINE_TEMPLATE const IntValue *value_cast(const Value *v)
{
    if (v) return v->asIntValue();
    else   return 0;
}

template <> Q_INLINE_TEMPLATE const RealValue *value_cast(const Value *v)
{
    if (v) return v->asRealValue();
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

template <> Q_INLINE_TEMPLATE const UrlValue *value_cast(const Value *v)
{
    if (v) return v->asUrlValue();
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

template <> Q_INLINE_TEMPLATE const ColorValue *value_cast(const Value *v)
{
    if (v) return v->asColorValue();
    else   return 0;
}

template <> Q_INLINE_TEMPLATE const AnchorLineValue *value_cast(const Value *v)
{
    if (v) return v->asAnchorLineValue();
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

class QMLJS_EXPORT RealValue: public NumberValue
{
public:
    virtual const RealValue *asRealValue() const;
};

class QMLJS_EXPORT IntValue: public NumberValue
{
public:
    virtual const IntValue *asIntValue() const;
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

class QMLJS_EXPORT UrlValue: public StringValue
{
public:
    virtual const UrlValue *asUrlValue() const;
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

class QMLJS_EXPORT ScopeChain
{
public:
    ScopeChain();

    class QMLJS_EXPORT QmlComponentChain
    {
        Q_DISABLE_COPY(QmlComponentChain)
    public:
        QmlComponentChain();
        ~QmlComponentChain();

        QList<const QmlComponentChain *> instantiatingComponents;
        Document::Ptr document;

        void collect(QList<const ObjectValue *> *list) const;
        void clear();
    };

    const ObjectValue *globalScope;
    QSharedPointer<const QmlComponentChain> qmlComponentScope;
    QList<const ObjectValue *> qmlScopeObjects;
    const TypeScope *qmlTypes;
    const JSImportScope *jsImports;
    QList<const ObjectValue *> jsScopes;

    // rebuilds the flat list of all scopes
    void update();
    QList<const ObjectValue *> all() const;

private:
    QList<const ObjectValue *> _all;
};

class QMLJS_EXPORT Context
{
public:
    Context(const Snapshot &snapshot);
    ~Context();

    Engine *engine() const;
    Snapshot snapshot() const;

    const ScopeChain &scopeChain() const;
    ScopeChain &scopeChain();

    const Imports *imports(const Document *doc) const;
    void setImports(const Document *doc, const Imports *imports);

    const Value *lookup(const QString &name, const ObjectValue **foundInScope = 0) const;
    const ObjectValue *lookupType(const Document *doc, AST::UiQualifiedId *qmlTypeName,
                                  AST::UiQualifiedId *qmlTypeNameEnd = 0) const;
    const ObjectValue *lookupType(const Document *doc, const QStringList &qmlTypeName) const;
    const Value *lookupReference(const Value *value) const;

    const Value *property(const ObjectValue *object, const QString &name) const;
    void setProperty(const ObjectValue *object, const QString &name, const Value *value);

    QString defaultPropertyName(const ObjectValue *object) const;

private:
    typedef QHash<QString, const Value *> Properties;

    Snapshot _snapshot;
    QSharedPointer<Engine> _engine;
    QHash<const ObjectValue *, Properties> _properties;
    QHash<const Document *, QSharedPointer<const Imports> > _imports;
    ScopeChain _scopeChain;
    int _qmlScopeObjectIndex;
    bool _qmlScopeObjectSet;

    // for avoiding reference cycles during lookup
    mutable QList<const Reference *> _referenceStack;
};

class QMLJS_EXPORT Reference: public Value
{
public:
    Reference(Engine *engine);
    virtual ~Reference();

    Engine *engine() const;

    // Value interface
    virtual const Reference *asReference() const;
    virtual void accept(ValueVisitor *) const;

private:
    virtual const Value *value(const Context *context) const;

    Engine *_engine;
    friend class Context;
};

class QMLJS_EXPORT ColorValue: public Value
{
public:
    // Value interface
    virtual const ColorValue *asColorValue() const;
    virtual void accept(ValueVisitor *) const;
};

class QMLJS_EXPORT AnchorLineValue: public Value
{
public:
    // Value interface
    virtual const AnchorLineValue *asAnchorLineValue() const;
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

    // may return a reference, prototypes may form a cycle: use PrototypeIterator!
    const Value *prototype() const;
    // prototypes may form a cycle: use PrototypeIterator!
    const ObjectValue *prototype(const Context *context) const;
    void setPrototype(const Value *prototype);

    virtual void processMembers(MemberProcessor *processor) const;

    virtual void setMember(const QString &name, const Value *value);
    virtual void removeMember(const QString &name);

    virtual const Value *lookupMember(const QString &name, const Context *context,
                                      const ObjectValue **foundInObject = 0,
                                      bool examinePrototypes = true) const;

    // Value interface
    virtual const ObjectValue *asObjectValue() const;
    virtual void accept(ValueVisitor *visitor) const;

private:
    bool checkPrototype(const ObjectValue *prototype, QSet<const ObjectValue *> *processed) const;

private:
    Engine *_engine;
    QHash<QString, const Value *> _members;
    QString _className;

protected:
    const Value *_prototype;
};

class QMLJS_EXPORT PrototypeIterator
{
public:
    enum Error
    {
        NoError,
        ReferenceResolutionError,
        CycleError
    };

    PrototypeIterator(const ObjectValue *start, const Context *context);

    bool hasNext();
    const ObjectValue *peekNext();
    const ObjectValue *next();
    Error error() const;

    QList<const ObjectValue *> all();

private:
    const ObjectValue *m_current;
    const ObjectValue *m_next;
    QList<const ObjectValue *> m_prototypes;
    const Context *m_context;
    Error m_error;
};

// A ObjectValue based on a FakeMetaObject.
// May only have other QmlObjectValues as ancestors.
class QMLJS_EXPORT QmlObjectValue: public ObjectValue
{
public:
    QmlObjectValue(LanguageUtils::FakeMetaObject::ConstPtr metaObject, const QString &className,
                   const QString &packageName, const LanguageUtils::ComponentVersion version,
                   Engine *engine);
    virtual ~QmlObjectValue();

    virtual void processMembers(MemberProcessor *processor) const;
    const Value *propertyValue(const LanguageUtils::FakeMetaProperty &prop) const;

    using ObjectValue::prototype;
    const QmlObjectValue *prototype() const;

    const QmlObjectValue *attachedType() const;
    void setAttachedType(QmlObjectValue *value);

    LanguageUtils::FakeMetaObject::ConstPtr metaObject() const;

    QString packageName() const;
    LanguageUtils::ComponentVersion version() const;

    QString defaultPropertyName() const;
    QString propertyType(const QString &propertyName) const;
    bool isListProperty(const QString &name) const;
    bool isWritable(const QString &propertyName) const;
    bool isPointer(const QString &propertyName) const;
    bool hasLocalProperty(const QString &typeName) const;
    bool hasProperty(const QString &typeName) const;
    bool hasChildInPackage() const;

    LanguageUtils::FakeMetaEnum getEnum(const QString &typeName) const;

    // deprecated
    bool isEnum(const QString &typeName) const;
    QStringList keysForEnum(const QString &enumName) const;
    bool enumContainsKey(const QString &enumName, const QString &enumKeyName) const;
protected:
    const Value *findOrCreateSignature(int index, const LanguageUtils::FakeMetaMethod &method,
                                       QString *methodName) const;
    bool isDerivedFrom(LanguageUtils::FakeMetaObject::ConstPtr base) const;

private:
    QmlObjectValue *_attachedType;
    LanguageUtils::FakeMetaObject::ConstPtr _metaObject;
    const QString _packageName;
    const LanguageUtils::ComponentVersion _componentVersion;
    mutable QHash<int, const Value *> _metaSignature;
};

class QMLJS_EXPORT QmlEnumValue: public NumberValue
{
public:
    QmlEnumValue(const LanguageUtils::FakeMetaEnum &metaEnum, Engine *engine);
    virtual ~QmlEnumValue();

    QString name() const;
    QStringList keys() const;

private:
    LanguageUtils::FakeMetaEnum *_metaEnum;
};

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
    virtual const Value *lookupMember(const QString &name, const Context *context,
                                      const ObjectValue **foundInObject = 0,
                                      bool examinePrototypes = true) const;

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

class QMLJS_EXPORT CppQmlTypesLoader
{
public:
    /** Loads a set of qmltypes files into the builtin objects list
        and returns errors and warnings
    */
    static void loadQmlTypes(const QFileInfoList &qmltypesFiles,
                             QStringList *errors, QStringList *warnings);

    static QHash<QString, LanguageUtils::FakeMetaObject::ConstPtr> builtinObjects;
    static QHash<QString, QList<LanguageUtils::ComponentVersion> > builtinPackages;

    // parses the contents of a qmltypes file and fills the newObjects map
    static void parseQmlTypeDescriptions(
        const QByteArray &qmlTypes,
        QHash<QString, LanguageUtils::FakeMetaObject::ConstPtr> *newObjects,
        QString *errorMessage, QString *warningMessage);
};

class QMLJS_EXPORT CppQmlTypes
{
public:
    // package name for objects that should be always available
    static const QLatin1String defaultPackage;
    // package name for objects with their raw cpp name
    static const QLatin1String cppPackage;

    template <typename T>
    QList<QmlObjectValue *> load(Interpreter::Engine *interpreter, const T &objects);

    QList<Interpreter::QmlObjectValue *> typesForImport(const QString &prefix, LanguageUtils::ComponentVersion version) const;
    Interpreter::QmlObjectValue *typeByCppName(const QString &cppName) const;

    bool hasPackage(const QString &package) const;

    QHash<QString, QmlObjectValue *> types() const
    { return _typesByFullyQualifiedName; }

    static QString qualifiedName(const QString &package, const QString &type, LanguageUtils::ComponentVersion version);
    QmlObjectValue *typeByQualifiedName(const QString &fullyQualifiedName) const;
    QmlObjectValue *typeByQualifiedName(const QString &package, const QString &type,
                                        LanguageUtils::ComponentVersion version) const;

private:
    void setPrototypes(QmlObjectValue *object);
    QmlObjectValue *getOrCreate(Engine *engine,
                                LanguageUtils::FakeMetaObject::ConstPtr metaObject,
                                const LanguageUtils::FakeMetaObject::Export &exp,
                                bool *wasCreated = 0);
    QmlObjectValue *getOrCreateForPackage(const QString &package, const QString &cppName);


    QHash<QString, QList<QmlObjectValue *> > _typesByPackage;
    QHash<QString, QmlObjectValue *> _typesByFullyQualifiedName;
};

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

class QMLJS_EXPORT TypeId: protected ValueVisitor
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
    virtual void visit(const ColorValue *);
    virtual void visit(const AnchorLineValue *);
};

class QMLJS_EXPORT Engine
{
    Q_DISABLE_COPY(Engine)

public:
    Engine();
    ~Engine();

    const NullValue *nullValue() const;
    const UndefinedValue *undefinedValue() const;
    const NumberValue *numberValue() const;
    const RealValue *realValue() const;
    const IntValue *intValue() const;
    const BooleanValue *booleanValue() const;
    const StringValue *stringValue() const;
    const UrlValue *urlValue() const;
    const ColorValue *colorValue() const;
    const AnchorLineValue *anchorLineValue() const;

    ObjectValue *newObject(const ObjectValue *prototype);
    ObjectValue *newObject();
    Function *newFunction();
    const Value *newArray(); // ### remove me

    // QML objects
    const ObjectValue *qmlKeysObject();
    const ObjectValue *qmlFontObject();
    const ObjectValue *qmlPointObject();
    const ObjectValue *qmlSizeObject();
    const ObjectValue *qmlRectObject();
    const ObjectValue *qmlVector3DObject();

    const Value *defaultValueForBuiltinType(const QString &typeName) const;

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
    CppQmlTypes &cppQmlTypes()
    { return _cppQmlTypes; }
    const CppQmlTypes &cppQmlTypes() const
    { return _cppQmlTypes; }

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
    ObjectValue *_qmlKeysObject;
    ObjectValue *_qmlFontObject;
    ObjectValue *_qmlPointObject;
    ObjectValue *_qmlSizeObject;
    ObjectValue *_qmlRectObject;
    ObjectValue *_qmlVector3DObject;

    NullValue _nullValue;
    UndefinedValue _undefinedValue;
    NumberValue _numberValue;
    RealValue _realValue;
    IntValue _intValue;
    BooleanValue _booleanValue;
    StringValue _stringValue;
    UrlValue _urlValue;
    ColorValue _colorValue;
    AnchorLineValue _anchorLineValue;
    QList<Value *> _registeredValues;

    ConvertToNumber _convertToNumber;
    ConvertToString _convertToString;
    ConvertToObject _convertToObject;
    TypeId _typeId;

    CppQmlTypes _cppQmlTypes;

    QMutex _mutex;
};


// internal
class QMLJS_EXPORT QmlPrototypeReference: public Reference
{
public:
    QmlPrototypeReference(AST::UiQualifiedId *qmlTypeName, const Document *doc, Engine *engine);
    virtual ~QmlPrototypeReference();

    AST::UiQualifiedId *qmlTypeName() const;

private:    
    virtual const Value *value(const Context *context) const;

    AST::UiQualifiedId *_qmlTypeName;
    const Document *_doc;
};

class QMLJS_EXPORT ASTVariableReference: public Reference
{
    AST::VariableDeclaration *_ast;

public:
    ASTVariableReference(AST::VariableDeclaration *ast, Engine *engine);
    virtual ~ASTVariableReference();

private:
    virtual const Value *value(const Context *context) const;
};

class QMLJS_EXPORT ASTFunctionValue: public FunctionValue
{
    AST::FunctionExpression *_ast;
    const Document *_doc;
    QList<NameId *> _argumentNames;

public:
    ASTFunctionValue(AST::FunctionExpression *ast, const Document *doc, Engine *engine);
    virtual ~ASTFunctionValue();

    AST::FunctionExpression *ast() const;

    virtual const Value *returnValue() const;
    virtual int argumentCount() const;
    virtual const Value *argument(int) const;
    virtual QString argumentName(int index) const;
    virtual bool isVariadic() const;

    virtual bool getSourceLocation(QString *fileName, int *line, int *column) const;
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

private:
    virtual const Value *value(const Context *context) const;
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

private:
    virtual const Value *value(const Context *context) const;
};

class QMLJS_EXPORT ASTObjectValue: public ObjectValue
{
    AST::UiQualifiedId *_typeName;
    AST::UiObjectInitializer *_initializer;
    const Document *_doc;
    QList<ASTPropertyReference *> _properties;
    QList<ASTSignalReference *> _signals;
    ASTPropertyReference *_defaultPropertyRef;

public:
    ASTObjectValue(AST::UiQualifiedId *typeName,
                   AST::UiObjectInitializer *initializer,
                   const Document *doc,
                   Engine *engine);
    virtual ~ASTObjectValue();

    bool getSourceLocation(QString *fileName, int *line, int *column) const;
    virtual void processMembers(MemberProcessor *processor) const;

    QString defaultPropertyName() const;

    AST::UiObjectInitializer *initializer() const;
    AST::UiQualifiedId *typeName() const;
    const Document *document() const;
};

class QMLJS_EXPORT ImportInfo
{
public:
    enum Type {
        InvalidImport,
        ImplicitDirectoryImport,
        LibraryImport,
        FileImport,
        DirectoryImport,
        UnknownFileImport // refers a file/directory that wasn't found
    };

    ImportInfo();
    ImportInfo(Type type, const QString &name,
               LanguageUtils::ComponentVersion version = LanguageUtils::ComponentVersion(),
               AST::UiImport *ast = 0);

    bool isValid() const;
    Type type() const;

    // LibraryImport: uri with '/' separator
    // Other: absoluteFilePath
    QString name() const;

    // null if the import has no 'as', otherwise the target id
    QString id() const;

    LanguageUtils::ComponentVersion version() const;
    AST::UiImport *ast() const;

private:
    Type _type;
    QString _name;
    LanguageUtils::ComponentVersion _version;
    AST::UiImport *_ast;
};

class QMLJS_EXPORT Import {
public:
    Import();

    // const!
    ObjectValue *object;
    ImportInfo info;
    // uri imports: path to library, else empty
    QString libraryPath;
};

class Imports;

class QMLJS_EXPORT TypeScope: public ObjectValue
{
public:
    TypeScope(const Imports *imports, Engine *engine);

    virtual const Value *lookupMember(const QString &name, const Context *context,
                                      const ObjectValue **foundInObject = 0,
                                      bool examinePrototypes = true) const;
    virtual void processMembers(MemberProcessor *processor) const;

private:
    const Imports *_imports;
};

class QMLJS_EXPORT JSImportScope: public ObjectValue
{
public:
    JSImportScope(const Imports *imports, Engine *engine);

    virtual const Value *lookupMember(const QString &name, const Context *context,
                                      const ObjectValue **foundInObject = 0,
                                      bool examinePrototypes = true) const;
    virtual void processMembers(MemberProcessor *processor) const;

private:
    const Imports *_imports;
};

class QMLJS_EXPORT Imports
{
public:
    Imports(Engine *engine);

    void append(const Import &import);

    ImportInfo info(const QString &name, const Context *context) const;
    QList<Import> all() const;

    const TypeScope *typeScope() const;
    const JSImportScope *jsImportScope() const;

#ifdef QT_DEBUG
    void dump() const;
#endif

private:
    // holds imports in the order they appeared,
    // lookup order is back to front
    QList<Import> _imports;
    TypeScope *_typeScope;
    JSImportScope *_jsImportScope;
};

} } // namespace QmlJS::Interpreter

#endif // QMLJS_INTERPRETER_H
