// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercorelib_exports.h>

#include <QString>

namespace QmlDesigner::UniqueName {

QString generate(const QString &name, std::function<bool(const QString &)> predicate);
QString generatePath(const QString &path);

QMLDESIGNERCORE_EXPORT QString generateId(QStringView id,
                                          std::function<bool(const QString &)> predicate = {});
QMLDESIGNERCORE_EXPORT QString generateId(QStringView id,
                                          const QString &fallbackId,
                                          std::function<bool(const QString &)> predicate = {});

} // namespace QmlDesigner::UniqueName
