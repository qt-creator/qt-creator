// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "expanddata.h"

#include <QVariant>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

ExpandData::ExpandData(const QString &path, const QString &rawDisplayName, int priority)
    : path(path)
    , rawDisplayName(rawDisplayName)
    , priority(priority)
{}

bool ExpandData::operator==(const ExpandData &other) const
{
    return path == other.path && rawDisplayName == other.rawDisplayName
           && priority == other.priority;
}

ExpandData ExpandData::fromSettings(const QVariant &v)
{
    const QVariantList list = v.toList();
    if (list.size() < 2)
        return {};
    return ExpandData(
        list.at(0).toString(),
        list.size() == 3 ? list.at(2).toString() : QString(),
        list.at(1).toInt());
}

QVariant ExpandData::toSettings() const
{
    return QVariantList{path, priority, rawDisplayName};
}

size_t ProjectExplorer::Internal::qHash(const ExpandData &data)
{
    return qHash(data.path) ^ qHash(data.rawDisplayName) ^ data.priority;
}
