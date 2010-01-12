/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <qml/qml_global.h>

#include <QList>
#include <QString>
#include <QExplicitlySharedDataPointer>
#include <QIcon>

class QmlContext;

namespace Qml {

class MetaInfo;

namespace Internal {
    class MetaInfoPrivate;
    class MetaInfoParser;
    class NodeMetaInfoData;
    class SubComponentManagerPrivate;
    class ItemLibraryInfoData;
}

class PropertyMetaInfo;

class QML_EXPORT NodeMetaInfo
{
    friend class Qml::MetaInfo;
    friend class Qml::Internal::ItemLibraryInfoData;
    friend class Qml::Internal::MetaInfoPrivate;
    friend class Qml::Internal::MetaInfoParser;
    friend QML_EXPORT uint qHash(const NodeMetaInfo &nodeMetaInfo);
    friend QML_EXPORT bool operator ==(const NodeMetaInfo &firstNodeInfo, const NodeMetaInfo &secondNodeInfo);

public:
    ~NodeMetaInfo();

    NodeMetaInfo(const NodeMetaInfo &other);
    NodeMetaInfo &operator=(const NodeMetaInfo &other);

    bool isValid() const;
    MetaInfo metaInfo() const;

    QObject *createInstance(QmlContext *parentContext) const;

    PropertyMetaInfo property(const QString &propertyName, bool resolveDotSyntax = false) const;

    QList<NodeMetaInfo> superClasses() const;
    QList<NodeMetaInfo> directSuperClasses() const;
    QHash<QString,PropertyMetaInfo> properties(bool resolveDotSyntax = false) const;


    QString typeName() const;
    int majorVersion() const;
    int minorVersion() const;

    bool hasDefaultProperty() const;
    QString defaultProperty() const;

    bool hasProperty(const QString &propertyName, bool resolveDotSyntax = false) const;
    bool isContainer() const;
    bool isVisibleToItemLibrary() const;

    bool isWidget() const;
    bool isGraphicsWidget() const;
    bool isGraphicsObject() const;
    bool isQmlGraphicsItem() const;
    bool isComponent() const;
    bool isSubclassOf(const QString& type, int majorVersion = 4, int minorVersion = 6) const;

    QIcon icon() const;
    QString category() const;

private:
    NodeMetaInfo();
    NodeMetaInfo(const MetaInfo &metaInfo);

    void setInvalid();
    void setTypeName(const QString &typeName);
    void addProperty(const PropertyMetaInfo &property);
    void setIsContainer(bool isContainer);
    void setIsVisibleToItemLibrary(bool isVisibleToItemLibrary);
    void setIcon(const QIcon &icon);
    void setCategory(const QString &category);
    void setQmlFile(const QString &filePath);
    void setDefaultProperty(const QString &defaultProperty);
    void setMajorVersion(int version);
    void setMinorVersion(int version);

    bool hasLocalProperty(const QString &propertyName, bool resolveDotSyntax = false) const;
    QHash<QString,PropertyMetaInfo> dotProperties() const;

private:
    QExplicitlySharedDataPointer<Internal::NodeMetaInfoData> m_data;
};

QML_EXPORT uint qHash(const NodeMetaInfo &nodeMetaInfo);
QML_EXPORT bool operator ==(const NodeMetaInfo &firstNodeInfo,
                            const NodeMetaInfo &secondNodeInfo);
} // namespace Qml

#endif // NODEMETAINFO_H
