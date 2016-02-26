/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <qmljs/qmljs_global.h>
#include <qmljs/qmljsconstants.h>
#include <qmljs/qmljsimportdependencies.h>
#include <qmljs/parser/qmljsastfwd_p.h>

#include <languageutils/fakemetaobject.h>

#include <QFileInfoList>
#include <QHash>
#include <QList>
#include <QMutex>
#include <QSet>
#include <QSharedPointer>
#include <QString>

namespace QmlJS {
////////////////////////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////////////////////////
class ASTFunctionValue;
class ASTObjectValue;
class ASTPropertyReference;
class ASTSignal;
class ASTVariableReference;
class AnchorLineValue;
class BooleanValue;
class ColorValue;
class Context;
class CppComponentValue;
class Document;
class Function;
class FunctionValue;
class Imports;
class IntValue;
class JSImportScope;
class NameId;
class NullValue;
class NumberValue;
class ModuleApiInfo;
class ObjectValue;
class QmlEnumValue;
class QmlPrototypeReference;
class RealValue;
class Reference;
class ReferenceContext;
class StringValue;
class TypeScope;
class UndefinedValue;
class UnknownValue;
class UrlValue;
class Value;
class ValueOwner;
class MetaFunction;
typedef QSharedPointer<const Context> ContextPtr;

namespace Internal {
class QtObjectPrototypeReference;
} // namespace Internal

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
    virtual void visit(const UnknownValue *);
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
    virtual const UnknownValue *asUnknownValue() const;
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
    virtual const CppComponentValue *asCppComponentValue() const;
    virtual const ASTObjectValue *asAstObjectValue() const;
    virtual const QmlEnumValue *asQmlEnumValue() const;
    virtual const QmlPrototypeReference *asQmlPrototypeReference() const;
    virtual const ASTPropertyReference *asAstPropertyReference() const;
    virtual const ASTVariableReference *asAstVariableReference() const;
    virtual const Internal::QtObjectPrototypeReference *asQtObjectPrototypeReference() const;
    virtual const ASTSignal *asAstSignal() const;
    virtual const ASTFunctionValue *asAstFunctionValue() const;
    virtual const Function *asFunction() const;
    virtual const MetaFunction *asMetaFunction() const;
    virtual const JSImportScope *asJSImportScope() const;
    virtual const TypeScope *asTypeScope() const;

    virtual void accept(ValueVisitor *) const = 0;

    virtual bool getSourceLocation(QString *fileName, int *line, int *column) const;
};

template <typename RetTy> const RetTy *value_cast(const Value *)
{
    // Produce a good error message if a specialization is missing.
    RetTy::ERROR_MissingValueCastSpecialization();
    return 0;
}

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

template <> Q_INLINE_TEMPLATE const UnknownValue *value_cast(const Value *v)
{
    if (v) return v->asUnknownValue();
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

template <> Q_INLINE_TEMPLATE const ASTFunctionValue *value_cast(const Value *v)
{
    if (v) return v->asAstFunctionValue();
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

template <> Q_INLINE_TEMPLATE const CppComponentValue *value_cast(const Value *v)
{
    if (v) return v->asCppComponentValue();
    else   return 0;
}

template <> Q_INLINE_TEMPLATE const ASTObjectValue *value_cast(const Value *v)
{
    if (v) return v->asAstObjectValue();
    else   return 0;
}

template <> Q_INLINE_TEMPLATE const QmlEnumValue *value_cast(const Value *v)
{
    if (v) return v->asQmlEnumValue();
    else   return 0;
}

template <> Q_INLINE_TEMPLATE const QmlPrototypeReference *value_cast(const Value *v)
{
    if (v) return v->asQmlPrototypeReference();
    else   return 0;
}

template <> Q_INLINE_TEMPLATE const ASTPropertyReference *value_cast(const Value *v)
{
    if (v) return v->asAstPropertyReference();
    else   return 0;
}

template <> Q_INLINE_TEMPLATE const Internal::QtObjectPrototypeReference *value_cast(const Value *v)
{
    if (v) return v->asQtObjectPrototypeReference();
    else   return 0;
}

template <> Q_INLINE_TEMPLATE const ASTVariableReference *value_cast(const Value *v)
{
    if (v) return v->asAstVariableReference();
    else   return 0;
}

template <> Q_INLINE_TEMPLATE const Function *value_cast(const Value *v)
{
    if (v) return v->asFunction();
    else   return 0;
}

template <> Q_INLINE_TEMPLATE const MetaFunction *value_cast(const Value *v)
{
    if (v) return v->asMetaFunction();
    else   return 0;
}

template <> Q_INLINE_TEMPLATE const JSImportScope *value_cast(const Value *v)
{
    if (v) return v->asJSImportScope();
    else   return 0;
}

template <> Q_INLINE_TEMPLATE const TypeScope *value_cast(const Value *v)
{
    if (v) return v->asTypeScope();
    else   return 0;
}

template <> Q_INLINE_TEMPLATE const ASTSignal *value_cast(const Value *v)
{
    if (v) return v->asAstSignal();
    else   return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Value nodes
////////////////////////////////////////////////////////////////////////////////
class QMLJS_EXPORT NullValue: public Value
{
public:
    const NullValue *asNullValue() const override;
    void accept(ValueVisitor *visitor) const override;
};

class QMLJS_EXPORT UndefinedValue: public Value
{
public:
    const UndefinedValue *asUndefinedValue() const override;
    void accept(ValueVisitor *visitor) const override;
};

class QMLJS_EXPORT UnknownValue: public Value
{
public:
    const UnknownValue *asUnknownValue() const override;
    void accept(ValueVisitor *) const override;
};

class QMLJS_EXPORT NumberValue: public Value
{
public:
    const NumberValue *asNumberValue() const override;
    void accept(ValueVisitor *visitor) const override;
};

class QMLJS_EXPORT RealValue: public NumberValue
{
public:
    const RealValue *asRealValue() const override;
};

class QMLJS_EXPORT IntValue: public NumberValue
{
public:
    const IntValue *asIntValue() const override;
};

class QMLJS_EXPORT BooleanValue: public Value
{
public:
    const BooleanValue *asBooleanValue() const override;
    void accept(ValueVisitor *visitor) const override;
};

class QMLJS_EXPORT StringValue: public Value
{
public:
    const StringValue *asStringValue() const override;
    void accept(ValueVisitor *visitor) const override;
};

class QMLJS_EXPORT UrlValue: public StringValue
{
public:
    const UrlValue *asUrlValue() const override;
};

class PropertyInfo {
public:
    enum PropertyFlag {
        Readable    = 1,
        Writeable   = 2,
        ListType    = 4,
        PointerType= 8,
        ValueType  = 16,
        PointerOrValue = PointerType|ValueType,
        Default     = Readable|Writeable|PointerOrValue
    };

    PropertyInfo(uint flags = Default);
    uint flags;
    bool isPointer() const {
        return (flags & PointerOrValue) == PointerType;
    }
    bool isValue() const {
        return (flags & PointerOrValue) == ValueType;
    }
    bool canBePointer() const {
        return (flags & PointerType) != 0;
    }
    bool canBeValue() const {
        return (flags & ValueType) != 0;
    }
    bool isReadable() const {
        return (flags & Readable) != 0;
    }
    bool isWriteable() const {
        return (flags & Writeable) != 0;
    }
    bool isList() const {
        return (flags & ListType) != 0;
    }
    QString toString() const;
};

class QMLJS_EXPORT MemberProcessor
{
    MemberProcessor(const MemberProcessor &other);
    void operator = (const MemberProcessor &other);

public:
    MemberProcessor();
    virtual ~MemberProcessor();

    // Returns false to stop the processor.
    virtual bool processProperty(const QString &name, const Value *value,
                                 const PropertyInfo &propertyInfo);
    virtual bool processEnumerator(const QString &name, const Value *value);
    virtual bool processSignal(const QString &name, const Value *value);
    virtual bool processSlot(const QString &name, const Value *value);
    virtual bool processGeneratedSlot(const QString &name, const Value *value);
};

class QMLJS_EXPORT Reference: public Value
{
public:
    Reference(ValueOwner *valueOwner);
    ~Reference();

    ValueOwner *valueOwner() const;

    // Value interface
    const Reference *asReference() const override;
    void accept(ValueVisitor *) const override;

private:
    virtual const Value *value(ReferenceContext *referenceContext) const;

    ValueOwner *m_valueOwner;
    friend class ReferenceContext;
};

class QMLJS_EXPORT ColorValue: public Value
{
public:
    // Value interface
    const ColorValue *asColorValue() const override;
    void accept(ValueVisitor *) const override;
};

class QMLJS_EXPORT AnchorLineValue: public Value
{
public:
    // Value interface
    const AnchorLineValue *asAnchorLineValue() const override;
    void accept(ValueVisitor *) const override;
};

class QMLJS_EXPORT PropertyData {
public:
    const Value *value;
    PropertyInfo propertyInfo;
    PropertyData(const Value *value = 0,
                 PropertyInfo propertyInfo = PropertyInfo(PropertyInfo::Default))
        : value(value), propertyInfo(propertyInfo)
    { }
};

class QMLJS_EXPORT ObjectValue: public Value
{
public:
    ObjectValue(ValueOwner *valueOwner, const QString &originId = QString());
    ~ObjectValue();

    ValueOwner *valueOwner() const;

    QString className() const;
    void setClassName(const QString &className);

    // may return a reference, prototypes may form a cycle: use PrototypeIterator!
    const Value *prototype() const;
    // prototypes may form a cycle: use PrototypeIterator!
    const ObjectValue *prototype(const Context *context) const;
    const ObjectValue *prototype(const ContextPtr &context) const
    { return prototype(context.data()); }
    void setPrototype(const Value *prototype);

    virtual void processMembers(MemberProcessor *processor) const;

    virtual void setMember(const QString &name, const Value *value);
    virtual void setPropertyInfo(const QString &name, const PropertyInfo &propertyInfo);
    virtual void removeMember(const QString &name);

    virtual const Value *lookupMember(const QString &name, const Context *context,
                                      const ObjectValue **foundInObject = 0,
                                      bool examinePrototypes = true) const;
    virtual const Value *lookupMember(const QString &name, const ContextPtr &context,
                              const ObjectValue **foundInObject = 0,
                              bool examinePrototypes = true) const
    { return lookupMember(name, context.data(), foundInObject, examinePrototypes); }

    // Value interface
    const ObjectValue *asObjectValue() const override;
    void accept(ValueVisitor *visitor) const override;
    QString originId() const
    { return m_originId; }


private:
    bool checkPrototype(const ObjectValue *prototype, QSet<const ObjectValue *> *processed) const;

private:
    ValueOwner *m_valueOwner;
    QHash<QString, PropertyData> m_members;
    QString m_className;
    QString m_originId;

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
    PrototypeIterator(const ObjectValue *start, const ContextPtr &context);

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

class QMLJS_EXPORT QmlEnumValue: public NumberValue
{
public:
    QmlEnumValue(const CppComponentValue *owner, int index);
    ~QmlEnumValue();

    const QmlEnumValue *asQmlEnumValue() const override;

    QString name() const;
    QStringList keys() const;
    const CppComponentValue *owner() const;

private:
    const CppComponentValue *m_owner;
    int m_enumIndex;
};


// A ObjectValue based on a FakeMetaObject.
// May only have other CppComponentValue as ancestors.
class QMLJS_EXPORT CppComponentValue: public ObjectValue
{
public:
    CppComponentValue(LanguageUtils::FakeMetaObject::ConstPtr metaObject, const QString &className,
                   const QString &moduleName, const LanguageUtils::ComponentVersion &componentVersion,
                   const LanguageUtils::ComponentVersion &importVersion, int metaObjectRevision,
                   ValueOwner *valueOwner, const QString &originId);
    ~CppComponentValue();

    const CppComponentValue *asCppComponentValue() const override;

    void processMembers(MemberProcessor *processor) const override;
    const Value *valueForCppName(const QString &typeName) const;

    using ObjectValue::prototype;
    const CppComponentValue *prototype() const;
    QList<const CppComponentValue *> prototypes() const;

    LanguageUtils::FakeMetaObject::ConstPtr metaObject() const;

    QString moduleName() const;
    LanguageUtils::ComponentVersion componentVersion() const;
    LanguageUtils::ComponentVersion importVersion() const;

    QString defaultPropertyName() const;
    QString propertyType(const QString &propertyName) const;
    bool isListProperty(const QString &name) const;
    bool isWritable(const QString &propertyName) const;
    bool isPointer(const QString &propertyName) const;
    bool hasLocalProperty(const QString &typeName) const;
    bool hasProperty(const QString &typeName) const;

    LanguageUtils::FakeMetaEnum getEnum(const QString &typeName, const CppComponentValue **foundInScope = 0) const;
    const QmlEnumValue *getEnumValue(const QString &typeName, const CppComponentValue **foundInScope = 0) const;

    const ObjectValue *signalScope(const QString &signalName) const;
protected:
    bool isDerivedFrom(LanguageUtils::FakeMetaObject::ConstPtr base) const;

private:
    LanguageUtils::FakeMetaObject::ConstPtr m_metaObject;
    const QString m_moduleName;
    // _componentVersion is the version of the export
    // _importVersion is the version it's imported as, used to find correct prototypes
    // needed in cases when B 1.0 has A 1.1 as prototype when imported as 1.1
    const LanguageUtils::ComponentVersion m_componentVersion;
    const LanguageUtils::ComponentVersion m_importVersion;
    mutable QAtomicPointer< QList<const Value *> > m_metaSignatures;
    mutable QAtomicPointer< QHash<QString, const ObjectValue *> > m_signalScopes;
    QHash<QString, const QmlEnumValue * > m_enums;
    int m_metaObjectRevision;
};

class QMLJS_EXPORT FunctionValue: public ObjectValue
{
public:
    FunctionValue(ValueOwner *valueOwner);
    ~FunctionValue();

    virtual const Value *returnValue() const;

    // Access to the names of arguments
    // Named arguments can be optional (usually known for builtins only)
    virtual int namedArgumentCount() const;
    virtual QString argumentName(int index) const;

    // The number of optional named arguments
    // Example: JSON.stringify(value[, replacer[, space]])
    //          has namedArgumentCount = 3
    //          and optionalNamedArgumentCount = 2
    virtual int optionalNamedArgumentCount() const;

    // Whether the function accepts an unlimited number of arguments
    // after the named ones. Defaults to false.
    // Example: Math.max(...)
    virtual bool isVariadic() const;

    virtual const Value *argument(int index) const;

    // Value interface
    const FunctionValue *asFunctionValue() const override;
    void accept(ValueVisitor *visitor) const override;
};

class QMLJS_EXPORT Function: public FunctionValue
{
public:
    Function(ValueOwner *valueOwner);
    ~Function();

    void addArgument(const Value *argument, const QString &name = QString());
    void setReturnValue(const Value *returnValue);
    void setVariadic(bool variadic);
    void setOptionalNamedArgumentCount(int count);

    // FunctionValue interface
    const Value *returnValue() const override;
    int namedArgumentCount() const override;
    int optionalNamedArgumentCount() const override;
    const Value *argument(int index) const override;
    QString argumentName(int index) const override;
    bool isVariadic() const override;
    const Function *asFunction() const override;

private:
    ValueList m_arguments;
    QStringList m_argumentNames;
    const Value *m_returnValue;
    int m_optionalNamedArgumentCount;
    bool m_isVariadic;
};


////////////////////////////////////////////////////////////////////////////////
// typing environment
////////////////////////////////////////////////////////////////////////////////

class QMLJS_EXPORT CppQmlTypesLoader
{
public:
    typedef QHash<QString, LanguageUtils::FakeMetaObject::ConstPtr> BuiltinObjects;

    /** Loads a set of qmltypes files into the builtin objects list
        and returns errors and warnings
    */
    static BuiltinObjects loadQmlTypes(const QFileInfoList &qmltypesFiles,
                             QStringList *errors, QStringList *warnings);

    static BuiltinObjects defaultQtObjects;
    static BuiltinObjects defaultLibraryObjects;

    // parses the contents of a qmltypes file and fills the newObjects map
    static void parseQmlTypeDescriptions(const QByteArray &contents,
                                         BuiltinObjects *newObjects,
                                         QList<ModuleApiInfo> *newModuleApis,
                                         QStringList *newDependencies,
                                         QString *errorMessage,
                                         QString *warningMessage,
                                         const QString &fileName);
};

class QMLJS_EXPORT FakeMetaObjectWithOrigin
{
public:
    LanguageUtils::FakeMetaObject::ConstPtr fakeMetaObject;
    QString originId;
    FakeMetaObjectWithOrigin(LanguageUtils::FakeMetaObject::ConstPtr fakeMetaObject,
                             const QString &originId);
    bool operator ==(const FakeMetaObjectWithOrigin &o) const;
};

QMLJS_EXPORT uint qHash(const FakeMetaObjectWithOrigin &fmoo);

class QMLJS_EXPORT CppQmlTypes
{
public:
    CppQmlTypes(ValueOwner *valueOwner);

    // package name for objects that should be always available
    static const QLatin1String defaultPackage;
    // package name for objects with their raw cpp name
    static const QLatin1String cppPackage;

    template <typename T>
    void load(const QString &originId, const T &fakeMetaObjects, const QString &overridePackage = QString());

    QList<const CppComponentValue *> createObjectsForImport(const QString &package, LanguageUtils::ComponentVersion version);
    bool hasModule(const QString &module) const;

    static QString qualifiedName(const QString &module, const QString &type,
                                 LanguageUtils::ComponentVersion version);
    const CppComponentValue *objectByQualifiedName(const QString &fullyQualifiedName) const;
    const CppComponentValue *objectByQualifiedName(
            const QString &package, const QString &type,
            LanguageUtils::ComponentVersion version) const;
    const CppComponentValue *objectByCppName(const QString &cppName) const;

    void setCppContextProperties(const ObjectValue *contextProperties);
    const ObjectValue *cppContextProperties() const;

private:
    // "Package.CppName ImportVersion" ->  CppComponentValue
    QHash<QString, const CppComponentValue *> m_objectsByQualifiedName;
    QHash<QString, QSet<FakeMetaObjectWithOrigin> > m_fakeMetaObjectsByPackage;
    const ObjectValue *m_cppContextProperties;
    ValueOwner *m_valueOwner;
};

class ConvertToNumber: protected ValueVisitor // ECMAScript ToInt()
{
public:
    ConvertToNumber(ValueOwner *valueOwner);

    const Value *operator()(const Value *value);

protected:
    const Value *switchResult(const Value *value);

    void visit(const NullValue *) override;
    void visit(const UndefinedValue *) override;
    void visit(const NumberValue *) override;
    void visit(const BooleanValue *) override;
    void visit(const StringValue *) override;
    void visit(const ObjectValue *) override;
    void visit(const FunctionValue *) override;

private:
    ValueOwner *m_valueOwner;
    const Value *m_result;
};

class ConvertToString: protected ValueVisitor // ECMAScript ToString
{
public:
    ConvertToString(ValueOwner *valueOwner);

    const Value *operator()(const Value *value);

protected:
    const Value *switchResult(const Value *value);

    void visit(const NullValue *) override;
    void visit(const UndefinedValue *) override;
    void visit(const NumberValue *) override;
    void visit(const BooleanValue *) override;
    void visit(const StringValue *) override;
    void visit(const ObjectValue *) override;
    void visit(const FunctionValue *) override;

private:
    ValueOwner *m_valueOwner;
    const Value *m_result;
};

class ConvertToObject: protected ValueVisitor // ECMAScript ToObject
{
public:
    ConvertToObject(ValueOwner *valueOwner);

    const Value *operator()(const Value *value);

protected:
    const Value *switchResult(const Value *value);

    void visit(const NullValue *) override;
    void visit(const UndefinedValue *) override;
    void visit(const NumberValue *) override;
    void visit(const BooleanValue *) override;
    void visit(const StringValue *) override;
    void visit(const ObjectValue *) override;
    void visit(const FunctionValue *) override;

private:
    ValueOwner *m_valueOwner;
    const Value *m_result;
};

class QMLJS_EXPORT TypeId: protected ValueVisitor
{
    QString _result;

public:
    QString operator()(const Value *value);

protected:
    void visit(const NullValue *) override;
    void visit(const UndefinedValue *) override;
    void visit(const NumberValue *) override;
    void visit(const BooleanValue *) override;
    void visit(const StringValue *) override;
    void visit(const ObjectValue *object) override;
    void visit(const FunctionValue *object) override;
    void visit(const ColorValue *) override;
    void visit(const AnchorLineValue *) override;
};

// internal
class QMLJS_EXPORT QmlPrototypeReference: public Reference
{
public:
    QmlPrototypeReference(AST::UiQualifiedId *qmlTypeName, const Document *doc, ValueOwner *valueOwner);
    ~QmlPrototypeReference();

    const QmlPrototypeReference *asQmlPrototypeReference() const override;

    AST::UiQualifiedId *qmlTypeName() const;
    const Document *document() const;

private:
    const Value *value(ReferenceContext *referenceContext) const override;

    AST::UiQualifiedId *m_qmlTypeName;
    const Document *m_doc;
};

class QMLJS_EXPORT ASTVariableReference: public Reference
{
    AST::VariableDeclaration *m_ast;
    const Document *m_doc;

public:
    ASTVariableReference(AST::VariableDeclaration *ast, const Document *doc, ValueOwner *valueOwner);
    ~ASTVariableReference();
    const ASTVariableReference *asAstVariableReference() const override;
    const AST::VariableDeclaration *ast() const;
private:
    const Value *value(ReferenceContext *referenceContext) const override;
    bool getSourceLocation(QString *fileName, int *line, int *column) const override;
};

class QMLJS_EXPORT ASTFunctionValue: public FunctionValue
{
    AST::FunctionExpression *m_ast;
    const Document *m_doc;
    QList<QString> m_argumentNames;
    bool m_isVariadic;

public:
    ASTFunctionValue(AST::FunctionExpression *ast, const Document *doc, ValueOwner *valueOwner);
    ~ASTFunctionValue();

    AST::FunctionExpression *ast() const;

    int namedArgumentCount() const override;
    QString argumentName(int index) const override;
    bool isVariadic() const override;
    const ASTFunctionValue *asAstFunctionValue() const override;

    bool getSourceLocation(QString *fileName, int *line, int *column) const override;
};

class QMLJS_EXPORT ASTPropertyReference: public Reference
{
    AST::UiPublicMember *m_ast;
    const Document *m_doc;
    QString m_onChangedSlotName;

public:
    ASTPropertyReference(AST::UiPublicMember *ast, const Document *doc, ValueOwner *valueOwner);
    ~ASTPropertyReference();

    const ASTPropertyReference *asAstPropertyReference() const override;

    AST::UiPublicMember *ast() const { return m_ast; }
    QString onChangedSlotName() const { return m_onChangedSlotName; }

    bool getSourceLocation(QString *fileName, int *line, int *column) const override;

private:
    const Value *value(ReferenceContext *referenceContext) const override;
};

class QMLJS_EXPORT ASTSignal: public FunctionValue
{
    AST::UiPublicMember *m_ast;
    const Document *m_doc;
    QString m_slotName;
    const ObjectValue *m_bodyScope;

public:
    ASTSignal(AST::UiPublicMember *ast, const Document *doc, ValueOwner *valueOwner);
    ~ASTSignal();

    const ASTSignal *asAstSignal() const override;

    AST::UiPublicMember *ast() const { return m_ast; }
    QString slotName() const { return m_slotName; }
    const ObjectValue *bodyScope() const { return m_bodyScope; }

    // FunctionValue interface
    int namedArgumentCount() const override;
    const Value *argument(int index) const override;
    QString argumentName(int index) const override;

    // Value interface
    bool getSourceLocation(QString *fileName, int *line, int *column) const override;
};

class QMLJS_EXPORT ASTObjectValue: public ObjectValue
{
    AST::UiQualifiedId *m_typeName;
    AST::UiObjectInitializer *m_initializer;
    const Document *m_doc;
    QList<ASTPropertyReference *> m_properties;
    QList<ASTSignal *> m_signals;
    ASTPropertyReference *m_defaultPropertyRef;

public:
    ASTObjectValue(AST::UiQualifiedId *typeName,
                   AST::UiObjectInitializer *initializer,
                   const Document *doc,
                   ValueOwner *valueOwner);
    ~ASTObjectValue();

    const ASTObjectValue *asAstObjectValue() const override;

    bool getSourceLocation(QString *fileName, int *line, int *column) const override;
    void processMembers(MemberProcessor *processor) const override;

    QString defaultPropertyName() const;

    AST::UiObjectInitializer *initializer() const;
    AST::UiQualifiedId *typeName() const;
    const Document *document() const;
};

class QMLJS_EXPORT ImportInfo
{
public:
    ImportInfo();

    static ImportInfo moduleImport(QString uri, LanguageUtils::ComponentVersion version,
                                   const QString &as, AST::UiImport *ast = 0);
    static ImportInfo pathImport(const QString &docPath, const QString &path,
                                 LanguageUtils::ComponentVersion version,
                                 const QString &as, AST::UiImport *ast = 0);
    static ImportInfo invalidImport(AST::UiImport *ast = 0);
    static ImportInfo implicitDirectoryImport(const QString &directory);
    static ImportInfo qrcDirectoryImport(const QString &directory);

    bool isValid() const;
    ImportType::Enum type() const;

    // LibraryImport: uri with ',' separator
    // Other: non-absolute path
    QString name() const;

    // LibraryImport: uri with '/' separator
    // Other: absoluteFilePath
    QString path() const;

    // null if the import has no 'as', otherwise the target id
    QString as() const;

    LanguageUtils::ComponentVersion version() const;
    AST::UiImport *ast() const;

private:
    ImportType::Enum m_type;
    LanguageUtils::ComponentVersion m_version;
    QString m_name;
    QString m_path;
    QString m_as;
    AST::UiImport *m_ast;
};

class QMLJS_EXPORT Import {
public:
    Import();
    Import(const Import &other);

    // const!
    ObjectValue *object;
    ImportInfo info;
    DependencyInfo::ConstPtr deps;
    // uri imports: path to library, else empty
    QString libraryPath;
    // whether the import succeeded
    bool valid;
    mutable bool used;
};

class Imports;

class QMLJS_EXPORT TypeScope: public ObjectValue
{
public:
    TypeScope(const Imports *imports, ValueOwner *valueOwner);

    virtual const Value *lookupMember(const QString &name, const Context *context,
                                      const ObjectValue **foundInObject = 0,
                                      bool examinePrototypes = true) const override;
    void processMembers(MemberProcessor *processor) const override;
    const TypeScope *asTypeScope() const override;
private:
    const Imports *m_imports;
};

class QMLJS_EXPORT JSImportScope: public ObjectValue
{
public:
    JSImportScope(const Imports *imports, ValueOwner *valueOwner);

    virtual const Value *lookupMember(const QString &name, const Context *context,
                                      const ObjectValue **foundInObject = 0,
                                      bool examinePrototypes = true) const override;
    void processMembers(MemberProcessor *processor) const override;
    const JSImportScope *asJSImportScope() const override;
private:
    const Imports *m_imports;
};

class QMLJS_EXPORT Imports
{
public:
    Imports(ValueOwner *valueOwner);

    void append(const Import &import);
    void setImportFailed();

    ImportInfo info(const QString &name, const Context *context) const;
    QString nameForImportedObject(const ObjectValue *value, const Context *context) const;
    bool importFailed() const;

    const QList<Import> &all() const;

    const TypeScope *typeScope() const;
    const JSImportScope *jsImportScope() const;

#ifdef QT_DEBUG
    void dump() const;
#endif

private:
    // holds imports in the order they appeared,
    // lookup order is back to front
    QList<Import> m_imports;
    TypeScope *m_typeScope;
    JSImportScope *m_jsImportScope;
    bool m_importFailed;
};

class QMLJS_EXPORT MetaFunction: public FunctionValue
{
    LanguageUtils::FakeMetaMethod m_method;

public:
    MetaFunction(const LanguageUtils::FakeMetaMethod &method, ValueOwner *valueOwner);

    int namedArgumentCount() const override;
    QString argumentName(int index) const override;
    bool isVariadic() const override;
    const MetaFunction *asMetaFunction() const override;
    const LanguageUtils::FakeMetaMethod &fakeMetaMethod() const;
};

class QMLJS_EXPORT CustomImportsProvider : public QObject
{
    Q_OBJECT
public:
    explicit CustomImportsProvider(QObject *parent = 0) : QObject(parent) {}
    virtual ~CustomImportsProvider() {}
    virtual QList<Import> imports(ValueOwner *valueOwner, const Document *context) const = 0;
};

} // namespace QmlJS
