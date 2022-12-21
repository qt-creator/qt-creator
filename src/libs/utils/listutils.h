// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>

namespace Utils {

template <class T1, class T2>
QList<T1> qwConvertList(const QList<T2> &list)
{
    QList<T1> convertedList;
    for (T2 listEntry : list) {
        convertedList << qobject_cast<T1>(listEntry);
    }
    return convertedList;
}

} // namespace Utils
