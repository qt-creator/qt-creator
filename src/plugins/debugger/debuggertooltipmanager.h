// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "debuggerconstants.h"

#include <utils/filepath.h>

#include <QCoreApplication>
#include <QDate>
#include <QPoint>

namespace Debugger::Internal {

class DebuggerEngine;
class StackFrame;

class DebuggerToolTipContext
{
public:
    DebuggerToolTipContext();
    bool isValid() const { return !expression.isEmpty(); }
    bool matchesFrame(const StackFrame &frame) const;
    bool isSame(const DebuggerToolTipContext &other) const;
    QString toolTip() const;

    Utils::FilePath fileName;
    int position;
    int line;
    int column;
    int scopeFromLine;
    int scopeToLine;
    QString function; //!< Optional, informational only.
    QString engineType;
    QDate creationDate;

    QPoint mousePosition;
    QString expression;
    QString iname;
    bool isCppEditor;
};

using DebuggerToolTipContexts = QList<DebuggerToolTipContext>;

class DebuggerToolTipManager
{
    Q_DISABLE_COPY_MOVE(DebuggerToolTipManager)

public:
    explicit DebuggerToolTipManager(DebuggerEngine *engine);
    ~DebuggerToolTipManager();

    void deregisterEngine();
    void updateToolTips();
    bool hasToolTips() const;

    DebuggerToolTipContexts pendingTooltips() const;

    void closeAllToolTips();
    void resetLocation();

private:
    class DebuggerToolTipManagerPrivate *d;
};

} // Debugger::Internal
