/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** 
** Non-Open Source Usage  
** 
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.  
** 
** GNU General Public License Usage 
** 
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/
/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

/* This file is part of KDevelop
Copyright (C) 2002-2005 Roberto Raggi <roberto@kdevelop.org>
Copyright (C) 2005 Trolltech AS

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License version 2 as published by the Free Software Foundation.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public License
along with this library; see the file COPYING.LIB.  If not, write to
the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
Boston, MA 02110-1301, USA.
*/

#ifndef CODEMODELITEMS_H
#define CODEMODELITEMS_H

#include <QtCore/QStringList>

class CodeModelItem;
class ArgumentModelItem;
class ClassModelItem;
class EnumModelItem;
class EnumeratorModelItem;
class FileModelItem;
class FunctionDefinitionModelItem;
class FunctionModelItem;
class NamespaceModelItem;
class TemplateParameterModelItem;
class TypeAliasModelItem;
class VariableModelItem;
class TypeInfo;

typedef QList<ArgumentModelItem*> ArgumentList;
typedef QList<ClassModelItem*> ClassList;
typedef QList<EnumModelItem*> EnumList;
typedef QList<EnumeratorModelItem*> EnumeratorList;
typedef QList<FileModelItem*> FileList;
typedef QList<FunctionDefinitionModelItem*> FunctionDefinitionList;
typedef QList<FunctionModelItem*> FunctionList;
typedef QList<NamespaceModelItem*> NamespaceList;
typedef QList<TemplateParameterModelItem*> TemplateParameterList;
typedef QList<TypeAliasModelItem*> TypeAliasList;
typedef QList<VariableModelItem*> VariableList;
typedef QList<TypeInfo*> TypeInfoList;

#define DECLARE_MODEL_NODE(name) \
    enum { __node_kind = Kind_##name };


template <class T> inline T model_cast(CodeModelItem *item) { return 0; }

#define DECLARE_MODELITEM(name) \
    template <> inline name##ModelItem *model_cast<##name##ModelItem *>(CodeModelItem *item) { \
    if (item && item->kind() == CodeModelItem::Kind_##name) \
        return static_cast<##name##ModelItem *>(item); \
    return 0; }

class CodeModel
{
public:
    enum AccessPolicy
    {
        Public,
        Protected,
        Private
    };

    enum FunctionType
    {
        Normal,
        Signal,
        Slot
    };

    enum ClassType
    {
        Class,
        Struct,
        Union
    };

public:
    virtual ~CodeModel();

/*    virtual FileList files() const = 0;
    virtual NamespaceModelItem *globalNamespace() const = 0; */
};

class TypeInfo
{
public:
    TypeInfo(): flags (0) {}
    virtual ~TypeInfo() { qDeleteAll(m_arguments); }

    QString qualifiedName() const { return m_qualifiedName; }
    void setQualifiedName(const QString &qualified_name) { m_qualifiedName = qualified_name; }

    bool isConstant() const { return m_constant; }
    void setConstant(bool is) { m_constant = is; }

    bool isVolatile() const { return m_volatile; }
    void setVolatile(bool is) { m_volatile = is; }

    bool isReference() const { return m_reference; }
    void setReference(bool is) { m_reference = is; }

    int indirections() const { return m_indirections; }
    void setIndirections(int indirections) { m_indirections = indirections; }

    bool isFunctionPointer() const { return m_functionPointer; }
    void setFunctionPointer(bool is) { m_functionPointer = is; }

    QStringList arrayElements() const { return m_arrayElements; }
    void setArrayElements(const QStringList &arrayElements) { m_arrayElements = arrayElements; }

    TypeInfoList arguments() const { return m_arguments; }
    void addArgument(TypeInfo *arg) { m_arguments.append(arg); }

    bool operator==(const TypeInfo &other);
    bool operator!=(const TypeInfo &other) { return !(*this==other); }

private:
    union
    {
        uint flags;

        struct
        {
            uint m_constant: 1;
            uint m_volatile: 1;
            uint m_reference: 1;
            uint m_functionPointer: 1;
            uint m_indirections: 6;
            uint m_padding: 22;
        };
    };

    QString m_qualifiedName;
    QStringList m_arrayElements;
    TypeInfoList m_arguments;
};

class CodeModelItemData;
class CodeModelItem
{
public:
    enum Kind
    {
        /* These are bit-flags resembling inheritance */
        Kind_Scope = 0x1,
        Kind_Namespace = 0x2 | Kind_Scope,
        Kind_Member = 0x4,
        Kind_Function = 0x8 | Kind_Member,
        KindMask = 0xf,

        /* These are for classes that are not inherited from */
        FirstKind = 0x8,
        Kind_Argument = 1 << FirstKind,
        Kind_Class = 2 << FirstKind | Kind_Scope,
        Kind_Enum = 3 << FirstKind,
        Kind_Enumerator = 4 << FirstKind,
        Kind_File = 5 << FirstKind | Kind_Namespace,
        Kind_FunctionDefinition = 6 << FirstKind | Kind_Function,
        Kind_TemplateParameter = 7 << FirstKind,
        Kind_TypeAlias = 8 << FirstKind,
        Kind_Variable = 9 << FirstKind | Kind_Member
    };

    CodeModelItem(CodeModel *model, int kind);
    CodeModelItem(const CodeModelItem &item);
    virtual ~CodeModelItem();

    int kind() const;

    QString qualifiedName() const;

    QString name() const;
    void setName(const QString &name);

    QString scope() const;
    void setScope(const QString &scope);

    void setFile(FileModelItem *file);
    FileModelItem *file() const;

    void startPosition(int *line, int *column);
    void setStartPosition(int line, int column);

    void endPosition(int *line, int *column);
    void setEndPosition(int line, int column);

    std::size_t creationId() const;
    void setCreationId(std::size_t creation_id);

    CodeModel *model() const;

protected:
    void setKind(int kind);

private:
    CodeModelItemData *d;
};

class ScopeModelItemData;
class ScopeModelItem: public CodeModelItem
{
public:
    DECLARE_MODEL_NODE(Scope)
    ScopeModelItem(CodeModel *model, int kind = __node_kind);
    virtual ~ScopeModelItem();

    void addClass(ClassModelItem *item);
    void addEnum(EnumModelItem *item);
    void addFunction(FunctionModelItem *item);
    void addFunctionDefinition(FunctionDefinitionModelItem *item);
    void addTypeAlias(TypeAliasModelItem *item);
    void addVariable(VariableModelItem *item);

private:
    ScopeModelItemData *d;
};
DECLARE_MODELITEM(Scope)

class ClassModelItemData;
class ClassModelItem: public ScopeModelItem
{
public:
    DECLARE_MODEL_NODE(Class)
    ClassModelItem(CodeModel *model, int kind = __node_kind);
    virtual ~ClassModelItem();

    QStringList baseClasses() const;

    void setBaseClasses(const QStringList &baseClasses);
    void addBaseClass(const QString &baseClass);

    TemplateParameterList templateParameters() const;
    void setTemplateParameters(const TemplateParameterList &templateParameters);

    bool extendsClass(const QString &name) const;

    void setClassType(CodeModel::ClassType type);
    CodeModel::ClassType classType() const;

private:
    ClassModelItemData *d;
};
DECLARE_MODELITEM(Class)

class NamespaceModelItemData;
class NamespaceModelItem: public ScopeModelItem
{
public:
    DECLARE_MODEL_NODE(Namespace)
    NamespaceModelItem(CodeModel *model, int kind = __node_kind);
    virtual ~NamespaceModelItem();

    NamespaceList namespaces() const;
    void addNamespace(NamespaceModelItem *item);
    NamespaceModelItem *findNamespace(const QString &name) const;

private:
    NamespaceModelItemData *d;
};
DECLARE_MODELITEM(Namespace)

class FileModelItem: public NamespaceModelItem
{
public:
    DECLARE_MODEL_NODE(File)
    FileModelItem(CodeModel *model, int kind = __node_kind);
    virtual ~FileModelItem();
};
DECLARE_MODELITEM(File)

class ArgumentModelItemData;
class ArgumentModelItem: public CodeModelItem
{
public:
    DECLARE_MODEL_NODE(Argument)
    ArgumentModelItem(CodeModel *model, int kind = __node_kind);
    virtual ~ArgumentModelItem();

public:
    TypeInfo *type() const;
    void setType(TypeInfo *type);

    bool defaultValue() const;
    void setDefaultValue(bool defaultValue);

    QString defaultValueExpression() const;
    void setDefaultValueExpression(const QString &expr);

private:
    ArgumentModelItemData *d;
};
DECLARE_MODELITEM(Argument)

class MemberModelItemData;
class MemberModelItem: public CodeModelItem
{
public:
    DECLARE_MODEL_NODE(Member)
    MemberModelItem(CodeModel *model, int kind);
    virtual ~MemberModelItem();

    bool isConstant() const;
    void setConstant(bool isConstant);

    bool isVolatile() const;
    void setVolatile(bool isVolatile);

    bool isStatic() const;
    void setStatic(bool isStatic);

    bool isAuto() const;
    void setAuto(bool isAuto);

    bool isFriend() const;
    void setFriend(bool isFriend);

    bool isRegister() const;
    void setRegister(bool isRegister);

    bool isExtern() const;
    void setExtern(bool isExtern);

    bool isMutable() const;
    void setMutable(bool isMutable);

    CodeModel::AccessPolicy accessPolicy() const;
    void setAccessPolicy(CodeModel::AccessPolicy accessPolicy);

    TemplateParameterList templateParameters() const;
    void setTemplateParameters(const TemplateParameterList &templateParameters);

    TypeInfo *type() const;
    void setType(TypeInfo *type);

private:
    MemberModelItemData *d;
};
DECLARE_MODELITEM(Member)

class FunctionModelItemData;
class FunctionModelItem: public MemberModelItem
{
public:
    DECLARE_MODEL_NODE(Function)
    FunctionModelItem(CodeModel *model, int kind = __node_kind);
    virtual ~FunctionModelItem();

    ArgumentList arguments() const;

    void addArgument(ArgumentModelItem *item);

    CodeModel::FunctionType functionType() const;
    void setFunctionType(CodeModel::FunctionType functionType);

    bool isVirtual() const;
    void setVirtual(bool isVirtual);

    bool isInline() const;
    void setInline(bool isInline);

    bool isExplicit() const;
    void setExplicit(bool isExplicit);

    bool isAbstract() const;
    void setAbstract(bool isAbstract);

    bool isVariadics() const;
    void setVariadics(bool isVariadics);

    bool isSimilar(FunctionModelItem *other) const;

private:
    FunctionModelItemData *d;
};
DECLARE_MODELITEM(Function)

class FunctionDefinitionModelItem: public FunctionModelItem
{
public:
    DECLARE_MODEL_NODE(FunctionDefinition)
    FunctionDefinitionModelItem(CodeModel *model, int kind = __node_kind);
    virtual ~FunctionDefinitionModelItem();
};
DECLARE_MODELITEM(FunctionDefinition)

class VariableModelItem: public MemberModelItem
{
public:
    DECLARE_MODEL_NODE(Variable)
    VariableModelItem(CodeModel *model, int kind = __node_kind);
    virtual ~VariableModelItem();
};
DECLARE_MODELITEM(Variable)

class TypeAliasModelItemData;
class TypeAliasModelItem: public CodeModelItem
{
public:
    DECLARE_MODEL_NODE(TypeAlias)
    TypeAliasModelItem(CodeModel *model, int kind = __node_kind);
    virtual ~TypeAliasModelItem();

    TypeInfo *type() const;
    void setType(TypeInfo *type);

private:
    TypeAliasModelItemData *d;
};
DECLARE_MODELITEM(TypeAlias)

class EnumModelItemData;
class EnumModelItem: public CodeModelItem
{
public:
    DECLARE_MODEL_NODE(Enum)
    EnumModelItem(CodeModel *model, int kind = __node_kind);
    virtual ~EnumModelItem();

    CodeModel::AccessPolicy accessPolicy() const;
    void setAccessPolicy(CodeModel::AccessPolicy accessPolicy);

    EnumeratorList enumerators() const;
    void addEnumerator(EnumeratorModelItem *item);

private:
    EnumModelItemData *d;
};
DECLARE_MODELITEM(Enum)

class EnumeratorModelItemData;
class EnumeratorModelItem: public CodeModelItem
{
public:
    DECLARE_MODEL_NODE(Enumerator)
    EnumeratorModelItem(CodeModel *model, int kind = __node_kind);
    virtual ~EnumeratorModelItem();

    QString value() const;
    void setValue(const QString &value);

private:
    EnumeratorModelItemData *d;
};
DECLARE_MODELITEM(Enumerator)

class TemplateParameterModelItemData;
class TemplateParameterModelItem: public CodeModelItem
{
public:
    DECLARE_MODEL_NODE(TemplateParameter)
    TemplateParameterModelItem(CodeModel *model, int kind = __node_kind);
    TemplateParameterModelItem(const TemplateParameterModelItem& item);
    virtual ~TemplateParameterModelItem();

    TypeInfo *type() const;
    void setType(TypeInfo *type);

    bool defaultValue() const;
    void setDefaultValue(bool defaultValue);

private:
    TemplateParameterModelItemData *d;
};
DECLARE_MODELITEM(TemplateParameter)

// ### todo, check language
#define DECLARE_LANGUAGE_MODELITEM(name, language) \
    template <> inline language##name##ModelItem *model_cast<##language##name##ModelItem *>(CodeModelItem *item) { \
    if (item && item->kind() == CodeModelItem::Kind_##name) \
        return static_cast<##language##name##ModelItem *>(item); \
    return 0; }

// ### todo, check language
template <class T> inline T model_cast(CodeModel *item) { return 0; }

#define DECLARE_LANGUAGE_CODEMODEL(language) \
    template <> inline language##CodeModel *model_cast<##language##CodeModel *>(CodeModel *item) { \
    return item ? static_cast<##language##CodeModel *>(item) : 0; }
    
#endif //CODEMODELITEMS_H

// kate: space-indent on; indent-width 2; replace-tabs on;
