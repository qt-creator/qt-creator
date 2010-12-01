/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef NODEMETAINFO_H
#define NODEMETAINFO_H

#include <QList>
#include <QString>
#include <QExplicitlySharedDataPointer>
#include <QIcon>

#include "corelib_global.h"
#include "invalidmetainfoexception.h"

QT_BEGIN_NAMESPACE
class QDeclarativeContext;
QT_END_NAMESPACE

namespace QmlDesigner {

class MetaInfo;
class Model;
class AbstractProperty;

namespace Internal {
    class MetaInfoPrivate;
    class MetaInfoParser;
    class SubComponentManagerPrivate;
    class ItemLibraryEntryData;
    class NodeMetaInfoPrivate;
}

class CORESHARED_EXPORT NodeMetaInfo
{
public:
    NodeMetaInfo();
    NodeMetaInfo(Model *model, QString type, int maj, int min);

    ~NodeMetaInfo();

    NodeMetaInfo(const NodeMetaInfo &other);
    NodeMetaInfo &operator=(const NodeMetaInfo &other);

    bool isValid() const;
    bool isComponent() const;
    bool hasProperty(const QString &propertyName) const;
    QStringList propertyNames() const;
    QStringList directPropertyNames() const;
    QString defaultPropertyName() const;
    bool hasDefaultProperty() const;
    QString propertyTypeName(const QString &propertyName) const;
    bool propertyIsWritable(const QString &propertyName) const;
    bool propertyIsListProperty(const QString &propertyName) const;
    bool propertyIsEnumType(const QString &propertyName) const;
    QString propertyEnumScope(const QString &propertyName) const;
    QStringList propertyKeysForEnum(const QString &propertyName) const;
    QVariant propertyCastedValue(const QString &propertyName, const QVariant &value) const;

    QList<NodeMetaInfo> superClasses() const;
    NodeMetaInfo directSuperClass() const;

    QString typeName() const;
    int majorVersion() const;
    int minorVersion() const;

    QString componentSource() const;
    QString componentFileName() const;

    bool availableInVersion(int majorVersion, int minorVersion) const;
    bool isSubclassOf(const QString& type, int majorVersion, int minorVersio) const;

private:
    QSharedPointer<Internal::NodeMetaInfoPrivate> m_privateData;
};

} //QmlDesigner

#endif // NODEMETAINFO_H
