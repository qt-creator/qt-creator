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

#include "fakemetaobject.h"

using namespace LanguageUtils;

FakeMetaEnum::FakeMetaEnum()
{}

FakeMetaEnum::FakeMetaEnum(const QString &name)
    : m_name(name)
{}

bool FakeMetaEnum::isValid() const
{ return !m_name.isEmpty(); }

QString FakeMetaEnum::name() const
{ return m_name; }

void FakeMetaEnum::setName(const QString &name)
{ m_name = name; }

void FakeMetaEnum::addKey(const QString &key, int value)
{ m_keys.append(key); m_values.append(value); }

QString FakeMetaEnum::key(int index) const
{ return m_keys.at(index); }

int FakeMetaEnum::keyCount() const
{ return m_keys.size(); }

QStringList FakeMetaEnum::keys() const
{ return m_keys; }

bool FakeMetaEnum::hasKey(const QString &key) const
{ return m_keys.contains(key); }

FakeMetaMethod::FakeMetaMethod(const QString &name, const QString &returnType)
    : m_name(name)
    , m_returnType(returnType)
    , m_methodTy(FakeMetaMethod::Method)
    , m_methodAccess(FakeMetaMethod::Public)
    , m_revision(0)
{}

FakeMetaMethod::FakeMetaMethod()
    : m_methodTy(FakeMetaMethod::Method)
    , m_methodAccess(FakeMetaMethod::Public)
    , m_revision(0)
{}

QString FakeMetaMethod::methodName() const
{ return m_name; }

void FakeMetaMethod::setMethodName(const QString &name)
{ m_name = name; }

void FakeMetaMethod::setReturnType(const QString &type)
{ m_returnType = type; }

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

int FakeMetaMethod::revision() const
{ return m_revision; }

void FakeMetaMethod::setRevision(int r)
{ m_revision = r; }


FakeMetaProperty::FakeMetaProperty(const QString &name, const QString &type, bool isList,
                                   bool isWritable, bool isPointer, int revision)
    : m_propertyName(name)
    , m_type(type)
    , m_isList(isList)
    , m_isWritable(isWritable)
    , m_isPointer(isPointer)
    , m_revision(revision)
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

int FakeMetaProperty::revision() const
{ return m_revision; }


FakeMetaObject::FakeMetaObject()
{
}

QString FakeMetaObject::className() const
{ return m_className; }
void FakeMetaObject::setClassName(const QString &name)
{ m_className = name; }

void FakeMetaObject::addExport(const QString &name, const QString &package, ComponentVersion version)
{
    Export exp;
    exp.type = name;
    exp.package = package;
    exp.version = version;
    m_exports.append(exp);
}

void FakeMetaObject::setExportMetaObjectRevision(int exportIndex, int metaObjectRevision)
{
    m_exports[exportIndex].metaObjectRevision = metaObjectRevision;
}

QList<FakeMetaObject::Export> FakeMetaObject::exports() const
{ return m_exports; }
FakeMetaObject::Export FakeMetaObject::exportInPackage(const QString &package) const
{
    foreach (const Export &exp, m_exports) {
        if (exp.package == package)
            return exp;
    }
    return Export();
}

void FakeMetaObject::setSuperclassName(const QString &superclass)
{ m_superName = superclass; }
QString FakeMetaObject::superclassName() const
{ return m_superName; }

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
int FakeMetaObject::methodIndex(const QString &name) const //If performances becomes an issue, just use a nameToIdx hash
{
    for (int i=0; i<m_methods.count(); i++)
        if (m_methods[i].methodName() == name)
            return i;
    return -1;
}

QString FakeMetaObject::defaultPropertyName() const
{ return m_defaultPropertyName; }
void FakeMetaObject::setDefaultPropertyName(const QString &defaultPropertyName)
{ m_defaultPropertyName = defaultPropertyName; }

QString FakeMetaObject::attachedTypeName() const
{ return m_attachedTypeName; }
void FakeMetaObject::setAttachedTypeName(const QString &name)
{ m_attachedTypeName = name; }

FakeMetaObject::Export::Export()
    : metaObjectRevision(0)
{}
bool FakeMetaObject::Export::isValid() const
{ return version.isValid() || !package.isEmpty() || !type.isEmpty(); }
