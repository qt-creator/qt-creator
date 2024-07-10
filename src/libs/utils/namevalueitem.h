// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "environmentfwd.h"
#include "utils_global.h"

#include <QStringList>
#include <QVariantList>

namespace Utils {

class QTCREATOR_UTILS_EXPORT EnvironmentItem
{
public:
    enum Operation : char { SetEnabled, Unset, Prepend, Append, SetDisabled, Comment };
    EnvironmentItem() = default;
    EnvironmentItem(const QString &key, const QString &value, Operation operation = SetEnabled)
        : name(key)
        , value(value)
        , operation(operation)
    {}

    void apply(NameValueDictionary *dictionary) const { apply(dictionary, operation); }

    static void sort(EnvironmentItems *list);
    static EnvironmentItems fromStringList(const QStringList &list);
    static QStringList toStringList(const EnvironmentItems &list);
    static EnvironmentItems itemsFromVariantList(const QVariantList &list);
    static QVariantList toVariantList(const EnvironmentItems &list);
    static EnvironmentItem itemFromVariantList(const QVariantList &list);
    static QVariantList toVariantList(const EnvironmentItem &item);

    friend bool operator==(const EnvironmentItem &first, const EnvironmentItem &second)
    {
        return first.operation == second.operation && first.name == second.name
               && first.value == second.value;
    }
    friend bool operator!=(const EnvironmentItem &first, const EnvironmentItem &second)
    {
        return !(first == second);
    }

    friend QTCREATOR_UTILS_EXPORT QDebug operator<<(QDebug debug, const EnvironmentItem &i);

public:
    QString name;
    QString value;
    Operation operation = Unset;

private:
    void apply(NameValueDictionary *dictionary, Operation op) const;
};

} // namespace Utils
