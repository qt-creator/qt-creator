// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldesignercorelib_global.h"

#include <QIcon>

namespace QmlDesigner {

class ItemLibraryEntry
{
public:
    QString name() const { return {}; }
    TypeName typeName() const { return {}; }
    QIcon typeIcon() const { return {}; }
    QString libraryEntryIconPath() const { return {}; }
};

class ItemLibraryInfo
{
public:
    QList<ItemLibraryEntry> entries() const { return {}; }
    QList<ItemLibraryEntry> entriesForType([[maybe_unused]] const QByteArray &typeName,
                                           [[maybe_unused]] int majorVersion,
                                           [[maybe_unused]] int minorVersion) const
    {
        return {};
    }
    ItemLibraryEntry entry([[maybe_unused]] const QString &name) const { return {}; }
};

} // namespace QmlDesigner
