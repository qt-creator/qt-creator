/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "environmentfwd.h"
#include "utils_global.h"

#include <QStringList>
#include <QVariantList>
#include <QVector>

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

public:
    QString name;
    QString value;
    Operation operation = Unset;

private:
    void apply(NameValueDictionary *dictionary, Operation op) const;
};

QTCREATOR_UTILS_EXPORT QDebug operator<<(QDebug debug, const NameValueItem &i);

} // namespace Utils
