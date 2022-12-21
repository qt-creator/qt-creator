// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>
#include <QVector>

namespace Debugger {
namespace Internal {

////////////////////////////////////////////////////////////////////////
//
// ThreadData
//
////////////////////////////////////////////////////////////////////////

/*! A structure containing information about a single thread. */
struct ThreadData
{
    ThreadData() = default;

    enum {
        IdColumn,
        AddressColumn,
        FunctionColumn,
        FileColumn,
        LineColumn,
        StateColumn,
        NameColumn,
        TargetIdColumn,
        DetailsColumn,
        CoreColumn,
        ComboNameColumn,
        ColumnCount = CoreColumn,
    };

    // Permanent data.
    QString id;
    QString groupId;
    QString targetId;
    QString core;
    bool stopped = true;

    // State information when stopped.
    qint32  frameLevel = -1;
    qint32  lineNumber = -1;
    quint64 address = 0;
    QString function;
    QString module;
    QString fileName;
    QString details;
    QString state;
    QString name;
};

using Threads = QVector<ThreadData>;

} // namespace Internal
} // namespace Debugger
