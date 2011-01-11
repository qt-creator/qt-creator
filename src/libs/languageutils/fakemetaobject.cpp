/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "fakemetaobject.h"

using namespace LanguageUtils;

FakeMetaEnum::FakeMetaEnum(const QString &name)
    : m_name(name)
{}

QString FakeMetaEnum::name() const
{ return m_name; }

void FakeMetaEnum::addKey(const QString &key, int value)
{ m_keys.append(key); m_values.append(value); }

QString FakeMetaEnum::key(int index) const
{ return m_keys.at(index); }

int FakeMetaEnum::keyCount() const
{ return m_keys.size(); }

QStringList FakeMetaEnum::keys() const
{ return m_keys; }

FakeMetaMethod::FakeMetaMethod(const QString &name, const QString &returnType)
    : m_name(name)
    , m_returnType(returnType)
    , m_methodTy(FakeMetaMethod::Method)
    , m_methodAccess(FakeMetaMethod::Public)
{}

QString FakeMetaMethod::methodName() const
{ return m_name; }

QStringList FakeMetaMethod::parameterNames() const
{ return m_paramNames; }

QStringList FakeMetaMethod::parameterTypes() const
{ return m_paramTypes; }

void FakeMetaMethod::addParameter(const QString &name, const QString &type)
{ m_paramNames.append(name); m_paramTypes.append(type); }

int FakeMetaMethod::methodType() const
{ return m_methodTy; }

void FakeMetaMethod::setMethodType(int methodType)
{ m_methodTy = methodType; }

int FakeMetaMethod::access() const
{ return m_methodAccess; }


FakeMetaProperty::FakeMetaProperty(const QString &name, const QString &type, bool isList, bool isWritable, bool isPointer)
    : m_propertyName(name), m_type(type), m_isList(isList), m_isWritable(isWritable), m_isPointer(isPointer)
{}

QString FakeMetaProperty::name() const
{ return m_propertyName; }

QString FakeMetaProperty::typeName() const
{ return m_type; }

bool FakeMetaProperty::isList() const
{ return m_isList; }

bool FakeMetaProperty::isWritable() const
{ return m_isWritable; }

bool FakeMetaProperty::isPointer() const
{ return m_isPointer; }


FakeMetaObject::FakeMetaObject()
    : m_super(0)
{
}

void FakeMetaObject::addExport(const QString &name, const QString &package, ComponentVersion version)
{
    Export exp;
    exp.type = name;
    exp.package = package;
    exp.version = version;
    exp.packageNameVersion = QString::fromLatin1("%1.%2 %3.%4").arg(
                package, name,
                QString::number(version.majorVersion()),
                QString::number(version.minorVersion()));
    m_exports.append(exp);
}
QList<FakeMetaObject::Export> FakeMetaObject::exports() const
{ return m_exports; }

void FakeMetaObject::setSuperclassName(const QString &superclass)
{ m_superName = superclass; }
QString FakeMetaObject::superclassName() const
{ return m_superName; }

void FakeMetaObject::setSuperclass(ConstPtr superClass)
{ m_super = superClass; }
FakeMetaObject::ConstPtr FakeMetaObject::superClass() const
{ return m_super; }

void FakeMetaObject::addEnum(const FakeMetaEnum &fakeEnum)
{ m_enumNameToIndex.insert(fakeEnum.name(), m_enums.size()); m_enums.append(fakeEnum); }
int FakeMetaObject::enumeratorCount() const
{ return m_enums.size(); }
int FakeMetaObject::enumeratorOffset() const
{ return 0; }
FakeMetaEnum FakeMetaObject::enumerator(int index) const
{ return m_enums.at(index); }
int FakeMetaObject::enumeratorIndex(const QString &name) const
{ return m_enumNameToIndex.value(name, -1); }

void FakeMetaObject::addProperty(const FakeMetaProperty &property)
{ m_propNameToIdx.insert(property.name(), m_props.size()); m_props.append(property); }
int FakeMetaObject::propertyCount() const
{ return m_props.size(); }
int FakeMetaObject::propertyOffset() const
{ return 0; }
FakeMetaProperty FakeMetaObject::property(int index) const
{ return m_props.at(index); }
int FakeMetaObject::propertyIndex(const QString &name) const
{ return m_propNameToIdx.value(name, -1); }

void FakeMetaObject::addMethod(const FakeMetaMethod &method)
{ m_methods.append(method); }
int FakeMetaObject::methodCount() const
{ return m_methods.size(); }
int FakeMetaObject::methodOffset() const
{ return 0; }
FakeMetaMethod FakeMetaObject::method(int index) const
{ return m_methods.at(index); }

QString FakeMetaObject::defaultPropertyName() const
{ return m_defaultPropertyName; }

void FakeMetaObject::setDefaultPropertyName(const QString defaultPropertyName)
{ m_defaultPropertyName = defaultPropertyName; }
