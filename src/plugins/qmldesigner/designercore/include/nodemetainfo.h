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

    QObject *createInstance(QDeclarativeContext *parentContext) const;

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
