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
class AbstractProperty;

namespace Internal {
    class MetaInfoPrivate;
    class MetaInfoParser;
    class NodeMetaInfoData;
    class SubComponentManagerPrivate;
    class ItemLibraryEntryData;
}

class PropertyMetaInfo;

class CORESHARED_EXPORT NodeMetaInfo
{
    friend class QmlDesigner::MetaInfo;
    friend class QmlDesigner::Internal::ItemLibraryEntryData;
    friend class QmlDesigner::Internal::MetaInfoPrivate;
    friend class QmlDesigner::Internal::MetaInfoParser;
    friend class QmlDesigner::Internal::SubComponentManagerPrivate;
    friend class QmlDesigner::AbstractProperty;
    friend CORESHARED_EXPORT uint qHash(const NodeMetaInfo &nodeMetaInfo);
    friend CORESHARED_EXPORT bool operator ==(const NodeMetaInfo &firstNodeInfo, const NodeMetaInfo &secondNodeInfo);

public:
    ~NodeMetaInfo();

    NodeMetaInfo(const NodeMetaInfo &other);
    NodeMetaInfo &operator=(const NodeMetaInfo &other);

    bool isValid() const;
    MetaInfo metaInfo() const;

    PropertyMetaInfo property(const QString &propertyName, bool resolveDotSyntax = false) const;

    QList<NodeMetaInfo> superClasses() const;
    NodeMetaInfo directSuperClass() const;
    QHash<QString,PropertyMetaInfo> properties(bool resolveDotSyntax = false) const;

    QString typeName() const;
    int majorVersion() const;
    int minorVersion() const;

    bool availableInVersion(int majorVersion, int minorVersion) const;

    bool hasDefaultProperty() const;
    QString defaultProperty() const;

    bool hasProperty(const QString &propertyName, bool resolveDotSyntax = false) const;
    bool isContainer() const;
    bool isComponent() const;
    bool isSubclassOf(const QString& type, int majorVersion, int minorVersio) const;

    QString componentString() const;
    QIcon icon() const;

private:
    NodeMetaInfo();
    NodeMetaInfo(const MetaInfo &metaInfo);

    void setInvalid();
    void setType(const QString &typeName, int majorVersion, int minorVersion);
    void addProperty(const PropertyMetaInfo &property);
    void setIsContainer(bool isContainer);
    void setIcon(const QIcon &icon);
    void setQmlFile(const QString &filePath);
    void setDefaultProperty(const QString &defaultProperty);
    void setSuperClass(const QString &typeName, int majorVersion = -1, int minorVersion = -1);

    bool hasLocalProperty(const QString &propertyName, bool resolveDotSyntax = false) const;
    QHash<QString,PropertyMetaInfo> dotProperties() const;

private:
    QExplicitlySharedDataPointer<Internal::NodeMetaInfoData> m_data;
};

CORESHARED_EXPORT uint qHash(const NodeMetaInfo &nodeMetaInfo);
CORESHARED_EXPORT bool operator ==(const NodeMetaInfo &firstNodeInfo,
                                           const NodeMetaInfo &secondNodeInfo);

//QDataStream& operator<<(QDataStream& stream, const NodeMetaInfo& nodeMetaInfo);
//QDataStream& operator>>(QDataStream& stream, NodeMetaInfo& nodeMetaInfo);
}

#endif // NODEMETAINFO_H
