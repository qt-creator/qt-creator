// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>
#include <QHash>
#include <QDebug>

namespace ProjectExplorer::Internal {

class ExpandData
{
public:
    ExpandData() = default;
    ExpandData(const QString &path, const QString &rawDisplayName, int priority);
    bool operator==(const ExpandData &other) const;

    static ExpandData fromSettings(const QVariant &v);
    QVariant toSettings() const;

    friend size_t qHash(const ExpandData &data, size_t seed);

    QString path;
    QString rawDisplayName;
    int priority = 0;
};

} // namespace ProjectExplorer::Internal
