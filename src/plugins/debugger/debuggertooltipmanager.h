// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "debugger_global.h"

#include <utils/filepath.h>

#include <QPoint>

#include <functional>

namespace Debugger::Internal {

class DebuggerEngine;
class StackFrame;

class DebuggerToolTipContext
{
public:
    bool isValid() const { return !expression.isEmpty(); }
    bool matchesFrame(const StackFrame &frame) const;
    bool isSame(const DebuggerToolTipContext &other) const;
    QString toolTip() const;

    Utils::FilePath fileName;
    int position = 0;
    int line = 0;
    int column = 0;
    int scopeFromLine = 0;
    int scopeToLine = 0;
    QString function; //!< Optional, informational only.
    QString engineType;

    QPoint mousePosition;
    QString expression;
    QString iname;
    bool isCppEditor = true;
};

using DebuggerToolTipContexts = QList<DebuggerToolTipContext>;

// What to evaluate at a position, for a language Qt Creator has no expression
// parser of its own for. Line and column are one based. False leaves the file
// to the built-in heuristic, an empty answer the one position. The answer is
// only looked at when true was returned, and may come long after.
using DebuggerToolTipExpressionProvider
    = std::function<bool(const Utils::FilePath &file, int line, int column,
                         const std::function<void(const QString &)> &answer)>;
DEBUGGER_EXPORT DebuggerToolTipExpressionProvider &debuggerToolTipExpressionProvider();

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
