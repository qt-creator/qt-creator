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

#ifndef FAKEMETAOBJECT_H
#define FAKEMETAOBJECT_H

#include "languageutils_global.h"
#include "componentversion.h"

#include <QString>
#include <QStringList>
#include <QList>
#include <QHash>
#include <QSharedPointer>

namespace LanguageUtils {

class LANGUAGEUTILS_EXPORT FakeMetaEnum {
    QString m_name;
    QStringList m_keys;
    QList<int> m_values;

public:
    FakeMetaEnum();
    explicit FakeMetaEnum(const QString &name);

    bool isValid() const;

    QString name() const;
    void setName(const QString &name);

    void addKey(const QString &key, int value);
    QString key(int index) const;
    int keyCount() const;
    QStringList keys() const;
    bool hasKey(const QString &key) const;
};

class LANGUAGEUTILS_EXPORT FakeMetaMethod {
public:
    enum {
        Signal,
        Slot,
        Method
    };

    enum {
        Private,
        Protected,
        Public
    };

public:
    FakeMetaMethod();
    explicit FakeMetaMethod(const QString &name, const QString &returnType = QString());

    QString methodName() const;
    void setMethodName(const QString &name);

    void setReturnType(const QString &type);

    QStringList parameterNames() const;
    QStringList parameterTypes() const;
    void addParameter(const QString &name, const QString &type);

    int methodType() const;
    void setMethodType(int methodType);

    int access() const;

    int revision() const;
    void setRevision(int r);

private:
    QString m_name;
    QString m_returnType;
    QStringList m_paramNames;
    QStringList m_paramTypes;
    int m_methodTy;
    int m_methodAccess;
    int m_revision;
};

class LANGUAGEUTILS_EXPORT FakeMetaProperty {
    QString m_propertyName;
    QString m_type;
    bool m_isList;
    bool m_isWritable;
    bool m_isPointer;
    int m_revision;

public:
    FakeMetaProperty(const QString &name, const QString &type, bool isList, bool isWritable, bool isPointer, int revision);

    QString name() const;
    QString typeName() const;

    bool isList() const;
    bool isWritable() const;
    bool isPointer() const;
    int revision() const;
};

class LANGUAGEUTILS_EXPORT FakeMetaObject {
    Q_DISABLE_COPY(FakeMetaObject);

public:
    typedef QSharedPointer<FakeMetaObject> Ptr;
    typedef QSharedPointer<const FakeMetaObject> ConstPtr;

    class LANGUAGEUTILS_EXPORT Export {
    public:
        Export();

        QString package;
        QString type;
        ComponentVersion version;
        int metaObjectRevision;

        bool isValid() const;
    };

private:
    QString m_className;
    QList<Export> m_exports;
    QString m_superName;
    QList<FakeMetaEnum> m_enums;
    QHash<QString, int> m_enumNameToIndex;
    QList<FakeMetaProperty> m_props;
    QHash<QString, int> m_propNameToIdx;
    QList<FakeMetaMethod> m_methods;
    QString m_defaultPropertyName;
    QString m_attachedTypeName;

public:
    FakeMetaObject();

    QString className() const;
    void setClassName(const QString &name);

    void addExport(const QString &name, const QString &package, ComponentVersion version);
    void setExportMetaObjectRevision(int exportIndex, int metaObjectRevision);
    QList<Export> exports() const;
    Export exportInPackage(const QString &package) const;

    void setSuperclassName(const QString &superclass);
    QString superclassName() const;

    void addEnum(const FakeMetaEnum &fakeEnum);
    int enumeratorCount() const;
    int enumeratorOffset() const;
    FakeMetaEnum enumerator(int index) const;
    int enumeratorIndex(const QString &name) const;

    void addProperty(const FakeMetaProperty &property);
    int propertyCount() const;
    int propertyOffset() const;
    FakeMetaProperty property(int index) const;
    int propertyIndex(const QString &name) const;

    void addMethod(const FakeMetaMethod &method);
    int methodCount() const;
    int methodOffset() const;
    FakeMetaMethod method(int index) const;
    int methodIndex(const QString &name) const; // Note: Returns any method with that name in case of overloads

    QString defaultPropertyName() const;
    void setDefaultPropertyName(const QString &defaultPropertyName);

    QString attachedTypeName() const;
    void setAttachedTypeName(const QString &name);
};

} // namespace LanguageUtils

#endif // FAKEMETAOBJECT_H
