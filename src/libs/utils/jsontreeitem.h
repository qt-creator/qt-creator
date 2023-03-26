// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "treemodel.h"

#include "utils_global.h"

#include <QCoreApplication>
#include <QJsonValue>

namespace Utils {

class QTCREATOR_UTILS_EXPORT JsonTreeItem : public TypedTreeItem<JsonTreeItem>
{
public:
    JsonTreeItem() = default;
    JsonTreeItem(const QString &displayName, const QJsonValue &value);

    QVariant data(int column, int role) const override;
    bool canFetchMore() const override;
    void fetchMore() override;

private:
    bool canFetchObjectChildren() const;
    bool canFetchArrayChildren() const;

    QString m_name;
    QJsonValue m_value;
};

} // namespace Utils
