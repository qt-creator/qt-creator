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

////////////////////////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////////////////////////
class ValueOwner;
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
class Context;
typedef QSharedPointer<const Context> ContextPtr;
class ReferenceContext;

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

class QMLJS_EXPORT Reference: public Value
{
public:
    Reference(ValueOwner *valueOwner);
    virtual ~Reference();

    ValueOwner *valueOwner() const;

    // Value interface
    virtual const Reference *asReference() const;
    virtual void accept(ValueVisitor *) const;

private:
    virtual const Value *value(ReferenceContext *referenceContext) const;

    ValueOwner *_valueOwner;
    friend class ReferenceContext;
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
    ObjectValue(ValueOwner *valueOwner);
    virtual ~ObjectValue();

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
    virtual void removeMember(const QString &name);

    virtual const Value *lookupMember(const QString &name, const Context *context,
                                      const ObjectValue **foundInObject = 0,
                                      bool examinePrototypes = true) const;
    const Value *lookupMember(const QString &name, const ContextPtr &context,
                              const ObjectValue **foundInObject = 0,
                              bool examinePrototypes = true) const
    { return lookupMember(name, context.data(), foundInObject, examinePrototypes); }

    // Value interface
    virtual const ObjectValue *asObjectValue() const;
    virtual void accept(ValueVisitor *visitor) const;

private:
    bool checkPrototype(const ObjectValue *prototype, QSet<const ObjectValue *> *processed) const;

private:
    ValueOwner *_valueOwner;
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

class QmlObjectValue;

class QMLJS_EXPORT QmlEnumValue: public NumberValue
{
public:
    QmlEnumValue(const QmlObjectValue *owner, int index);
    virtual ~QmlEnumValue();

    QString name() const;
    QStringList keys() const;
    const QmlObjectValue *owner() const;

private:
    const QmlObjectValue *_owner;
    int _enumIndex;
};


// A ObjectValue based on a FakeMetaObject.
// May only have other QmlObjectValues as ancestors.
class QMLJS_EXPORT QmlObjectValue: public ObjectValue
{
public:
    QmlObjectValue(LanguageUtils::FakeMetaObject::ConstPtr metaObject, const QString &className,
                   const QString &moduleName, const LanguageUtils::ComponentVersion &componentVersion,
                   const LanguageUtils::ComponentVersion &importVersion,
                   ValueOwner *valueOwner);
    virtual ~QmlObjectValue();

    virtual void processMembers(MemberProcessor *processor) const;
    const Value *propertyValue(const LanguageUtils::FakeMetaProperty &prop) const;

    using ObjectValue::prototype;
    const QmlObjectValue *prototype() const;

    const QmlObjectValue *attachedType() const;
    void setAttachedType(QmlObjectValue *value);

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

    LanguageUtils::FakeMetaEnum getEnum(const QString &typeName, const QmlObjectValue **foundInScope = 0) const;
    const QmlEnumValue *getEnumValue(const QString &typeName, const QmlObjectValue **foundInScope = 0) const;
protected:
    const Value *findOrCreateSignature(int index, const LanguageUtils::FakeMetaMethod &method,
                                       QString *methodName) const;
    bool isDerivedFrom(LanguageUtils::FakeMetaObject::ConstPtr base) const;

private:
    QmlObjectValue *_attachedType;
    LanguageUtils::FakeMetaObject::ConstPtr _metaObject;
    const QString _moduleName;
    // _componentVersion is the version of the export
    // _importVersion is the version it's imported as, used to find correct prototypes
    // needed in cases when B 1.0 has A 1.1 as prototype when imported as 1.1
    const LanguageUtils::ComponentVersion _componentVersion;
    const LanguageUtils::ComponentVersion _importVersion;
    mutable QHash<int, const Value *> _metaSignature;
    QHash<QString, const QmlEnumValue * > _enums;
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
    FunctionValue(ValueOwner *valueOwner);
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
    Function(ValueOwner *valueOwner);
    virtual ~Function();

    void addArgument(const Value *argument, const QString &name = QString());
    void setReturnValue(const Value *returnValue);

    // FunctionValue interface
    virtual const Value *returnValue() const;
    virtual int argumentCount() const;
    virtual const Value *argument(int index) const;
    virtual QString argumentName(int index) const;
    virtual const Value *invoke(const Activation *activation) const;

private:
    ValueList _arguments;
    QStringList _argumentNames;
    const Value *_returnValue;
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
    static void parseQmlTypeDescriptions(
        const QByteArray &qmlTypes,
        BuiltinObjects *newObjects,
        QString *errorMessage, QString *warningMessage);
};

class QMLJS_EXPORT CppQmlTypes
{
public:
    CppQmlTypes(ValueOwner *valueOwner);

    // package name for objects that should be always available
    static const QLatin1String defaultPackage;
    // package name for objects with their raw cpp name
    static const QLatin1String cppPackage;

    template <typename T>
    void load(const T &fakeMetaObjects, const QString &overridePackage = QString());

    QList<const QmlObjectValue *> createObjectsForImport(const QString &package, LanguageUtils::ComponentVersion version);
    bool hasModule(const QString &module) const;

    static QString qualifiedName(const QString &module, const QString &type,
                                 LanguageUtils::ComponentVersion version);
    const QmlObjectValue *objectByQualifiedName(const QString &fullyQualifiedName) const;
    const QmlObjectValue *objectByQualifiedName(
            const QString &package, const QString &type,
            LanguageUtils::ComponentVersion version) const;
    const QmlObjectValue *objectByCppName(const QString &cppName) const;

private:
    // "Package.CppName ImportVersion" ->  QmlObjectValue
    QHash<QString, const QmlObjectValue *> _objectsByQualifiedName;
    QHash<QString, QSet<LanguageUtils::FakeMetaObject::ConstPtr> > _fakeMetaObjectsByPackage;
    ValueOwner *_valueOwner;
};

class ConvertToNumber: protected ValueVisitor // ECMAScript ToInt()
{
public:
    ConvertToNumber(ValueOwner *valueOwner);

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
    ValueOwner *_valueOwner;
    const Value *_result;
};

class ConvertToString: protected ValueVisitor // ECMAScript ToString
{
public:
    ConvertToString(ValueOwner *valueOwner);

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
    ValueOwner *_valueOwner;
    const Value *_result;
};

class ConvertToObject: protected ValueVisitor // ECMAScript ToObject
{
public:
    ConvertToObject(ValueOwner *valueOwner);

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
    ValueOwner *_valueOwner;
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

// internal
class QMLJS_EXPORT QmlPrototypeReference: public Reference
{
public:
    QmlPrototypeReference(AST::UiQualifiedId *qmlTypeName, const Document *doc, ValueOwner *valueOwner);
    virtual ~QmlPrototypeReference();

    AST::UiQualifiedId *qmlTypeName() const;

private:    
    virtual const Value *value(ReferenceContext *referenceContext) const;

    AST::UiQualifiedId *_qmlTypeName;
    const Document *_doc;
};

class QMLJS_EXPORT ASTVariableReference: public Reference
{
    AST::VariableDeclaration *_ast;
    const Document *_doc;

public:
    ASTVariableReference(AST::VariableDeclaration *ast, const Document *doc, ValueOwner *valueOwner);
    virtual ~ASTVariableReference();

private:
    virtual const Value *value(ReferenceContext *referenceContext) const;
    virtual bool getSourceLocation(QString *fileName, int *line, int *column) const;
};

class QMLJS_EXPORT ASTFunctionValue: public FunctionValue
{
    AST::FunctionExpression *_ast;
    const Document *_doc;
    QList<QString> _argumentNames;

public:
    ASTFunctionValue(AST::FunctionExpression *ast, const Document *doc, ValueOwner *valueOwner);
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
    ASTPropertyReference(AST::UiPublicMember *ast, const Document *doc, ValueOwner *valueOwner);
    virtual ~ASTPropertyReference();

    AST::UiPublicMember *ast() const { return _ast; }
    QString onChangedSlotName() const { return _onChangedSlotName; }

    virtual bool getSourceLocation(QString *fileName, int *line, int *column) const;

private:
    virtual const Value *value(ReferenceContext *referenceContext) const;
};

class QMLJS_EXPORT ASTSignal: public FunctionValue
{
    AST::UiPublicMember *_ast;
    const Document *_doc;
    QString _slotName;

public:
    ASTSignal(AST::UiPublicMember *ast, const Document *doc, ValueOwner *valueOwner);
    virtual ~ASTSignal();

    AST::UiPublicMember *ast() const { return _ast; }
    QString slotName() const { return _slotName; }

    // FunctionValue interface
    virtual int argumentCount() const;
    virtual const Value *argument(int index) const;
    virtual QString argumentName(int index) const;

    // Value interface
    virtual bool getSourceLocation(QString *fileName, int *line, int *column) const;
};

class QMLJS_EXPORT ASTObjectValue: public ObjectValue
{
    AST::UiQualifiedId *_typeName;
    AST::UiObjectInitializer *_initializer;
    const Document *_doc;
    QList<ASTPropertyReference *> _properties;
    QList<ASTSignal *> _signals;
    ASTPropertyReference *_defaultPropertyRef;

public:
    ASTObjectValue(AST::UiQualifiedId *typeName,
                   AST::UiObjectInitializer *initializer,
                   const Document *doc,
                   ValueOwner *valueOwner);
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
    TypeScope(const Imports *imports, ValueOwner *valueOwner);

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
    JSImportScope(const Imports *imports, ValueOwner *valueOwner);

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
    Imports(ValueOwner *valueOwner);

    void append(const Import &import);

    ImportInfo info(const QString &name, const Context *context) const;
    QString nameForImportedObject(const ObjectValue *value, const Context *context) const;
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

} // namespace QmlJS

#endif // QMLJS_INTERPRETER_H
