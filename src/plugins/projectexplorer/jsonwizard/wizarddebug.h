// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
