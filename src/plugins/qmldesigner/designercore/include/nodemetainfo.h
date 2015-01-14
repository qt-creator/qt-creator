/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef NODEMETAINFO_H
#define NODEMETAINFO_H

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

    QList<NodeMetaInfo> superClasses() const;
    NodeMetaInfo directSuperClass() const;

    QList<TypeName> superClassNames() const;

    bool defaultPropertyIsComponent() const;

    TypeName typeName() const;
    int majorVersion() const;
    int minorVersion() const;

    QString componentSource() const;
    QString componentFileName() const;

    bool hasCustomParser() const;

    bool availableInVersion(int majorVersion, int minorVersion) const;
    bool isSubclassOf(const TypeName &type, int majorVersion, int minorVersio) const;

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

#endif // NODEMETAINFO_H
