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

#ifndef METAINFO_H
#define METAINFO_H

#include "corelib_global.h"

#include <QMultiHash>
#include <QString>
#include <QStringList>
#include <QtCore/QSharedPointer>

#include <nodemetainfo.h>
#include "propertymetainfo.h"
#include "itemlibraryinfo.h"
#include "enumeratormetainfo.h"


namespace QmlDesigner {

class ModelNode;
class AbstractProperty;
class ItemLibraryInfo;

namespace Internal {
    class MetaInfoPrivate;
    class ModelPrivate;
    class SubComponentManagerPrivate;
    typedef QSharedPointer<MetaInfoPrivate> MetaInfoPrivatePointer;
}

CORESHARED_EXPORT bool operator==(const MetaInfo &first, const MetaInfo &second);
CORESHARED_EXPORT bool operator!=(const MetaInfo &first, const MetaInfo &second);


class CORESHARED_EXPORT MetaInfo
{
    friend class QmlDesigner::Internal::MetaInfoPrivate;
    friend class QmlDesigner::Internal::ModelPrivate;
    friend class QmlDesigner::Internal::MetaInfoParser;
    friend class QmlDesigner::Internal::SubComponentManagerPrivate;
    friend class QmlDesigner::NodeMetaInfo;
    friend bool QmlDesigner::operator==(const MetaInfo &, const MetaInfo &);

public:
    MetaInfo(const MetaInfo &metaInfo);
    ~MetaInfo();
    MetaInfo& operator=(const MetaInfo &other);

    bool hasNodeMetaInfo(const QString &typeName, int majorVersion = -1, int minorVersion = -1) const;
    NodeMetaInfo nodeMetaInfo(const QString &typeName, int majorVersion = -1, int minorVersion = -1) const;

    bool hasEnumerator(const QString &enumeratorName) const;
    EnumeratorMetaInfo enumerator(const QString &enumeratorName) const;

    ItemLibraryInfo *itemLibraryInfo() const;

    QString fromQtTypes(const QString &type) const;


public:
    static MetaInfo global();
    static void clearGlobal();

    static void setPluginPaths(const QStringList &paths);

private:
    void addNodeInfo(NodeMetaInfo &info);
    void removeNodeInfo(NodeMetaInfo &info);

    EnumeratorMetaInfo addEnumerator(const QString &enumeratorScope, const QString &enumeratorName);
    EnumeratorMetaInfo addFlag(const QString &enumeratorScope, const QString &enumeratorName);

    bool isGlobal() const;

private:
    MetaInfo();

    Internal::MetaInfoPrivatePointer m_p;
    static MetaInfo s_global;
    static QStringList s_pluginDirs;
};

} //namespace QmlDesigner

#endif // METAINFO_H
