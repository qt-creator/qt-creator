/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <algorithm>

#include <QString>
#include <QDebug>
#include <QVariant>

namespace ProjectExplorer {
QDebug &operator<<(QDebug &debug, const QList<QVariant> &container);
QDebug &operator<<(QDebug &debug, const QMap<QString, QVariant> &container);
QDebug &debugString(QDebug &debug, const QString &value);

inline QDebug &operator<<(QDebug &dbg, const QVariant &var)
{
    switch (var.typeId()) {
    case QMetaType::UnknownType:
        dbg << "(invalid)";
        break;
    case QMetaType::QString:
        debugString(dbg, var.toString());
        break;
    case QMetaType::QStringList:
        dbg << var.toList();
        break;
    case QVariant::List:
        dbg << var.toList();
        break;
    case QVariant::Map:
        dbg << var.toMap();
        break;
    default: {
        QMetaType metaType(var.typeId());
        bool streamed = metaType.debugStream(dbg, var.data());
        if (!streamed && var.canConvert<QString>())
            dbg << var.toString();
    } break;
    }

    return dbg;
}

inline QDebug &operator<<(QDebug &dbg, const QList<QVariant> &c)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();

    dbg << "[";
    if (!c.empty()) {
        std::for_each(std::cbegin(c), std::cend(c) - 1, [&dbg](auto item) {
            dbg << item << ", ";
        });
        dbg << c.back();
    }

    dbg << "] ";

    return dbg;
}

inline QDebug &operator<<(QDebug &dbg, const QMap<QString, QVariant> &m)
{
    QDebugStateSaver saver(dbg);

    dbg.nospace();

    dbg << "{";
    if (!m.empty()) {
        auto keys = m.keys();
        std::for_each(std::cbegin(keys), std::cend(keys) - 1, [&dbg, &m](const QString &k) {
            dbg << k << ":" << m[k] << ",";
        });
        auto lastKey = keys.back();
        dbg << lastKey << ":" << m[lastKey];
    }

    dbg << "}";

    return dbg;
}

inline QDebug &debugString(QDebug &dbg, const QString &value)
{
    QDebugStateSaver saver(dbg);

    dbg.noquote();
    dbg << value;

    return dbg;
}

} // ProjectExplorer
