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

#ifndef METAINFO_H
#define METAINFO_H

#include "qmldesignercorelib_global.h"

#include <QStringList>
#include <QSharedPointer>

#include <nodemetainfo.h>
#include "itemlibraryinfo.h"

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

QMLDESIGNERCORE_EXPORT bool operator==(const MetaInfo &first, const MetaInfo &second);
QMLDESIGNERCORE_EXPORT bool operator!=(const MetaInfo &first, const MetaInfo &second);


class QMLDESIGNERCORE_EXPORT MetaInfo
{
    friend class QmlDesigner::Internal::MetaInfoPrivate;
    friend class QmlDesigner::Internal::ModelPrivate;
    friend class QmlDesigner::Internal::MetaInfoReader;
    friend class QmlDesigner::Internal::SubComponentManagerPrivate;
    friend QMLDESIGNERCORE_EXPORT bool operator==(const MetaInfo &, const MetaInfo &);

public:
    MetaInfo(const MetaInfo &metaInfo);
    ~MetaInfo();
    MetaInfo& operator=(const MetaInfo &other);

    ItemLibraryInfo *itemLibraryInfo() const;
public:
    static MetaInfo global();
    static void clearGlobal();

    static void setPluginPaths(const QStringList &paths);

private:
    bool isGlobal() const;

private:
    MetaInfo();

    Internal::MetaInfoPrivatePointer m_p;
    static MetaInfo s_global;
    static QStringList s_pluginDirs;
};

} //namespace QmlDesigner

#endif // METAINFO_H
