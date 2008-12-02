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

#include <QtCore/QHash>
#include "codemodelitems.h"

CodeModel::~CodeModel()
{

}

// ---------------------------------------------------------------------------
bool TypeInfo::operator==(const TypeInfo &other)
{
    if (arrayElements().count() != other.arguments().count())
        return false;

    return flags == other.flags
        && m_qualifiedName == other.m_qualifiedName
        && (!m_functionPointer || m_arguments == other.m_arguments);
}

// ---------------------------------------------------------------------------
class CodeModelItemData {
public:
    CodeModel *_M_model;
    int _M_kind;
    int _M_startLine;
    int _M_startColumn;
    int _M_endLine;
    int _M_endColumn;
    std::size_t _M_creation_id;
    QString _M_name;
    FileModelItem *_M_file;
    QString _M_scope;
};

CodeModelItem::CodeModelItem(CodeModel *model, int kind)
{
    d = new CodeModelItemData;
    d->_M_model = model;
    d->_M_kind = kind;
    d->_M_startLine = 0;
    d->_M_startColumn = 0;
    d->_M_endLine = 0;
    d->_M_endColumn = 0;
    d->_M_creation_id = 0;
}

CodeModelItem::CodeModelItem(const CodeModelItem &item)
{
    d = new CodeModelItemData;
    *d = *(item.d);
}

CodeModelItem::~CodeModelItem()
{
    delete d;
}

int CodeModelItem::kind() const
{
    return d->_M_kind;
}

void CodeModelItem::setKind(int kind)
{
    d->_M_kind = kind;
}

QString CodeModelItem::qualifiedName() const
{
    if (kind() == CodeModelItem::Kind_File)
        return QString();

    QString q = scope();

    if (!q.isEmpty() && !name().isEmpty())
        q += QLatin1String("::");
   
    q += name();

    return q;
}

QString CodeModelItem::name() const
{
    return d->_M_name;
}

void CodeModelItem::setName(const QString &name)
{
    d->_M_name = name;
}

QString CodeModelItem::scope() const
{
    return d->_M_scope;
}

void CodeModelItem::setScope(const QString &scope)
{
    d->_M_scope = scope;
}

void CodeModelItem::setFile(FileModelItem *file)
{
    d->_M_file = file;
}

FileModelItem *CodeModelItem::file() const
{
    return d->_M_file;
}

void CodeModelItem::startPosition(int *line, int *column)
{
    *line = d->_M_startLine;
    *column = d->_M_startColumn;
}

void CodeModelItem::setStartPosition(int line, int column)
{
    d->_M_startLine = line;
    d->_M_startColumn = column;
}

void CodeModelItem::endPosition(int *line, int *column)
{
    *line = d->_M_endLine;
    *column = d->_M_endColumn;
}

void CodeModelItem::setEndPosition(int line, int column)
{
    d->_M_endLine = line;
    d->_M_endColumn = column;
}

std::size_t CodeModelItem::creationId() const
{
    return d->_M_creation_id;
}

void CodeModelItem::setCreationId(std::size_t creation_id)
{
    d->_M_creation_id = creation_id;
}

CodeModel *CodeModelItem::model() const
{ 
    return d->_M_model;
}

// ---------------------------------------------------------------------------
class ClassModelItemData
{
public:
    ~ClassModelItemData() {
        qDeleteAll(_M_templateParameters);
    }
    QStringList _M_baseClasses;
    TemplateParameterList _M_templateParameters;
    CodeModel::ClassType _M_classType;
};

ClassModelItem::ClassModelItem(CodeModel *model, int kind)
: ScopeModelItem(model, kind)
{
    d = new ClassModelItemData();
    d->_M_classType = CodeModel::Class;
}

ClassModelItem::~ClassModelItem()
{
    delete d;
}

QStringList ClassModelItem::baseClasses() const
{
    return d->_M_baseClasses;
}

void ClassModelItem::setBaseClasses(const QStringList &baseClasses)
{
    d->_M_baseClasses = baseClasses;
}

TemplateParameterList ClassModelItem::templateParameters() const
{
    return d->_M_templateParameters;
}

void ClassModelItem::setTemplateParameters(const TemplateParameterList &templateParameters)
{
    d->_M_templateParameters = templateParameters;
}

void ClassModelItem::addBaseClass(const QString &baseClass)
{
    d->_M_baseClasses.append(baseClass);
}

bool ClassModelItem::extendsClass(const QString &name) const
{
    return d->_M_baseClasses.contains(name);
}

void ClassModelItem::setClassType(CodeModel::ClassType type)
{
    d->_M_classType = type;
}

CodeModel::ClassType ClassModelItem::classType() const
{
    return d->_M_classType;
}

// ---------------------------------------------------------------------------
class ScopeModelItemData
{
public:
    ~ScopeModelItemData() {
        qDeleteAll(_M_classes.values());
        qDeleteAll(_M_enums.values());
        qDeleteAll(_M_typeAliases.values());
        qDeleteAll(_M_variables.values());
        qDeleteAll(_M_functionDefinitions.values());
        qDeleteAll(_M_functions.values());
    }

    QHash<QString, ClassModelItem*> _M_classes;
    QHash<QString, EnumModelItem*> _M_enums;
    QHash<QString, TypeAliasModelItem*> _M_typeAliases;
    QHash<QString, VariableModelItem*> _M_variables;
    QMultiHash<QString, FunctionDefinitionModelItem*> _M_functionDefinitions;
    QMultiHash<QString, FunctionModelItem*> _M_functions;
};

ScopeModelItem::ScopeModelItem(CodeModel *model, int kind)
    : CodeModelItem(model, kind) 
{
    d = new ScopeModelItemData;
}

ScopeModelItem::~ScopeModelItem()
{
    delete d;
}

void ScopeModelItem::addClass(ClassModelItem *item)
{
    d->_M_classes.insert(item->name(), item);
}

void ScopeModelItem::addFunction(FunctionModelItem *item)
{
    d->_M_functions.insert(item->name(), item);
}

void ScopeModelItem::addFunctionDefinition(FunctionDefinitionModelItem *item)
{
    d->_M_functionDefinitions.insert(item->name(), item);
}

void ScopeModelItem::addVariable(VariableModelItem *item)
{
    d->_M_variables.insert(item->name(), item);
}

void ScopeModelItem::addTypeAlias(TypeAliasModelItem *item)
{
    d->_M_typeAliases.insert(item->name(), item);
}

void ScopeModelItem::addEnum(EnumModelItem *item)
{
    d->_M_enums.insert(item->name(), item);
}

// ---------------------------------------------------------------------------
class NamespaceModelItemData
{
public:
    ~NamespaceModelItemData() {
        qDeleteAll(_M_namespaces.values());
    }

    QHash<QString, NamespaceModelItem *> _M_namespaces;
};

NamespaceModelItem::NamespaceModelItem(CodeModel *model, int kind)
: ScopeModelItem(model, kind)
{
    d = new NamespaceModelItemData();
}

NamespaceModelItem::~NamespaceModelItem()
{
    delete d;
}

NamespaceList NamespaceModelItem::namespaces() const
{
    return d->_M_namespaces.values();
}
void NamespaceModelItem::addNamespace(NamespaceModelItem *item)
{
    d->_M_namespaces.insert(item->name(), item);
}

// ---------------------------------------------------------------------------
class ArgumentModelItemData 
{
public:
    ~ArgumentModelItemData() {
        delete _M_type;
    }
    TypeInfo *_M_type;
    QString _M_defaultValueExpression;
    bool _M_defaultValue;
};

ArgumentModelItem::ArgumentModelItem(CodeModel *model, int kind)
: CodeModelItem(model, kind)
{
    d = new ArgumentModelItemData();
    d->_M_defaultValue = false;
}

ArgumentModelItem::~ArgumentModelItem()
{
    delete d;
}

TypeInfo *ArgumentModelItem::type() const
{
    return d->_M_type;
}

void ArgumentModelItem::setType(TypeInfo *type)
{
    d->_M_type = type;
}

bool ArgumentModelItem::defaultValue() const
{
    return d->_M_defaultValue;
}

void ArgumentModelItem::setDefaultValue(bool defaultValue)
{
    d->_M_defaultValue = defaultValue;
}

QString ArgumentModelItem::defaultValueExpression() const
{
    return d->_M_defaultValueExpression;
}

void ArgumentModelItem::setDefaultValueExpression(const QString &expr)
{
    d->_M_defaultValueExpression = expr;
}

// ---------------------------------------------------------------------------
class FunctionModelItemData
{
public:
    ~FunctionModelItemData() {
        qDeleteAll(_M_arguments);
    }

    ArgumentList _M_arguments;
    CodeModel::FunctionType _M_functionType;
    union
    {
        struct
        {
            uint _M_isVirtual: 1;
            uint _M_isInline: 1;
            uint _M_isAbstract: 1;
            uint _M_isExplicit: 1;
            uint _M_isVariadics: 1;
        };
        uint _M_flags;
    };
};

FunctionModelItem::FunctionModelItem(CodeModel *model, int kind)
: MemberModelItem(model, kind)
{
    d = new FunctionModelItemData();
    d->_M_functionType = CodeModel::Normal;
    d->_M_flags = 0;
}

FunctionModelItem::~FunctionModelItem()
{
    delete d;
}

bool FunctionModelItem::isSimilar(FunctionModelItem *other) const
{
    if (name() != other->name())
        return false;

    if (isConstant() != other->isConstant())
        return false;

    if (isVariadics() != other->isVariadics())
        return false;

    if (arguments().count() != other->arguments().count())
        return false;

    // ### check the template parameters

    for (int i=0; i<arguments().count(); ++i)
    {
        ArgumentModelItem *arg1 = arguments().at(i);
        ArgumentModelItem *arg2 = other->arguments().at(i);

        TypeInfo *t1 = static_cast<TypeInfo *>(arg1->type());
        TypeInfo *t2 = static_cast<TypeInfo *>(arg2->type());

        if (*t1 != *t2)
            return false;
    }

    return true;
}

ArgumentList FunctionModelItem::arguments() const
{
    return d->_M_arguments;
}

void FunctionModelItem::addArgument(ArgumentModelItem *item)
{
    d->_M_arguments.append(item);
}

CodeModel::FunctionType FunctionModelItem::functionType() const
{
    return d->_M_functionType;
}

void FunctionModelItem::setFunctionType(CodeModel::FunctionType functionType)
{
    d->_M_functionType = functionType;
}

bool FunctionModelItem::isVariadics() const
{
    return d->_M_isVariadics;
}

void FunctionModelItem::setVariadics(bool isVariadics)
{
    d->_M_isVariadics = isVariadics;
}

bool FunctionModelItem::isVirtual() const
{
    return d->_M_isVirtual;
}

void FunctionModelItem::setVirtual(bool isVirtual)
{
    d->_M_isVirtual = isVirtual;
}

bool FunctionModelItem::isInline() const
{
    return d->_M_isInline;
}

void FunctionModelItem::setInline(bool isInline)
{
    d->_M_isInline = isInline;
}

bool FunctionModelItem::isExplicit() const
{
    return d->_M_isExplicit;
}

void FunctionModelItem::setExplicit(bool isExplicit)
{
    d->_M_isExplicit = isExplicit;
}

bool FunctionModelItem::isAbstract() const
{
    return d->_M_isAbstract;
}

void FunctionModelItem::setAbstract(bool isAbstract)
{
    d->_M_isAbstract = isAbstract;
}

// ---------------------------------------------------------------------------
class TypeAliasModelItemData
{
public:
    ~TypeAliasModelItemData() {
        delete _M_type;
    }

    TypeInfo *_M_type;
};

TypeAliasModelItem::TypeAliasModelItem(CodeModel *model, int kind)
: CodeModelItem(model, kind)
{
    d = new TypeAliasModelItemData;
}

TypeAliasModelItem::~TypeAliasModelItem()
{
    delete d;
}

TypeInfo *TypeAliasModelItem::type() const
{
    return d->_M_type;
}

void TypeAliasModelItem::setType(TypeInfo *type)
{
    d->_M_type = type;
}

// ---------------------------------------------------------------------------
class EnumModelItemData
{
public:
    ~EnumModelItemData() {
        qDeleteAll(_M_enumerators);
    }
    CodeModel::AccessPolicy _M_accessPolicy;
    EnumeratorList _M_enumerators;
};

EnumModelItem::EnumModelItem(CodeModel *model, int kind)
: CodeModelItem(model, kind)
{
    d = new EnumModelItemData;
    d->_M_accessPolicy = CodeModel::Public;
}

EnumModelItem::~EnumModelItem()
{
    delete d;
}

CodeModel::AccessPolicy EnumModelItem::accessPolicy() const
{
    return d->_M_accessPolicy;
}

void EnumModelItem::setAccessPolicy(CodeModel::AccessPolicy accessPolicy)
{
    d->_M_accessPolicy = accessPolicy;
}

EnumeratorList EnumModelItem::enumerators() const
{
    return d->_M_enumerators;
}

void EnumModelItem::addEnumerator(EnumeratorModelItem *item)
{
    d->_M_enumerators.append(item);
}

// ---------------------------------------------------------------------------
class EnumeratorModelItemData
{
public:
    QString _M_value;
};

EnumeratorModelItem::EnumeratorModelItem(CodeModel *model, int kind)
: CodeModelItem(model, kind)
{
    d = new EnumeratorModelItemData;
}

EnumeratorModelItem::~EnumeratorModelItem()
{
    delete d;
}

QString EnumeratorModelItem::value() const
{
    return d->_M_value;
}

void EnumeratorModelItem::setValue(const QString &value)
{
    d->_M_value = value;
}

// ---------------------------------------------------------------------------
class TemplateParameterModelItemData
{
public:
    ~TemplateParameterModelItemData() {
        delete _M_type;
    }

    TypeInfo *_M_type;
    bool _M_defaultValue;
};

TemplateParameterModelItem::TemplateParameterModelItem(CodeModel *model, int kind)
: CodeModelItem(model, kind)
{
    d = new TemplateParameterModelItemData;
    d->_M_defaultValue = false;
    d->_M_type = 0;
}

TemplateParameterModelItem::TemplateParameterModelItem(const TemplateParameterModelItem& item)
    : CodeModelItem(item)
{
    d = new TemplateParameterModelItemData;
    *d = *(item.d);
}

TemplateParameterModelItem::~TemplateParameterModelItem()
{
    delete d;
}

TypeInfo *TemplateParameterModelItem::type() const
{
    return d->_M_type;
}

void TemplateParameterModelItem::setType(TypeInfo *type)
{
    d->_M_type = type;
}

bool TemplateParameterModelItem::defaultValue() const
{
    return d->_M_defaultValue;
}

void TemplateParameterModelItem::setDefaultValue(bool defaultValue)
{
    d->_M_defaultValue = defaultValue;
}

// ---------------------------------------------------------------------------
FileModelItem::FileModelItem(CodeModel *model, int kind)
: NamespaceModelItem(model, kind)
{
}

FileModelItem::~FileModelItem()
{
}

FunctionDefinitionModelItem::FunctionDefinitionModelItem(CodeModel *model, int kind)
: FunctionModelItem(model, kind)
{

}

FunctionDefinitionModelItem::~FunctionDefinitionModelItem()
{

}

VariableModelItem::VariableModelItem(CodeModel *model, int kind)
: MemberModelItem(model, kind)
{

}

VariableModelItem::~VariableModelItem()
{

}

// ---------------------------------------------------------------------------
class MemberModelItemData
{
public:
    ~MemberModelItemData() {
        delete _M_type;
        qDeleteAll(_M_templateParameters);
    }
    TemplateParameterList _M_templateParameters;
    TypeInfo *_M_type;
    CodeModel::AccessPolicy _M_accessPolicy;
    union
    {
        struct
        {
            uint _M_isConstant: 1;
            uint _M_isVolatile: 1;
            uint _M_isStatic: 1;
            uint _M_isAuto: 1;
            uint _M_isFriend: 1;
            uint _M_isRegister: 1;
            uint _M_isExtern: 1;
            uint _M_isMutable: 1;
        };
        uint _M_flags;
    };
};

MemberModelItem::MemberModelItem(CodeModel *model, int kind)
: CodeModelItem(model, kind)
{
    d = new MemberModelItemData();
    d->_M_accessPolicy = CodeModel::Public;
    d->_M_flags = 0;
}

MemberModelItem::~MemberModelItem()
{
    delete d;
}

TypeInfo *MemberModelItem::type() const
{
    return d->_M_type;
}

void MemberModelItem::setType(TypeInfo *type)
{
    d->_M_type = type;
}

CodeModel::AccessPolicy MemberModelItem::accessPolicy() const
{
    return d->_M_accessPolicy;
}

void MemberModelItem::setAccessPolicy(CodeModel::AccessPolicy accessPolicy)
{
    d->_M_accessPolicy = accessPolicy;
}

bool MemberModelItem::isStatic() const
{
    return d->_M_isStatic;
}

void MemberModelItem::setStatic(bool isStatic)
{
    d->_M_isStatic = isStatic;
}

bool MemberModelItem::isConstant() const
{
    return d->_M_isConstant;
}

void MemberModelItem::setConstant(bool isConstant)
{
    d->_M_isConstant = isConstant;
}

bool MemberModelItem::isVolatile() const
{
    return d->_M_isVolatile;
}

void MemberModelItem::setVolatile(bool isVolatile)
{
    d->_M_isVolatile = isVolatile;
}

bool MemberModelItem::isAuto() const
{
    return d->_M_isAuto;
}

void MemberModelItem::setAuto(bool isAuto)
{
    d->_M_isAuto = isAuto;
}

bool MemberModelItem::isFriend() const
{
    return d->_M_isFriend;
}

void MemberModelItem::setFriend(bool isFriend)
{
    d->_M_isFriend = isFriend;
}

bool MemberModelItem::isRegister() const
{
    return d->_M_isRegister;
}

void MemberModelItem::setRegister(bool isRegister)
{
    d->_M_isRegister = isRegister;
}

bool MemberModelItem::isExtern() const
{
    return d->_M_isExtern;
}

void MemberModelItem::setExtern(bool isExtern)
{
    d->_M_isExtern = isExtern;
}

bool MemberModelItem::isMutable() const
{
    return d->_M_isMutable;
}

void MemberModelItem::setMutable(bool isMutable)
{
    d->_M_isMutable = isMutable;
}

TemplateParameterList MemberModelItem::templateParameters() const
{
    return d->_M_templateParameters;
}

void MemberModelItem::setTemplateParameters(const TemplateParameterList &templateParameters)
{
    d->_M_templateParameters = templateParameters;
}

// kate: space-indent on; indent-width 2; replace-tabs on;

