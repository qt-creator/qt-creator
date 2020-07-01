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

#include "qmldesignercorelib_global.h"

#include <QSharedPointer>
#include <QStringList>

#include "itemlibraryinfo.h"
#include <nodemetainfo.h>

namespace QmlDesigner {

class ModelNode;
class AbstractProperty;
class ItemLibraryInfo;

inline bool operator==(const MetaInfo &first, const MetaInfo &second)
{
    return {};
}
inline bool operator!=(const MetaInfo &first, const MetaInfo &second)
{
    return {};
}

class QMLDESIGNERCORE_EXPORT MetaInfo
{
public:
    ItemLibraryInfo *itemLibraryInfo() const { return {}; }

public:
    static MetaInfo global() { return {}; }
    static void clearGlobal() {}

    static void setPluginPaths(const QStringList &paths) {}
};

} //namespace QmlDesigner
