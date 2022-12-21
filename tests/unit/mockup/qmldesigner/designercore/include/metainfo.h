// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

inline bool operator==([[maybe_unused]] const MetaInfo &first, [[maybe_unused]] const MetaInfo &second)
{
    return {};
}
inline bool operator!=([[maybe_unused]] const MetaInfo &first, [[maybe_unused]] const MetaInfo &second)
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

    static void setPluginPaths([[maybe_unused]] const QStringList &paths) {}
};

} //namespace QmlDesigner
