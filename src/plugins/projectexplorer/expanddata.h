// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>
#include <QHash>
#include <QDebug>

namespace ProjectExplorer {
namespace Internal {

class ExpandData
{
public:
    ExpandData() = default;
    ExpandData(const QString &path, int priority);
    bool operator==(const ExpandData &other) const;

    static ExpandData fromSettings(const QVariant &v);
    QVariant toSettings() const;

    QString path;
    int priority = 0;
};

size_t qHash(const ExpandData &data);

} // namespace Internal
} // namespace ProjectExplorer
