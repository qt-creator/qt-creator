// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "environmentfwd.h"
#include "utils_global.h"

#include <QStringList>
#include <QVariantList>

namespace Utils {

class QTCREATOR_UTILS_EXPORT NameValueItem
{
public:
    enum Operation : char { SetEnabled, Unset, Prepend, Append, SetDisabled };
    NameValueItem() = default;
    NameValueItem(const QString &key, const QString &value, Operation operation = SetEnabled)
        : name(key)
        , value(value)
        , operation(operation)
    {}

    void apply(NameValueDictionary *dictionary) const { apply(dictionary, operation); }

    static void sort(NameValueItems *list);
    static NameValueItems fromStringList(const QStringList &list);
    static QStringList toStringList(const NameValueItems &list);
    static NameValueItems itemsFromVariantList(const QVariantList &list);
    static QVariantList toVariantList(const NameValueItems &list);
    static NameValueItem itemFromVariantList(const QVariantList &list);
    static QVariantList toVariantList(const NameValueItem &item);

    friend bool operator==(const NameValueItem &first, const NameValueItem &second)
    {
        return first.operation == second.operation && first.name == second.name
               && first.value == second.value;
    }
    friend bool operator!=(const NameValueItem &first, const NameValueItem &second)
    {
        return !(first == second);
    }

    friend QTCREATOR_UTILS_EXPORT QDebug operator<<(QDebug debug, const NameValueItem &i);

public:
    QString name;
    QString value;
    Operation operation = Unset;

private:
    void apply(NameValueDictionary *dictionary, Operation op) const;
};

} // namespace Utils
