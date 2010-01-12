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

#ifndef METAINFO_H
#define METAINFO_H

#include <qml/qml_global.h>
#include <qml/metatype/nodemetainfo.h>
#include <qml/metatype/propertymetainfo.h>

#include <QMultiHash>
#include <QString>
#include <QStringList>
#include <QtCore/QSharedPointer>

namespace Qml {

class ModelNode;
class AbstractProperty;

namespace Internal {
    class MetaInfoPrivate;
    class ModelPrivate;
    class SubComponentManagerPrivate;
    typedef QSharedPointer<MetaInfoPrivate> MetaInfoPrivatePointer;
}

QML_EXPORT bool operator==(const MetaInfo &first, const MetaInfo &second);
QML_EXPORT bool operator!=(const MetaInfo &first, const MetaInfo &second);

class QML_EXPORT MetaInfo
{
    friend class Qml::Internal::MetaInfoPrivate;
    friend class Qml::Internal::MetaInfoParser;
    friend class Qml::NodeMetaInfo;
    friend bool Qml::operator==(const MetaInfo &, const MetaInfo &);

public:
    MetaInfo(const MetaInfo &metaInfo);
    ~MetaInfo();
    MetaInfo& operator=(const MetaInfo &other);

    QList<NodeMetaInfo> allTypes() const;

    bool hasNodeMetaInfo(const QString &typeName, int majorVersion = 4, int minorVersion = 6) const;
    // ### makes no sense since ModelNode has minor/major version
    NodeMetaInfo nodeMetaInfo(const ModelNode &node) const;
    NodeMetaInfo nodeMetaInfo(const QString &typeName, int majorVersion = 4, int minorVersion = 6) const;

    // TODO: Move these to private
    bool isSubclassOf(const QString &className, const QString &superClassName) const;
    bool isSubclassOf(const ModelNode &modelNode, const QString &superClassName) const;

    bool hasEnumerator(const QString &enumeratorName) const;

    QStringList itemLibraryItems() const;

public:
    static MetaInfo global();
    static void clearGlobal();

    static void setPluginPaths(const QStringList &paths);

private:
    QStringList superClasses(const QString &className) const;
    QStringList directSuperClasses(const QString &className) const;
    QList<NodeMetaInfo> superClasses(const NodeMetaInfo &nodeMetaInfo) const;
    QList<NodeMetaInfo> directSuperClasses(const NodeMetaInfo &nodeMetaInfo) const;

    void addSuperClassRelationship(const QString &superClassName, const QString &className);

    void addNodeInfo(NodeMetaInfo &info, const QString &baseType);

    bool isGlobal() const;

private:
    MetaInfo();

    Internal::MetaInfoPrivatePointer m_p;
    static MetaInfo s_global;
    static QStringList s_pluginDirs;
};

} // namespace Qml

#endif // METAINFO_H
