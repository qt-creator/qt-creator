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

#include <QList>
#include <QString>
#include <QIcon>

#include "qmldesignercorelib_global.h"
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
    class MetaInfoReader;
    class SubComponentManagerPrivate;
    class ItemLibraryEntryData;
    class NodeMetaInfoPrivate;
}

class QMLDESIGNERCORE_EXPORT NodeMetaInfo
{
public:
    NodeMetaInfo();
    NodeMetaInfo(Model *model, TypeName type, int maj, int min);

    ~NodeMetaInfo();

    NodeMetaInfo(const NodeMetaInfo &other);
    NodeMetaInfo &operator=(const NodeMetaInfo &other);

    bool isValid() const;
    bool isFileComponent() const;
    bool hasProperty(const PropertyName &propertyName) const;
    PropertyNameList propertyNames() const;
    PropertyNameList signalNames() const;
    PropertyNameList directPropertyNames() const;
    PropertyName defaultPropertyName() const;
    bool hasDefaultProperty() const;
    TypeName propertyTypeName(const PropertyName &propertyName) const;
    bool propertyIsWritable(const PropertyName &propertyName) const;
    bool propertyIsListProperty(const PropertyName &propertyName) const;
    bool propertyIsEnumType(const PropertyName &propertyName) const;
    bool propertyIsPrivate(const PropertyName &propertyName) const;
    QString propertyEnumScope(const PropertyName &propertyName) const;
    QStringList propertyKeysForEnum(const PropertyName &propertyName) const;
    QVariant propertyCastedValue(const PropertyName &propertyName, const QVariant &value) const;

    QList<NodeMetaInfo> classHierarchy() const;
    QList<NodeMetaInfo> superClasses() const;
    NodeMetaInfo directSuperClass() const;

    bool defaultPropertyIsComponent() const;

    TypeName typeName() const;
    int majorVersion() const;
    int minorVersion() const;

    QString componentSource() const;
    QString componentFileName() const;

    bool hasCustomParser() const;

    bool availableInVersion(int majorVersion, int minorVersion) const;
    bool isSubclassOf(const TypeName &type, int majorVersion = -1, int minorVersion = -1) const;

    bool isGraphicalItem() const;
    bool isLayoutable() const;
    bool isView() const;
    bool isTabView() const;

    QString importDirectoryPath() const;

    static void clearCache();

private:
    QSharedPointer<Internal::NodeMetaInfoPrivate> m_privateData;
};

} //QmlDesigner
