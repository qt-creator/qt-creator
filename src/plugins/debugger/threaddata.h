// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>
#include <QString>

namespace Debugger::Internal {

enum class StopReason : quint8 { None, Breakpoint, Watchpoint, Signal, Exception, Other };

// ThreadData

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
    // Why the thread stopped, the response id of the breakpoint or watchpoint
    // it stopped at, and the signal it stopped by. A pending stop is one the
    // debugger holds back for a later stop.
    StopReason stopReason = StopReason::None;
    QString stopBreakpoint;
    QString stopSignal;
    bool stopPending = false;
};

using Threads = QList<ThreadData>;

} // Debugger::Internal
