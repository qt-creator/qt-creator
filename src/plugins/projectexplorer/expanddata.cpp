// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "expanddata.h"

#include <QVariant>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

ExpandData::ExpandData(const QString &path_, const QString &displayName_) :
    path(path_), displayName(displayName_)
{ }

bool ExpandData::operator==(const ExpandData &other) const
{
    return path == other.path && displayName == other.displayName;
}

ExpandData ExpandData::fromSettings(const QVariant &v)
{
    QStringList list = v.toStringList();
    return list.size() == 2 ? ExpandData(list.at(0), list.at(1)) : ExpandData();
}

QVariant ExpandData::toSettings() const
{
    return QVariant::fromValue(QStringList({path, displayName}));
}

size_t ProjectExplorer::Internal::qHash(const ExpandData &data)
{
    return qHash(data.path) ^ qHash(data.displayName);
}
